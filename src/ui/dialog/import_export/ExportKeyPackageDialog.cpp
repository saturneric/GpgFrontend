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

#include "ExportKeyPackageDialog.h"

#include <QFileDialog>

#include "core/function/KeyPackageOperator.h"
#include "core/utils/IOUtils.h"
#include "ui/dialog/MetaListDialog.h"
#include "ui/dialog/SecretPrompt.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/KeyDatabaseRefresh.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/MetaListPanel.h"
#include "ui/widgets/SecretEntryPanel.h"

namespace GpgFrontend::UI {

namespace {

auto StepIndex(KeyPackageExportStep step) -> int {
  return static_cast<int>(step);
}

/// Rows the key table shows before it scrolls. A longer selection scrolls
/// rather than stretching the wizard, so every step keeps the size of the
/// first.
constexpr int kKeyTableVisibleRows = 5;

/// The end of a long path is the part that names the file.
void ShowPath(QLineEdit* edit, const QString& path) {
  const auto native = QDir::toNativeSeparators(path);
  edit->setText(native);
  edit->setToolTip(native);
  edit->setCursorPosition(static_cast<int>(native.size()));
}

auto WarningLabel(QWidget* parent) -> QLabel* {
  auto* label = new QLabel(parent);
  label->setWordWrap(true);
  SetLabelTextColor(label, WarningColor(parent->palette()));
  label->hide();
  return label;
}

}  // namespace

ExportKeyPackageDialog::ExportKeyPackageDialog(int channel,
                                               GpgAbstractKeyPtrList keys,
                                               Preset preset, QWidget* parent)
    : GeneralWizard(typeid(ExportKeyPackageDialog).name(),
                    tr("Export key package"), parent),
      current_gpg_context_channel_(channel),
      keys_(std::move(keys)) {
  // A reload of the key databases replaces the channel this works on; find
  // the database again, or say it is gone.
  KeepBoundToKeyDatabase(
      this, [this]() { return current_gpg_context_channel_; },
      [this](int channel) { current_gpg_context_channel_ = channel; });

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Export As Key Package"));

  for (const auto& key : keys_) {
    if (key == nullptr) continue;
    entries_.append(
        {key->Name(), key->Email(), key->ID(), key->IsPrivateKey()});
    if (key->IsPrivateKey()) with_private_++;
  }

  build_contents_page(preset);
  build_location_page();
  build_protection_page();

  connect(only_private_box_, &QCheckBox::toggled, this,
          &ExportKeyPackageDialog::slot_state_changed);
  connect(include_private_box_, &QCheckBox::toggled, this,
          &ExportKeyPackageDialog::slot_state_changed);
  connect(entry_, &SecretEntryPanel::SignalStateChanged, this,
          &ExportKeyPackageDialog::slot_state_changed);
  connect(key_file_button_, &QPushButton::clicked, this,
          &ExportKeyPackageDialog::slot_choose_key_file);
  connect(key_file_reset_, &QPushButton::clicked, this,
          &ExportKeyPackageDialog::slot_reset_key_file);
  connect(name_edit_, &QLineEdit::textChanged, this,
          &ExportKeyPackageDialog::slot_state_changed);
  connect(location_button_, &QPushButton::clicked, this,
          &ExportKeyPackageDialog::slot_choose_location);
  connect(this, &QWizard::currentIdChanged, this,
          &ExportKeyPackageDialog::slot_page_changed);
  connect(remove_passphrase_box_, &QCheckBox::toggled, this,
          &ExportKeyPackageDialog::slot_state_changed);
  connect(gfpipe_box_, &QCheckBox::toggled, this,
          &ExportKeyPackageDialog::slot_gfpipe_toggled);
  connect(protection_group_, &QButtonGroup::idToggled, this,
          [this](int, bool checked) {
            if (checked) slot_protection_changed();
          });
  connect(legacy_box_, &QCheckBox::toggled, this,
          &ExportKeyPackageDialog::slot_protection_changed);

  set_location(GetDefaultUserFilePath());
}

void ExportKeyPackageDialog::slot_gfpipe_toggled(bool on) {
  if (on) {
    name_before_gfpipe_ = name_edit_->text();

    // gfpipe cannot read a PIN package or the older format. A plain key is
    // saved beside it, so the folder works as soon as it is written.
    legacy_box_->setChecked(false);
    if (protection_group_->checkedId() ==
        static_cast<int>(KeyPackageProtection::kPIN_FILE)) {
      protection_group_->button(static_cast<int>(KeyPackageProtection::kPLAIN))
          ->setChecked(true);
    }
    // The package gfpipe reads without being told which.
    name_edit_->setText(u"default"_s);
  } else if (!name_before_gfpipe_.isEmpty()) {
    name_edit_->setText(name_before_gfpipe_);
  }
  slot_protection_changed();
}

void ExportKeyPackageDialog::slot_protection_changed() { slot_state_changed(); }

auto ExportKeyPackageDialog::add_page(KeyPackageExportStep step)
    -> GeneralWizardPage* {
  auto* page = AddStep(StepIndex(step), kKeyPackageExportStepCount,
                       KeyPackageExportStepTitle(step));
  pages_.at(StepIndex(step)) = page;
  return page;
}

void ExportKeyPackageDialog::build_contents_page(Preset preset) {
  auto* page = add_page(KeyPackageExportStep::kContents);
  auto* layout = PageLayout(page);
  AddLead(layout,
          tr("Check which keys go into the package, and whether their private "
             "keys go with them."),
          page);

  contents_summary_ = new QLabel(page);
  contents_summary_->setWordWrap(true);
  layout->addWidget(contents_summary_);

  // Two independent questions. The first narrows the set of keys; the second
  // decides what of each key is written. Neither is offered where it could
  // only empty the set or change nothing.
  KeyPackageExportChoice initial;
  initial.selected = static_cast<int>(entries_.size());
  initial.with_private = with_private_;

  only_private_box_ =
      new QCheckBox(tr("Only export keys with a private key"), page);
  only_private_box_->setEnabled(CanFilterToPrivateKeys(initial));
  only_private_box_->setChecked(preset.only_with_private &&
                                only_private_box_->isEnabled());
  layout->addWidget(only_private_box_);

  include_private_box_ = new QCheckBox(tr("Include private keys"), page);
  include_private_box_->setEnabled(CanIncludePrivateKeys(initial));
  include_private_box_->setChecked(preset.include_private &&
                                   include_private_box_->isEnabled());
  layout->addWidget(include_private_box_);

  // Beside the box that causes it, and only while it is ticked: a warning that
  // is always on screen is a warning nobody reads.
  private_warning_label_ = WarningLabel(page);
  private_warning_label_->setText(
      tr("Private keys let anyone who has this package, its key file and the "
         "PIN decrypt your messages and sign as you."));
  include_private_box_->setAccessibleDescription(
      private_warning_label_->text());
  layout->addWidget(private_warning_label_);

  // Only meaningful once private keys go in; what it costs is said beside it,
  // and only while it is ticked.
  remove_passphrase_box_ = new QCheckBox(
      tr("Remove the passphrase from the private keys in the package"), page);
  layout->addWidget(remove_passphrase_box_);
  remove_passphrase_warning_ = WarningLabel(page);
  remove_passphrase_warning_->setText(
      tr("Needed for tools such as gfpipe that sign without asking. The "
         "package's own protection is then all that guards the keys. The keys "
         "in GpgFrontend keep their passphrase."));
  remove_passphrase_box_->setAccessibleDescription(
      remove_passphrase_warning_->text());
  layout->addWidget(remove_passphrase_warning_);

  limits_label_ =
      CreateDetailLabel(DescribeKeyPackageContentLimits(initial), page);
  limits_label_->setWordWrap(true);
  limits_label_->setVisible(!limits_label_->text().isEmpty());
  layout->addWidget(limits_label_);

  // A table rather than a list: two keys can share a name, and the email and
  // key ID beside it are what tell them apart before anything is written.
  keys_model_ = new QStandardItemModel(this);
  keys_model_->setHorizontalHeaderLabels(KeyPackageKeyColumns());

  keys_table_ = new QTableView(page);
  keys_table_->setModel(keys_model_);
  keys_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  keys_table_->setSelectionMode(QAbstractItemView::NoSelection);
  keys_table_->setFocusPolicy(Qt::TabFocus);
  keys_table_->setShowGrid(false);
  keys_table_->setWordWrap(false);
  keys_table_->setTextElideMode(Qt::ElideRight);
  keys_table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  keys_table_->verticalHeader()->hide();
  keys_table_->horizontalHeader()->setHighlightSections(false);
  keys_table_->horizontalHeader()->setStretchLastSection(true);
  keys_table_->setAccessibleName(tr("Keys to export"));
  refresh_key_table(initial);

  // Name and email share what is left; the ID and the outcome take what they
  // need, so neither is ever elided.
  auto* header = keys_table_->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setSectionResizeMode(1, QHeaderView::Stretch);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(3, QHeaderView::ResizeToContents);

  const auto visible_rows =
      std::clamp(static_cast<int>(entries_.size()), 1, kKeyTableVisibleRows);
  keys_table_->setFixedHeight(
      header->sizeHint().height() +
      (visible_rows * keys_table_->verticalHeader()->defaultSectionSize()) +
      (2 * keys_table_->frameWidth()));

  layout->addSpacing(4);
  layout->addWidget(keys_table_);

  layout->addStretch(1);

  page->complete = [this]() {
    return CanAdvanceKeyPackageExport(KeyPackageExportStep::kContents,
                                      choice());
  };
}

void ExportKeyPackageDialog::build_location_page() {
  auto* page = add_page(KeyPackageExportStep::kLocation);
  auto* layout = PageLayout(page);
  AddLead(layout, tr("Name the package and choose where to save it."), page);

  // gfpipe reads the package and key that lie beside it, so the folder
  // written here is ready to use as it is.
  bundled_gfpipe_ = KeyPackageOperator::BundledGfPipe();
  gfpipe_box_ = new QCheckBox(tr("Include gfpipe, ready to use"), page);
  gfpipe_box_->setObjectName(u"gfpipeBox"_s);
  gfpipe_box_->setEnabled(!bundled_gfpipe_.isEmpty());

  auto about = tr(
      "gfpipe is a small command line tool. It lets git and scripts sign and "
      "encrypt with these keys, like gpg.");
  if (bundled_gfpipe_.isEmpty()) {
    about += u" "_s + tr("This installation does not include it.");
  }
  gfpipe_about_ = CreateDetailLabel(about, page);
  gfpipe_note_ = CreateDetailLabel(
      tr("gfpipe, the package and its key are saved together in this folder. "
         "Keep the folder private: anyone who can open it can use your "
         "keys."),
      page);
  // Under the box's text, not under its indicator.
  auto* gfpipe_details = new QVBoxLayout();
  gfpipe_details->setContentsMargins(
      style()->pixelMetric(QStyle::PM_IndicatorWidth) +
          style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing),
      0, 0, 0);
  gfpipe_details->addWidget(gfpipe_about_);
  gfpipe_details->addWidget(gfpipe_note_);

  auto* grid = new QGridLayout();
  grid->setHorizontalSpacing(6);
  grid->setVerticalSpacing(6);
  grid->setColumnStretch(1, 1);

  auto* name_label = new QLabel(tr("Package name"), page);
  name_edit_ = new QLineEdit(
      KeyPackageOperator::GetInstance(current_gpg_context_channel_)
          .GenerateKeyPackageName(),
      page);
  name_edit_->setMaxLength(200);
  // Named for the manual's screenshots, which replace the random name.
  name_edit_->setObjectName(u"packageNameEdit"_s);
  name_label->setBuddy(name_edit_);

  generate_button_ = new QPushButton(tr("Generate name"), page);
  generate_button_->setAutoDefault(false);
  generate_button_->setAccessibleName(tr("Generate a package name"));
  connect(generate_button_, &QPushButton::clicked, this,
          &ExportKeyPackageDialog::slot_generate_name);

  grid->addWidget(name_label, 0, 0);
  grid->addWidget(name_edit_, 0, 1);
  grid->addWidget(generate_button_, 0, 2);

  // Each path is a read-only field with Browse beside it: the platform's own
  // shape for "a file, and the button that picks it".
  const auto add_path_row = [&](int row, const QString& caption,
                                const QString& accessible, QLineEdit*& edit,
                                QPushButton*& button) -> QLabel* {
    auto* label = new QLabel(caption, page);
    edit = new QLineEdit(page);
    edit->setReadOnly(true);
    edit->setPlaceholderText(tr("No location chosen yet"));
    label->setBuddy(edit);
    button = new QPushButton(tr("Browse..."), page);
    button->setAutoDefault(false);
    button->setAccessibleName(accessible);
    grid->addWidget(label, row, 0);
    grid->addWidget(edit, row, 1);
    grid->addWidget(button, row, 2);
    return label;
  };
  add_path_row(1, tr("Save to"), tr("Choose where to save the key package"),
               location_edit_, location_button_);
  key_file_label_ =
      add_path_row(2, tr("Key file"), tr("Choose where to save the key file"),
                   key_file_edit_, key_file_button_);

  // Whether the key file follows the package, and the way back when it does
  // not.
  key_file_note_ = CreateDetailLabel({}, page);
  key_file_note_->setWordWrap(true);
  key_file_reset_ = new QPushButton(tr("Keep next to the package"), page);
  key_file_reset_->setFlat(true);
  key_file_reset_->setAutoDefault(false);
  key_file_reset_->setCursor(Qt::PointingHandCursor);
  // In the link colour, like the PIN generator, so a flat button still reads
  // as something to click.
  auto reset_palette = key_file_reset_->palette();
  reset_palette.setColor(QPalette::ButtonText,
                         reset_palette.color(QPalette::Link));
  key_file_reset_->setPalette(reset_palette);
  auto* note_row = new QHBoxLayout();
  note_row->setSpacing(6);
  note_row->addWidget(key_file_note_, 1);
  note_row->addWidget(key_file_reset_);
  grid->addLayout(note_row, 3, 1, 1, 2);

  layout->addLayout(grid);

  problem_label_ = WarningLabel(page);
  layout->addWidget(problem_label_);

  // Below the fields it changes: ticking it names the package "default".
  layout->addSpacing(8);
  layout->addWidget(gfpipe_box_);
  layout->addLayout(gfpipe_details);

  // How the files are sealed, as one quiet line under a hairline: true and
  // worth checking, but not something to fill in.
  auto* rule = new QFrame(page);
  rule->setFrameShape(QFrame::HLine);
  rule->setFrameShadow(QFrame::Sunken);
  layout->addSpacing(4);
  layout->addWidget(rule);

  auto* lock = new QLabel(page);
  lock->setPixmap(QIcon(u":/icons/lock.png"_s).pixmap(16, 16));
  lock->setAlignment(Qt::AlignTop);

  mechanism_label_ = CreateDetailLabel({}, page);
  mechanism_label_->setWordWrap(true);

  auto* mechanism_row = new QHBoxLayout();
  mechanism_row->setSpacing(8);
  mechanism_row->addWidget(lock, 0, Qt::AlignTop);
  mechanism_row->addWidget(mechanism_label_, 1);
  layout->addLayout(mechanism_row);

  layout->addStretch(1);

  page->complete = [this]() {
    return CanAdvanceKeyPackageExport(KeyPackageExportStep::kLocation,
                                      choice());
  };
}

void ExportKeyPackageDialog::build_protection_page() {
  auto* page = add_page(KeyPackageExportStep::kProtection);
  auto* layout = PageLayout(page);

  // Said before the fields rather than under them: it is the one thing to know
  // before choosing, and under the fields it read as small print.
  AddLead(layout,
          tr("Choose how the package is protected. A PIN cannot be "
             "recovered."),
          page);

  // One option per protection, each with what it means under it.
  protection_group_ = new QButtonGroup(this);
  const auto add_option = [&](KeyPackageProtection protection,
                              const QString& text) {
    auto* radio = new QRadioButton(text, page);
    protection_group_->addButton(radio, static_cast<int>(protection));
    auto* detail =
        CreateDetailLabel(DescribeKeyPackageProtectionOption(protection), page);
    detail->setWordWrap(true);
    detail->setContentsMargins(24, 0, 0, 0);
    radio->setAccessibleDescription(detail->text());
    layout->addWidget(radio);
    layout->addWidget(detail);
  };
  add_option(KeyPackageProtection::kPIN_FILE,
             tr("A PIN and a separate key file (recommended for transfer)"));
  add_option(KeyPackageProtection::kPLAIN, tr("A plain key"));
  protection_group_->button(static_cast<int>(KeyPackageProtection::kPIN_FILE))
      ->setChecked(true);

  legacy_box_ = new QCheckBox(
      tr("Write the version 2 format, for older GpgFrontend versions"), page);
  layout->addWidget(legacy_box_);

  protection_problem_label_ = WarningLabel(page);
  layout->addWidget(protection_problem_label_);

  auto texts = DefaultSecretPromptTexts(SecretPromptSubject::kKeyPackage,
                                        SecretPromptMode::kSET);
  // The lead above says it; the row under the fields is kept for what goes
  // wrong and for the generator's notice.
  texts.hint = {};

  SecretEntryPanel::Config config;
  config.ask_current = false;
  config.ask_new = true;
  config.offer_generation = true;
  config.defer_validation = true;
  config.compact = true;
  config.texts = texts;
  entry_ = new SecretEntryPanel(config, page);
  entry_->setAccessibleName(tr("PIN to protect the key file"));
  layout->addWidget(entry_);

  // What is about to happen, right above the button with no undo.
  review_ = new MetaListPanel(page);
  layout->addWidget(review_);

  layout->addStretch(1);

  page->complete = [this]() {
    return !exporting_ && CanAdvanceKeyPackageExport(
                              KeyPackageExportStep::kProtection, choice());
  };

  // QWizard asks this page before it closes. The files are written behind a
  // waiting dialog first, and the wizard only closes once they exist, so a
  // failure leaves every choice where it was for another try.
  page->validate = [this]() {
    if (exported_) return true;
    if (!exporting_ && CanAdvanceKeyPackageExport(
                           KeyPackageExportStep::kProtection, choice())) {
      start_export();
    }
    return false;
  };
}

void ExportKeyPackageDialog::slot_page_changed(int id) {
  // Queued: QWizard settles focus on its own buttons after announcing the new
  // page, and this has to land after that.
  QTimer::singleShot(0, this, [this, id]() {
    if (currentId() != id) return;

    switch (static_cast<KeyPackageExportStep>(id)) {
      case KeyPackageExportStep::kContents:
        if (only_private_box_->isEnabled()) {
          only_private_box_->setFocus();
        } else if (include_private_box_->isEnabled()) {
          include_private_box_->setFocus();
        }
        break;
      case KeyPackageExportStep::kLocation:
        name_edit_->setFocus();
        break;
      case KeyPackageExportStep::kProtection:
        if (!entry_->Acceptable()) entry_->FocusFirstField();
        break;
    }
  });
}

void ExportKeyPackageDialog::refresh_key_table(
    const KeyPackageExportChoice& choice) {
  const auto rows = BuildKeyPackageKeyRows(entries_, choice);
  // The muted text colour rather than the disabled one, which all but
  // disappears in a dark palette.
  const auto dimmed = MutedTextColor(palette());

  keys_model_->setRowCount(static_cast<int>(rows.size()));
  for (int r = 0; r < rows.size(); ++r) {
    const auto& row = rows.at(r);
    const std::array cells = {row.name, row.email, row.id, row.exported};
    for (int c = 0; c < static_cast<int>(cells.size()); ++c) {
      auto* item = new QStandardItem(cells.at(c));
      item->setToolTip(cells.at(c));
      // Left out, but still listed: greyed rather than removed.
      if (row.left_out) item->setForeground(dimmed);
      keys_model_->setItem(r, c, item);
    }
  }
}

auto ExportKeyPackageDialog::choice() const -> KeyPackageExportChoice {
  KeyPackageExportChoice choice;
  choice.selected = static_cast<int>(entries_.size());
  choice.with_private = with_private_;
  choice.only_with_private = only_private_box_->isChecked();
  choice.include_private = include_private_box_->isChecked();
  choice.pin_acceptable = entry_->Acceptable();
  choice.name = name_edit_->text();
  choice.location = location_;
  choice.location_writable = location_writable_;
  // Beside the package, under its name, until the user puts it elsewhere.
  choice.key_file_path = key_file_path_.isEmpty()
                             ? DefaultKeyPackageKeyFilePath(choice)
                             : key_file_path_;
  choice.legacy_format = legacy_box_->isChecked();
  choice.protection =
      static_cast<KeyPackageProtection>(protection_group_->checkedId());
  choice.remove_passphrase = remove_passphrase_box_->isChecked();
  choice.for_gfpipe = gfpipe_box_->isChecked();
  return choice;
}

void ExportKeyPackageDialog::set_location(const QString& location) {
  location_ = location;
  const QFileInfo info(location_);
  location_writable_ = info.isDir() && info.isWritable();
  slot_state_changed();
}

void ExportKeyPackageDialog::slot_generate_name() {
  name_edit_->setText(
      KeyPackageOperator::GetInstance(current_gpg_context_channel_)
          .GenerateKeyPackageName());
  name_edit_->setFocus();
}

void ExportKeyPackageDialog::slot_choose_key_file() {
  const auto current = choice();
  const auto suggestion =
      current.key_file_path.isEmpty()
          ? QDir(GetDefaultUserFilePath())
                .filePath(u"KeyPackage"_s + kKeyPackageKeyFileExtension)
          : current.key_file_path;

  const auto chosen = QFileDialog::getSaveFileName(
      this, tr("Save Key File"), suggestion,
      tr("Key File") + " (*.key);;" + tr("All Files") + " (*)");
  if (chosen.isEmpty()) return;

  const auto path =
      WithKeyPackageExtension(chosen, kKeyPackageKeyFileExtension);
  // The picker asked about replacing exactly the path it returned, and no
  // other.
  if (path == chosen) confirmed_paths_.insert(path);

  // Picking the place it would have gone anyway keeps it following the
  // package.
  key_file_path_ =
      QDir::cleanPath(path) ==
              QDir::cleanPath(DefaultKeyPackageKeyFilePath(current))
          ? QString{}
          : path;

  slot_state_changed();
  if (auto* next = button(QWizard::NextButton); next && next->isEnabled()) {
    next->setFocus();
  }
}

void ExportKeyPackageDialog::slot_reset_key_file() {
  key_file_path_.clear();
  slot_state_changed();
  key_file_button_->setFocus();
}

void ExportKeyPackageDialog::slot_choose_location() {
  const auto folder =
      location_.isEmpty() ? GetDefaultUserFilePath() : location_;
  const auto name =
      IsValidKeyPackageName(name_edit_->text())
          ? name_edit_->text()
          : KeyPackageOperator::GetInstance(current_gpg_context_channel_)
                .GenerateKeyPackageName();

  const auto chosen = QFileDialog::getSaveFileName(
      this, tr("Export Key Package"),
      QDir(folder).filePath(
          WithKeyPackageExtension(name, kKeyPackageExtension)),
      tr("Key Package") + " (*.gfpack);;" + tr("All Files") + " (*)");
  if (chosen.isEmpty()) return;

  const auto path = WithKeyPackageExtension(chosen, kKeyPackageExtension);
  if (path == chosen) confirmed_paths_.insert(path);

  // One file, held as a folder and a name, so editing the name afterwards
  // keeps the folder that was picked.
  const QFileInfo info(path);
  auto file_name = info.fileName();
  file_name.chop(static_cast<int>(qstrlen(kKeyPackageExtension)));
  name_edit_->setText(file_name);
  set_location(info.absolutePath());

  if (auto* next = button(QWizard::NextButton); next && next->isEnabled()) {
    next->setFocus();
  }
}

void ExportKeyPackageDialog::slot_state_changed() {
  const auto current = choice();

  contents_summary_->setText(DescribeKeyPackageContents(current));
  private_warning_label_->setVisible(KeyPackageIncludesPrivate(current));
  // Version 2 has no way to say its keys are unprotected, so it never writes
  // them that way.
  remove_passphrase_box_->setEnabled(KeyPackageIncludesPrivate(current) &&
                                     !current.legacy_format);
  remove_passphrase_warning_->setVisible(KeyPackageRemovesPassphrase(current));
  refresh_key_table(current);

  ShowPath(location_edit_, location_);
  ShowPath(key_file_edit_, current.key_file_path);

  // gfpipe reads "default" unless told otherwise; the folder is the user's.
  name_edit_->setEnabled(!current.for_gfpipe);
  generate_button_->setEnabled(!current.for_gfpipe);
  gfpipe_note_->setVisible(current.for_gfpipe);

  const auto uses_key_file = KeyPackageUsesKeyFile(current);
  const auto follows = key_file_path_.isEmpty();
  key_file_note_->setText(follows ? tr("Saved beside the package.")
                                  : tr("Saved in a place of its own."));
  for (auto* widget : std::initializer_list<QWidget*>{
           key_file_label_, key_file_edit_, key_file_button_, key_file_note_}) {
    widget->setVisible(uses_key_file);
  }
  key_file_reset_->setVisible(uses_key_file && !follows);
  mechanism_label_->setText(DescribeKeyPackageProtection(current));

  // A PIN package is the only kind the older format and gfpipe disagree on.
  const auto legacy = current.legacy_format;
  legacy_box_->setEnabled(!current.for_gfpipe);
  protection_group_->button(static_cast<int>(KeyPackageProtection::kPLAIN))
      ->setEnabled(!legacy);
  protection_group_->button(static_cast<int>(KeyPackageProtection::kPIN_FILE))
      ->setEnabled(!current.for_gfpipe);
  entry_->setVisible(KeyPackageEffectiveProtection(current) ==
                     KeyPackageProtection::kPIN_FILE);

  QString protection_problem;
  for (const auto p :
       KeyPackageExportProblems(KeyPackageExportStep::kProtection, current)) {
    if (p == KeyPackageExportProblem::kGfPipeNeedsNoPin) {
      protection_problem = DescribeKeyPackageExportProblem(p);
      break;
    }
  }
  protection_problem_label_->setText(protection_problem);
  protection_problem_label_->setVisible(!protection_problem.isEmpty());

  review_->SetRows(BuildKeyPackageReviewRows(current));

  // Only what the user can fix on the location step, and only once there is
  // something to fix: an empty location already says so in its own field.
  QString problem;
  for (const auto p :
       KeyPackageExportProblems(KeyPackageExportStep::kLocation, current)) {
    if (p == KeyPackageExportProblem::kInvalidName ||
        p == KeyPackageExportProblem::kLocationNotWritable ||
        p == KeyPackageExportProblem::kSameFile) {
      problem = DescribeKeyPackageExportProblem(p);
      break;
    }
  }
  problem_label_->setText(problem);
  problem_label_->setVisible(!problem.isEmpty());
  name_edit_->setAccessibleDescription(problem);

  for (auto* page : pages_) emit page->completeChanged();
}

auto ExportKeyPackageDialog::confirm_replacements(const QStringList& paths)
    -> bool {
  for (const auto& path : paths) {
    if (!QFileInfo::exists(path) || confirmed_paths_.contains(path)) continue;

    const auto answer = QMessageBox::question(
        this, tr("Replace File?"),
        tr("%1 already exists. Do you want to replace it?")
            .arg(QDir::toNativeSeparators(path)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return false;

    confirmed_paths_.insert(path);
  }
  return true;
}

void ExportKeyPackageDialog::start_export() {
  const auto current = choice();
  const auto package_path = KeyPackagePath(current);
  const auto key_path =
      KeyPackageUsesKeyFile(current) ? current.key_file_path : QString{};

  QStringList targets = {package_path};
  if (!key_path.isEmpty()) targets.append(key_path);
  const auto gfpipe = current.for_gfpipe ? bundled_gfpipe_ : QString{};
  if (!gfpipe.isEmpty()) {
    const QDir folder = QFileInfo(package_path).absoluteDir();
    targets.append(folder.filePath(QFileInfo(gfpipe).fileName()));
    if (KeyPackageEffectiveProtection(current) ==
        KeyPackageProtection::kPLAIN) {
      targets.append(folder.filePath(
          QFileInfo(package_path).completeBaseName() + u".gfkey"_s));
    }
  }
  if (!confirm_replacements(targets)) return;

  // Filtered into a copy: the list the wizard was handed stays whole, so a
  // retry after a failure, or a change of mind on the first step, starts from
  // every key again.
  GpgAbstractKeyPtrList keys;
  for (const auto& key : keys_) {
    if (key != nullptr && KeyPackageKeepsKey(key->IsPrivateKey(), current)) {
      keys.push_back(key);
    }
  }
  const auto secret = KeyPackageIncludesPrivate(current);
  const auto pin = entry_->Secret();
  const auto channel = current_gpg_context_channel_;

  set_busy(true);

  QPointer<ExportKeyPackageDialog> self(this);

  if (current.legacy_format) {
    GpgOperaHelper::WaitForOpera(
        this, tr("Exporting"), [=](const OperaWaitingHd& op_hd) {
          KeyPackageOperator::GetInstance(channel).GenerateKeyPackage(
              package_path, key_path, keys, pin, secret,
              [=](int err, const DataObjectPtr& data_object) {
                op_hd();
                if (self == nullptr) return;

                QString error;
                if (err < 0 && data_object != nullptr &&
                    data_object->Check<QString>()) {
                  error = ExtractParams<QString>(data_object, 0);
                }
                self->finish_export(err >= 0, error, {}, package_path,
                                    key_path);
              });
        });
    return;
  }

  KeyPackageV3Request request;
  request.package_path = package_path;
  request.key_path = key_path;
  request.keys = keys;
  request.secret = secret;
  request.remove_passphrase = KeyPackageRemovesPassphrase(current);
  request.protection = KeyPackageEffectiveProtection(current);
  if (request.protection == KeyPackageProtection::kPIN_FILE) request.pin = pin;
  request.gfpipe_program = gfpipe;

  GpgOperaHelper::WaitForOpera(
      this, tr("Exporting"), [=](const OperaWaitingHd& op_hd) {
        KeyPackageOperator::GetInstance(channel).GenerateKeyPackageV3(
            request, [=](const KeyPackageV3Written& written) {
              op_hd();
              if (self == nullptr) return;
              self->finish_export(written.error.isEmpty(), written.error,
                                  written.plain_key, package_path, key_path,
                                  written.gfpipe_path, written.key_text_path);
            });
      });
}

void ExportKeyPackageDialog::finish_export(bool success, const QString& error,
                                           const GFBuffer& plain_key,
                                           const QString& package_path,
                                           const QString& key_path,
                                           const QString& gfpipe_path,
                                           const QString& key_text_path) {
  set_busy(false);

  if (!success) {
    QMessageBox box(QMessageBox::Critical, tr("Export Failed"),
                    tr("The key package could not be written."),
                    QMessageBox::Ok, this);
    box.setInformativeText(
        tr("Check that both folders still exist and can be written to, then "
           "try again. Your choices are kept."));
    if (!error.isEmpty()) box.setDetailedText(error);
    box.exec();
    return;
  }

  exported_ = true;
  entry_->Clear();

  // The wizard closes first, so the result is not read over a form that
  // looks as if it were still waiting for input. The dialog belongs to the
  // window the wizard was opened from, and is shown once the wizard is gone:
  // it deletes itself on close.
  const QPointer<QWidget> owner = parentWidget();
  const auto title = tr("Key Package Exported");
  const auto with_gfpipe = !gfpipe_path.isEmpty();
  // Saved beside gfpipe, the key needs no copying or saving here.
  const auto shown_key = key_text_path.isEmpty() ? plain_key : GFBuffer{};
  const auto subtitle =
      with_gfpipe ? tr("gfpipe is ready to use in this folder.")
      : key_path.isEmpty()
          ? tr("The key package has been saved.")
          : tr("The key package and its key file have been saved.");
  const auto git_command = u"git config --global gpg.program \"%1\""_s.arg(
      QDir::toNativeSeparators(gfpipe_path));
  const auto note =
      with_gfpipe
          ? tr("Run gfpipe from this folder as you would run gpg. To sign "
               "with git: %1. Anyone who can read this folder has the keys.")
                .arg(git_command)
      : !plain_key.Empty()
          ? tr("This key opens the package and is not shown again. Copy it or "
               "save it now, for example into the GFPIPE_KEY variable or a CI "
               "secret. Anyone with the key and the package has the keys.")
          : tr("Keep the key file and the PIN apart from the package, and "
               "delete both files once the transfer is done.");
  const auto done_text = tr("Done");
  const auto folder_text = tr("Show in Folder");
  const auto copy_text = tr("Copy Key");
  const auto save_text = tr("Save Key...");
  const auto key_caption = tr("Key");
  const auto save_title = tr("Save Key");

  accept();

  QTimer::singleShot(0, qApp, [=]() {
    // Shown until the user is done with it: copying or saving the key comes
    // back here, since the key is not shown anywhere else.
    for (;;) {
      // Named in rows, the same way the summary named them a moment ago: the
      // files are now facts rather than intentions.
      MetaListDialog done(title, subtitle, owner);
      auto rows = BuildKeyPackageExportDoneRows(package_path, key_path,
                                                gfpipe_path, key_text_path);
      if (!shown_key.Empty()) {
        rows.append(
            {.caption = key_caption, .value = shown_key.ConvertToQString()});
      }
      done.AddSection({}, rows);
      done.AddNote(note, !plain_key.Empty());
      done.AddButton(done_text, QDialogButtonBox::AcceptRole);
      const auto show_folder =
          done.AddButton(folder_text, QDialogButtonBox::ActionRole);
      const auto copy_key =
          shown_key.Empty()
              ? -1
              : done.AddButton(copy_text, QDialogButtonBox::ActionRole);
      const auto save_key =
          shown_key.Empty()
              ? -1
              : done.AddButton(save_text, QDialogButtonBox::ActionRole);
      done.exec();

      const auto picked = done.Choice();
      if (picked == show_folder) {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QFileInfo(package_path).absolutePath()));
      } else if (picked == copy_key && copy_key >= 0) {
        QGuiApplication::clipboard()->setText(plain_key.ConvertToQString());
        continue;
      } else if (picked == save_key && save_key >= 0) {
        const auto path = QFileDialog::getSaveFileName(
            owner, save_title,
            QDir(QFileInfo(package_path).absolutePath())
                .filePath(QFileInfo(package_path).completeBaseName() +
                          u".gfkey"_s));
        if (!path.isEmpty()) WriteFileGFBuffer(path, plain_key);
        continue;
      }
      break;
    }
  });
}

void ExportKeyPackageDialog::set_busy(bool busy) {
  exporting_ = busy;
  for (const auto which :
       {QWizard::BackButton, QWizard::FinishButton, QWizard::CancelButton}) {
    if (auto* b = button(which)) b->setEnabled(!busy);
  }
  if (!busy) {
    emit pages_.at(StepIndex(KeyPackageExportStep::kProtection))
        ->completeChanged();
  }
}

void ExportKeyPackageDialog::changeEvent(QEvent* event) {
  GeneralWizard::changeEvent(event);
  // choice() reads every page, so wait until the last one is built.
  if (event->type() == QEvent::PaletteChange && entry_ != nullptr) {
    refresh_key_table(choice());
  }
}

void ExportKeyPackageDialog::done(int result) {
  if (exporting_) return;
  GeneralWizard::done(result);
}

}  // namespace GpgFrontend::UI
