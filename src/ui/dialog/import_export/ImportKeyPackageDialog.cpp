/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ImportKeyPackageDialog.h"

#include <QFileDialog>

#include "core/function/KeyPackageOperator.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/dialog/SecretPrompt.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/MetaListPanel.h"
#include "ui/widgets/SecretEntryPanel.h"

namespace GpgFrontend::UI {

namespace {

auto StepIndex(KeyPackageImportStep step) -> int {
  return static_cast<int>(step);
}

/// Rows the key table shows before it scrolls, so a large package does not
/// stretch the wizard past the size of its other steps.
constexpr int kKeyTableVisibleRows = 4;

/// Where the fingerprint of a key row is kept, on its checkable first cell.
constexpr int kFprRole = Qt::UserRole + 1;

/// The end of a long path is the part that names the file.
void ShowPath(QLineEdit* edit, const QString& path) {
  const auto native = QDir::toNativeSeparators(path);
  edit->setText(native);
  edit->setToolTip(native);
  edit->setCursorPosition(static_cast<int>(native.size()));
}

auto Inspect(const QString& path) -> KeyPackageImportFile {
  KeyPackageImportFile file;
  file.path = path;
  if (path.isEmpty()) return file;

  const QFileInfo info(path);
  file.is_file = info.isFile();
  file.readable = info.isReadable();
  file.bytes = info.size();
  return file;
}

/// A flat button drawn in the link colour, so it still reads as clickable.
auto LinkButton(const QString& text, QWidget* parent) -> QPushButton* {
  auto* button = new QPushButton(text, parent);
  button->setFlat(true);
  button->setAutoDefault(false);
  button->setCursor(Qt::PointingHandCursor);
  auto palette = button->palette();
  palette.setColor(QPalette::ButtonText, palette.color(QPalette::Link));
  button->setPalette(palette);
  return button;
}

}  // namespace

ImportKeyPackageDialog::ImportKeyPackageDialog(int channel, QWidget* parent)
    : GeneralWizard(typeid(ImportKeyPackageDialog).name(), tr("Import keys"),
                    parent),
      current_gpg_context_channel_(channel) {
  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return current_gpg_context_channel_; },
      [this](int channel) { current_gpg_context_channel_ = channel; });

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Import Key Package"));

  build_files_page();
  build_unlock_page();
  build_keys_page();

  connect(package_button_, &QPushButton::clicked, this,
          &ImportKeyPackageDialog::slot_choose_package);
  connect(key_file_button_, &QPushButton::clicked, this,
          &ImportKeyPackageDialog::slot_choose_key_file);
  connect(key_file_reset_, &QPushButton::clicked, this,
          &ImportKeyPackageDialog::slot_reset_key_file);
  connect(entry_, &SecretEntryPanel::SignalStateChanged, this, [this]() {
    // A different PIN is a different attempt: what the old one opened is
    // not kept beside it.
    drop_opened();
    slot_state_changed();
  });
  connect(this, &QWizard::currentIdChanged, this,
          &ImportKeyPackageDialog::slot_page_changed);

  slot_state_changed();
}

auto ImportKeyPackageDialog::add_page(KeyPackageImportStep step)
    -> GeneralWizardPage* {
  auto* page = AddStep(StepIndex(step), kKeyPackageImportStepCount,
                       KeyPackageImportStepTitle(step));
  pages_.at(StepIndex(step)) = page;
  return page;
}

void ImportKeyPackageDialog::build_files_page() {
  auto* page = add_page(KeyPackageImportStep::kFiles);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Choose the key package and the key file that came with it. A "
             "key file saved beside the package is found by itself."),
          page);

  auto* grid = new QGridLayout();
  grid->setHorizontalSpacing(6);
  grid->setVerticalSpacing(6);
  grid->setColumnStretch(1, 1);

  // Each path is a read-only field with Browse beside it: the platform's own
  // shape for "a file, and the button that picks it".
  const auto add_path_row = [&](int row, const QString& caption,
                                const QString& accessible, QLineEdit*& edit,
                                QPushButton*& button) {
    auto* label = new QLabel(caption, page);
    edit = new QLineEdit(page);
    edit->setReadOnly(true);
    edit->setPlaceholderText(tr("No file chosen yet"));
    label->setBuddy(edit);
    button = new QPushButton(tr("Browse..."), page);
    button->setAutoDefault(false);
    button->setAccessibleName(accessible);
    grid->addWidget(label, row, 0);
    grid->addWidget(edit, row, 1);
    grid->addWidget(button, row, 2);
  };
  add_path_row(0, tr("Key package"), tr("Choose the key package"),
               package_edit_, package_button_);
  add_path_row(1, tr("Key file"), tr("Choose the key file"), key_file_edit_,
               key_file_button_);

  // Where the key file came from, and the way back to the one beside the
  // package when another was chosen.
  key_file_note_ = CreateDetailLabel({}, page);
  key_file_note_->setWordWrap(true);
  key_file_reset_ = LinkButton(tr("Use the one beside the package"), page);
  auto* note_row = new QHBoxLayout();
  note_row->setSpacing(6);
  note_row->addWidget(key_file_note_, 1);
  note_row->addWidget(key_file_reset_);
  grid->addLayout(note_row, 2, 1, 1, 2);

  layout->addLayout(grid);

  problem_label_ = new QLabel(page);
  problem_label_->setWordWrap(true);
  SetLabelTextColor(problem_label_, WarningColor(palette()));
  problem_label_->hide();
  layout->addWidget(problem_label_);

  layout->addStretch(1);

  page->complete = [this]() {
    return CanAdvanceKeyPackageImport(KeyPackageImportStep::kFiles, choice());
  };
}

void ImportKeyPackageDialog::build_unlock_page() {
  auto* page = add_page(KeyPackageImportStep::kUnlock);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Enter the PIN that protects the key file. It was chosen when the "
             "package was exported. Nothing is imported yet."),
          page);

  auto texts = DefaultSecretPromptTexts(SecretPromptSubject::kKeyPackage,
                                        SecretPromptMode::kUNLOCK);
  texts.hint = {};

  SecretEntryPanel::Config config;
  config.ask_current = true;
  config.ask_new = false;
  config.compact = true;
  config.texts = texts;
  entry_ = new SecretEntryPanel(config, page);
  entry_->setAccessibleName(tr("PIN that protects the key file"));
  layout->addWidget(entry_);

  // Which files the PIN is about to open.
  review_ = new MetaListPanel(page);
  layout->addWidget(review_);

  layout->addStretch(1);

  page->complete = [this]() {
    return !busy_ &&
           CanAdvanceKeyPackageImport(KeyPackageImportStep::kUnlock, choice());
  };

  // Continue opens the package. The wizard moves on only once it has, so a
  // wrong PIN is answered here, where it was typed.
  page->validate = [this]() {
    if (opened_) return true;
    if (!busy_ &&
        CanAdvanceKeyPackageImport(KeyPackageImportStep::kUnlock, choice())) {
      start_open();
    }
    return false;
  };
}

void ImportKeyPackageDialog::build_keys_page() {
  auto* page = add_page(KeyPackageImportStep::kKeys);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("These keys are in the package. Untick any you do not want to "
             "import."),
          page);

  selection_label_ = new QLabel(page);
  selection_label_->setWordWrap(true);

  auto* all = LinkButton(tr("Select all"), page);
  auto* none = LinkButton(tr("Select none"), page);
  const auto tick_all = [this](Qt::CheckState state) {
    for (int r = 0; r < keys_model_->rowCount(); ++r) {
      if (auto* item = keys_model_->item(r, 0)) item->setCheckState(state);
    }
  };
  connect(all, &QPushButton::clicked, this,
          [tick_all]() { tick_all(Qt::Checked); });
  connect(none, &QPushButton::clicked, this,
          [tick_all]() { tick_all(Qt::Unchecked); });

  auto* selection_row = new QHBoxLayout();
  selection_row->setSpacing(6);
  selection_row->addWidget(selection_label_, 1);
  selection_row->addWidget(all);
  selection_row->addWidget(none);
  layout->addLayout(selection_row);

  // A table rather than a list: two keys can share a name, and the email and
  // key ID beside it are what tell them apart before anything is imported.
  keys_model_ = new QStandardItemModel(this);
  keys_model_->setHorizontalHeaderLabels(KeyPackageImportKeyColumns());
  connect(keys_model_, &QStandardItemModel::itemChanged, this,
          &ImportKeyPackageDialog::slot_key_item_changed);

  keys_table_ = new QTableView(page);
  keys_table_->setModel(keys_model_);
  keys_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  keys_table_->setSelectionMode(QAbstractItemView::NoSelection);
  keys_table_->setShowGrid(false);
  keys_table_->setWordWrap(false);
  keys_table_->setTextElideMode(Qt::ElideRight);
  keys_table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  keys_table_->verticalHeader()->hide();
  keys_table_->horizontalHeader()->setHighlightSections(false);
  keys_table_->setAccessibleName(tr("Keys in the package"));

  // Name and email share what is left; the ID and the status take what they
  // need, so neither is ever elided.
  auto* header = keys_table_->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setSectionResizeMode(1, QHeaderView::Stretch);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(3, QHeaderView::ResizeToContents);

  keys_table_->setFixedHeight(
      header->sizeHint().height() +
      (kKeyTableVisibleRows *
       keys_table_->verticalHeader()->defaultSectionSize()) +
      (2 * keys_table_->frameWidth()));
  layout->addWidget(keys_table_);

  layout->addStretch(1);

  page->complete = [this]() {
    return !busy_ && opened_ &&
           CanAdvanceKeyPackageImport(KeyPackageImportStep::kKeys, choice());
  };

  // QWizard asks this page before it closes. The import runs first, and the
  // wizard only closes once it has worked.
  page->validate = [this]() {
    if (imported_) return true;
    if (!busy_ && opened_ &&
        CanAdvanceKeyPackageImport(KeyPackageImportStep::kKeys, choice())) {
      start_import();
    }
    return false;
  };
}

void ImportKeyPackageDialog::slot_page_changed(int id) {
  // Queued: QWizard settles focus on its own buttons after announcing the new
  // page, and this has to land after that.
  QTimer::singleShot(0, this, [this, id]() {
    if (currentId() != id) return;

    switch (static_cast<KeyPackageImportStep>(id)) {
      case KeyPackageImportStep::kFiles:
        if (package_path_.isEmpty()) package_button_->setFocus();
        break;
      case KeyPackageImportStep::kUnlock:
        entry_->FocusFirstField();
        break;
      case KeyPackageImportStep::kKeys:
        // The button rather than the table: a focused table frames its first
        // cell, which reads as a selection. Tab still reaches the ticks.
        if (auto* finish = button(QWizard::FinishButton)) finish->setFocus();
        break;
    }
  });
}

auto ImportKeyPackageDialog::sibling_key_file() const -> QString {
  const auto sibling = KeyPackageImportKeyFileFor(package_path_);
  return QFileInfo(sibling).isFile() ? sibling : QString{};
}

auto ImportKeyPackageDialog::choice() const -> KeyPackageImportChoice {
  KeyPackageImportChoice choice;
  choice.package = Inspect(package_path_);
  choice.key_file =
      Inspect(key_file_path_.isEmpty() ? sibling_key_file() : key_file_path_);
  choice.pin_entered = entry_->Acceptable();
  choice.keys_selected = static_cast<int>(selected_fprs().size());
  return choice;
}

auto ImportKeyPackageDialog::selected_fprs() const -> QStringList {
  QStringList fprs;
  if (keys_model_ == nullptr) return fprs;
  for (int r = 0; r < keys_model_->rowCount(); ++r) {
    const auto* item = keys_model_->item(r, 0);
    if (item != nullptr && item->checkState() == Qt::Checked) {
      fprs.append(item->data(kFprRole).toString());
    }
  }
  return fprs;
}

void ImportKeyPackageDialog::slot_choose_package() {
  const auto folder = package_path_.isEmpty()
                          ? GetDefaultUserFilePath()
                          : QFileInfo(package_path_).absolutePath();

  const auto chosen = QFileDialog::getOpenFileName(
      this, tr("Choose Key Package"), folder,
      tr("Key Package") + " (*.gfpack);;" + tr("All Files") + " (*)");
  if (chosen.isEmpty()) return;

  package_path_ = chosen;
  drop_opened();
  slot_state_changed();

  if (auto* next = button(QWizard::NextButton); next && next->isEnabled()) {
    next->setFocus();
  } else if (choice().key_file.path.isEmpty()) {
    key_file_button_->setFocus();
  }
}

void ImportKeyPackageDialog::slot_choose_key_file() {
  const auto current = choice().key_file.path;
  const auto folder = !current.isEmpty() ? QFileInfo(current).absolutePath()
                      : !package_path_.isEmpty()
                          ? QFileInfo(package_path_).absolutePath()
                          : GetDefaultUserFilePath();

  const auto chosen = QFileDialog::getOpenFileName(
      this, tr("Choose Key File"), folder,
      tr("Key File") + " (*.key);;" + tr("All Files") + " (*)");
  if (chosen.isEmpty()) return;

  // Choosing the one beside the package keeps following the package.
  key_file_path_ =
      QDir::cleanPath(chosen) == QDir::cleanPath(sibling_key_file()) ? QString{}
                                                                     : chosen;
  drop_opened();

  slot_state_changed();
  if (auto* next = button(QWizard::NextButton); next && next->isEnabled()) {
    next->setFocus();
  }
}

void ImportKeyPackageDialog::slot_reset_key_file() {
  key_file_path_.clear();
  drop_opened();
  slot_state_changed();
  key_file_button_->setFocus();
}

void ImportKeyPackageDialog::slot_key_item_changed() {
  // Every cell set while filling announces itself; counted once at the end.
  if (filling_) return;
  slot_state_changed();
}

void ImportKeyPackageDialog::slot_state_changed() {
  const auto current = choice();

  ShowPath(package_edit_, current.package.path);
  ShowPath(key_file_edit_, current.key_file.path);

  const auto has_sibling = !sibling_key_file().isEmpty();
  if (!key_file_path_.isEmpty()) {
    key_file_note_->setText(tr("Chosen by you."));
  } else if (has_sibling) {
    key_file_note_->setText(tr("Found beside the package."));
  } else if (!package_path_.isEmpty()) {
    key_file_note_->setText(tr("No key file was found beside the package."));
  } else {
    key_file_note_->clear();
  }
  key_file_note_->setVisible(!key_file_note_->text().isEmpty());
  key_file_reset_->setVisible(!key_file_path_.isEmpty() && has_sibling);

  // Only what the user caused: a field that is merely empty already says so.
  QString problem;
  for (const auto p :
       KeyPackageImportProblems(KeyPackageImportStep::kFiles, current)) {
    if (p != KeyPackageImportProblem::kNoPackage &&
        p != KeyPackageImportProblem::kNoKeyFile) {
      problem = DescribeKeyPackageImportProblem(p);
      break;
    }
  }
  problem_label_->setText(problem);
  problem_label_->setVisible(!problem.isEmpty());
  package_edit_->setAccessibleDescription(problem);

  review_->SetRows(BuildKeyPackageImportReviewRows(current));

  selection_label_->setText(DescribeKeyPackageImportSelection(
      current.keys_selected, static_cast<int>(found_.size())));

  for (auto* page : pages_) emit page->completeChanged();
}

void ImportKeyPackageDialog::fill_key_table() {
  const auto rows = BuildKeyPackageImportKeyRows(found_);
  const auto muted = MutedTextColor(palette());

  // Not behind a QSignalBlocker: the view learns about rows from the same
  // signals, and would stay empty. The rows are half built until the loop
  // ends, so nothing reads them before then.
  filling_ = true;
  keys_model_->setRowCount(0);
  keys_model_->setRowCount(static_cast<int>(rows.size()));

  for (int r = 0; r < rows.size(); ++r) {
    const auto& row = rows.at(r);
    const std::array cells = {row.name, row.email, row.id, row.status};
    for (int c = 0; c < static_cast<int>(cells.size()); ++c) {
      auto* item = new QStandardItem(cells.at(c));
      item->setToolTip(cells.at(c));
      item->setEditable(false);
      // Nothing new but updates: still offered, in a quieter colour.
      if (row.already_present && c == 3) item->setForeground(muted);
      if (c == 0) {
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        item->setData(found_.at(r).key.fpr, kFprRole);
      }
      keys_model_->setItem(r, c, item);
    }
  }
  filling_ = false;
}

void ImportKeyPackageDialog::drop_opened() {
  if (!opened_ && found_.isEmpty() && key_data_.Empty()) return;

  opened_ = false;
  key_data_ = GFBuffer{};
  found_.clear();
  if (keys_model_ != nullptr) keys_model_->setRowCount(0);
}

void ImportKeyPackageDialog::start_open() {
  const auto current = choice();
  const auto package_path = current.package.path;
  const auto key_path = current.key_file.path;
  const auto pin = entry_->Secret();
  const auto channel = current_gpg_context_channel_;

  set_busy(true);

  QPointer<ImportKeyPackageDialog> self(this);
  GpgOperaHelper::WaitForOpera(
      this, tr("Opening"), [=](const OperaWaitingHd& op_hd) {
        // Decrypting is deliberately slow (Argon2id), so it does not run on
        // this thread.
        Thread::Task::TaskCallback cb = [=](int,
                                            const DataObjectPtr& data_object) {
          op_hd();
          if (self == nullptr) return;

          QString error = u"Read Key Package Failed"_s;
          GFBuffer key_data;
          if (data_object != nullptr &&
              data_object->Check<QString, GFBuffer>()) {
            error = ExtractParams<QString>(data_object, 0);
            key_data = ExtractParams<GFBuffer>(data_object, 1);
          }
          self->finish_open(error, key_data);
        };

        auto* task = new Thread::Task{
            [=](const DataObjectPtr& data_object) -> int {
              auto [err, key_data] =
                  KeyPackageOperator::GetInstance(channel).OpenKeyPackage(
                      package_path, key_path, pin);
              data_object->Swap({err, key_data});
              return 0;
            },
            "open_key_package", TransferParams(), cb};

        Thread::TaskRunnerGetter::GetInstance()
            .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
            ->PostTask(task);
      });
}

void ImportKeyPackageDialog::finish_open(const QString& error,
                                         const GFBuffer& key_data) {
  set_busy(false);

  if (!error.isEmpty()) {
    const auto failure = DescribeKeyPackageImportError(error);
    if (failure.retry_pin) {
      // Answered where the PIN was typed, with the files still chosen.
      entry_->Clear();
      entry_->SetErrorText(failure.message);
      entry_->FocusFirstField();
      return;
    }
    show_failure(error);
    return;
  }

  const auto keys = KeyPackageOperator::ListKeyPackageKeys(key_data);
  if (keys.isEmpty()) {
    show_failure(u"No Selected Key Found in the Key Package"_s);
    return;
  }

  // What this key database already holds of each, so the table can say what
  // importing it would change.
  auto& repository =
      GpgKeyRepository::GetInstance(current_gpg_context_channel_);
  found_.clear();
  for (const auto& key : keys) {
    KeyPackageImportKey found;
    found.key = key;
    const auto local = repository.GetKeyPtr(key.fpr);
    found.present = local != nullptr && local->IsGood();
    found.present_private = found.present && local->IsPrivateKey();
    found_.append(found);
  }

  key_data_ = key_data;
  opened_ = true;
  fill_key_table();
  slot_state_changed();
  next();
}

void ImportKeyPackageDialog::start_import() {
  const auto key_data = key_data_;
  const auto fprs = selected_fprs();
  const auto channel = current_gpg_context_channel_;

  set_busy(true);

  QPointer<ImportKeyPackageDialog> self(this);
  GpgOperaHelper::WaitForOpera(
      this, tr("Importing"), [=](const OperaWaitingHd& op_hd) {
        Thread::Task::TaskCallback cb = [=](int,
                                            const DataObjectPtr& data_object) {
          op_hd();
          if (self == nullptr) return;

          QString error;
          QSharedPointer<GpgImportInformation> info;
          if (data_object != nullptr &&
              data_object
                  ->Check<QString, QSharedPointer<GpgImportInformation>>()) {
            error = ExtractParams<QString>(data_object, 0);
            info = ExtractParams<QSharedPointer<GpgImportInformation>>(
                data_object, 1);
          }
          self->finish_import(error, info);
        };

        auto* task = new Thread::Task{
            [=](const DataObjectPtr& data_object) -> int {
              auto [err, info] =
                  KeyPackageOperator::GetInstance(channel).ImportKeyPackageKeys(
                      key_data, fprs);
              data_object->Swap({err, info});
              return 0;
            },
            "import_key_package", TransferParams(), cb};

        Thread::TaskRunnerGetter::GetInstance()
            .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
            ->PostTask(task);
      });
}

void ImportKeyPackageDialog::finish_import(
    const QString& error, const QSharedPointer<GpgImportInformation>& info) {
  set_busy(false);

  if (info == nullptr) {
    show_failure(error);
    return;
  }

  imported_ = true;
  entry_->Clear();
  drop_opened();

  // Closed before the result is announced, so whatever reports it opens over
  // the window the wizard came from, not over the wizard. accept() only
  // schedules the deletion, so emitting afterwards is safe.
  const auto channel = current_gpg_context_channel_;
  accept();
  emit SignalKeysImported(channel, info);
}

void ImportKeyPackageDialog::show_failure(const QString& error) {
  QMessageBox box(QMessageBox::Critical, tr("Import Failed"),
                  DescribeKeyPackageImportError(error).message, QMessageBox::Ok,
                  this);
  if (!error.isEmpty()) box.setDetailedText(error);
  box.exec();
}

void ImportKeyPackageDialog::set_busy(bool busy) {
  busy_ = busy;
  for (const auto which : {QWizard::BackButton, QWizard::NextButton,
                           QWizard::FinishButton, QWizard::CancelButton}) {
    if (auto* b = button(which)) b->setEnabled(!busy);
  }
  if (!busy) {
    for (auto* page : pages_) emit page->completeChanged();
  }
}

void ImportKeyPackageDialog::done(int result) {
  if (busy_) return;
  GeneralWizard::done(result);
}

}  // namespace GpgFrontend::UI
