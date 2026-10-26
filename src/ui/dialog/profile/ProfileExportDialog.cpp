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

#include "ui/dialog/profile/ProfileExportDialog.h"

#include <QFileDialog>
#include <QStorageInfo>

#include "core/model/SettingsObject.h"
#include "core/profile/ProfilePackage.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "ui/dialog/SecretPrompt.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/ProfileController.h"
#include "ui/function/UIStyle.h"
#include "ui/widgets/MetaListPanel.h"
#include "ui/widgets/SecretEntryPanel.h"

namespace GpgFrontend::UI {

namespace {

auto StepIndex(ProfileExportStep step) -> int { return static_cast<int>(step); }

}  // namespace

ProfileExportDialog::ProfileExportDialog(QString display_name,
                                         const ProfileAccessor& storage,
                                         QWidget* parent)
    : GeneralWizard(typeid(ProfileExportDialog).name(), tr("Export backup"),
                    parent),
      display_name_(std::move(display_name)),
      // Sized as it will be packed: without the Recycle Bin's folders.
      areas_(MeasureProfileAreas(
          storage, RecycledKeyDatabaseDirsForPacking(
                       KeyDatabaseListSO(SettingsObject(kKeyDatabaseListObject))
                           .key_databases,
                       storage.PathOf(ProfileArea::kRoot)))) {
  setWindowTitle(tr("Export Profile"));

  build_contents_page();
  build_protection_page();
  build_save_page();

  connect(destination_button_, &QPushButton::clicked, this,
          &ProfileExportDialog::slot_choose_destination);
  connect(workspace_box_, &QCheckBox::toggled, this, [this](bool checked) {
    include_workspace_ = checked;
    slot_state_changed();
  });
  connect(entry_, &SecretEntryPanel::SignalStateChanged, this,
          &ProfileExportDialog::slot_state_changed);
  connect(this, &QWizard::currentIdChanged, this,
          &ProfileExportDialog::slot_page_changed);

  slot_state_changed();
}

auto ProfileExportDialog::add_page(ProfileExportStep step)
    -> GeneralWizardPage* {
  auto* page = AddStep(StepIndex(step), kProfileExportStepCount,
                       ProfileExportStepTitle(step));
  pages_.at(StepIndex(step)) = page;
  return page;
}

void ProfileExportDialog::build_contents_page() {
  auto* page = add_page(ProfileExportStep::kContents);
  auto* layout = PageLayout(page);
  AddLead(layout, tr("Check what the backup file will contain."), page);

  auto* name = new QLabel(display_name_, page);
  auto name_font = name->font();
  name_font.setBold(true);
  name->setFont(name_font);
  name->setTextFormat(Qt::PlainText);
  layout->addWidget(name);

  contents_summary_ = new QLabel(page);
  contents_summary_->setWordWrap(true);
  layout->addWidget(contents_summary_);

  // The one place the workspace is chosen. It is where the cleartext of things
  // a user meant to encrypt tends to live, so it is offered, never swept in,
  // and an empty one says so instead of offering nothing.
  const auto rows = BuildProfileExportContents(areas_, false);
  workspace_box_ =
      new QCheckBox(DescribeProfileExportWorkspaceOption(rows), page);
  workspace_box_->setEnabled(HasProfileExportWorkspace(rows));
  layout->addWidget(workspace_box_);

  contents_ = new MetaListPanel(page);
  contents_->SetRows(contents_rows());
  layout->addSpacing(4);
  layout->addWidget(contents_);

  layout->addStretch(1);
}

void ProfileExportDialog::build_protection_page() {
  auto* page = add_page(ProfileExportStep::kProtection);
  auto* layout = PageLayout(page);

  // Said before the fields rather than under them: it is the one thing to know
  // before choosing, and under the fields it read as small print.
  AddLead(layout,
          tr("Choose a passphrase to encrypt the backup. It cannot be "
             "recovered, so keep it somewhere safe."),
          page);

  // The same fields, reveal toggle, strength meter and floor the application
  // PIN is chosen with. A generator is offered here and not there because this
  // secret is going to be written down rather than remembered.
  auto texts = DefaultSecretPromptTexts(SecretPromptSubject::kProfilePackage,
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
  layout->addWidget(entry_);

  // How the file is sealed, as one quiet line under a hairline: true and worth
  // checking, but not something to fill in.
  auto* rule = new QFrame(page);
  rule->setFrameShape(QFrame::HLine);
  rule->setFrameShadow(QFrame::Sunken);
  layout->addSpacing(4);
  layout->addWidget(rule);

  auto* lock = new QLabel(page);
  lock->setPixmap(QIcon(u":/icons/lock.png"_s).pixmap(16, 16));
  lock->setAlignment(Qt::AlignTop);

  auto* mechanism = CreateDetailLabel(DescribeExportProtection(), page);
  mechanism->setWordWrap(true);

  auto* mechanism_row = new QHBoxLayout();
  mechanism_row->setSpacing(8);
  mechanism_row->addWidget(lock, 0, Qt::AlignTop);
  mechanism_row->addWidget(mechanism, 1);
  layout->addLayout(mechanism_row);

  layout->addStretch(1);

  // Continue waits for a passphrase the panel accepts. What is still wrong is
  // named once the field is left, so the greyed-out button is never the only
  // thing on screen for long.
  page->complete = [this]() {
    return CanAdvanceProfileExport(ProfileExportStep::kProtection, choice());
  };
}

void ProfileExportDialog::build_save_page() {
  auto* page = add_page(ProfileExportStep::kSave);
  auto* layout = PageLayout(page);
  AddLead(layout, tr("Check the summary, then choose where to save the file."),
          page);

  review_ = new MetaListPanel(page);
  layout->addWidget(review_);

  layout->addSpacing(8);

  // The platform's own shape for "a file, and the button that picks it": a
  // read-only path with Browse beside it. The field shows the whole path, so
  // nothing about where the file lands is left to a second screen.
  auto* save_to = new QLabel(tr("Save to"), page);
  layout->addWidget(save_to);

  destination_edit_ = new QLineEdit(page);
  destination_edit_->setReadOnly(true);
  destination_edit_->setPlaceholderText(tr("No location chosen yet"));
  save_to->setBuddy(destination_edit_);

  // The folder icon inside the field opens the picker too; it is where the eye
  // already is.
  auto* pick = destination_edit_->addAction(
      QIcon::fromTheme(u"folder"_s, QIcon(u":/icons/folder.png"_s)),
      QLineEdit::LeadingPosition);
  pick->setToolTip(tr("Choose where to save the file"));
  connect(pick, &QAction::triggered, this,
          &ProfileExportDialog::slot_choose_destination);

  destination_button_ = new QPushButton(tr("Browse..."), page);
  destination_button_->setAutoDefault(false);

  auto* destination_row = new QHBoxLayout();
  destination_row->setSpacing(6);
  destination_row->addWidget(destination_edit_, 1);
  destination_row->addWidget(destination_button_);
  layout->addLayout(destination_row);

  free_space_label_ = CreateDetailLabel({}, page);
  free_space_label_->hide();
  layout->addWidget(free_space_label_);

  warning_label_ = new QLabel(page);
  warning_label_->setWordWrap(true);
  SetLabelTextColor(warning_label_, WarningColor(palette()));
  warning_label_->hide();
  layout->addWidget(warning_label_);

  layout->addStretch(1);

  page->complete = [this]() {
    return CanAdvanceProfileExport(ProfileExportStep::kSave, choice());
  };
}

void ProfileExportDialog::slot_page_changed(int id) {
  // Queued: QWizard settles focus on its own buttons after announcing the new
  // page, and this has to land after that.
  QTimer::singleShot(0, this, [this, id]() {
    if (currentId() != id) return;

    switch (static_cast<ProfileExportStep>(id)) {
      case ProfileExportStep::kContents:
        if (workspace_box_->isEnabled()) workspace_box_->setFocus();
        break;
      case ProfileExportStep::kProtection:
        entry_->FocusFirstField();
        break;
      case ProfileExportStep::kSave:
        if (destination_.isEmpty()) destination_button_->setFocus();
        break;
    }
  });
}

auto ProfileExportDialog::contents_rows() const -> QList<MetaListRow> {
  return ToMetaListRows(BuildProfileExportContents(areas_, include_workspace_));
}

auto ProfileExportDialog::choice() const -> ProfileExportChoice {
  ProfileExportChoice choice;
  choice.has_destination = !destination_.isEmpty();
  choice.include_workspace = IncludeWorkspace();
  choice.passphrase_acceptable = entry_->Acceptable();
  choice.total_bytes = TotalProfileExportBytes(
      BuildProfileExportContents(areas_, choice.include_workspace));
  choice.free_bytes = free_bytes_;
  return choice;
}

void ProfileExportDialog::slot_choose_destination() {
  auto suggestion = QString("%1/%2%3").arg(
      destination_.isEmpty() ? GetDefaultUserFilePath()
                             : QFileInfo(destination_).absolutePath(),
      display_name_.simplified().replace(' ', '-').toLower(),
      kProfilePackageExtension);

  const auto chosen = QFileDialog::getSaveFileName(
      this, tr("Export Profile"), suggestion, ProfilePackageNameFilter());
  if (chosen.isEmpty()) return;

  destination_ = chosen.endsWith(kProfilePackageExtension, Qt::CaseInsensitive)
                     ? chosen
                     : chosen + kProfilePackageExtension;

  // Free space as a fact, not a prediction. What the file will actually occupy
  // is unknowable before it is packed, because the payload is compressed on the
  // way out; what the volume has right now is simply true.
  const QStorageInfo storage(QFileInfo(destination_).absolutePath());
  free_bytes_ = storage.isValid() ? storage.bytesAvailable() : -1;

  slot_state_changed();
  if (auto* finish = button(QWizard::FinishButton)) finish->setFocus();
}

void ProfileExportDialog::slot_state_changed() {
  const auto current = choice();

  // Re-rendered so the total visibly moves when the workspace is ticked; a
  // number that does not react to the checkbox beside it teaches the user that
  // the list is decoration.
  contents_summary_->setText(DescribeProfileExportContents(
      BuildProfileExportContents(areas_, include_workspace_)));
  contents_->RefreshValues(contents_rows());

  review_->SetRows(BuildProfileExportReviewRows(display_name_, current));

  const auto native = QDir::toNativeSeparators(destination_);
  destination_edit_->setText(native);
  destination_edit_->setToolTip(native);
  // The end of a long path is the part that names the file.
  destination_edit_->setCursorPosition(static_cast<int>(native.size()));

  const auto free_space = DescribeProfileExportFreeSpace(free_bytes_);
  free_space_label_->setText(free_space);
  free_space_label_->setVisible(!destination_.isEmpty() &&
                                !free_space.isEmpty());

  // Only the first warning is shown. They are ordered by what it would cost to
  // ignore them, and a stack of alarms is read as one alarm.
  const auto readiness = EvaluateProfileExport(current);
  warning_label_->setVisible(!readiness.warnings.isEmpty());
  if (!readiness.warnings.isEmpty()) {
    warning_label_->setText(
        DescribeProfileExportWarning(readiness.warnings.front()));
  }

  emit pages_.at(StepIndex(ProfileExportStep::kProtection))->completeChanged();
  emit pages_.at(StepIndex(ProfileExportStep::kSave))->completeChanged();
}

auto ProfileExportDialog::DestinationPath() const -> QString {
  return destination_;
}

auto ProfileExportDialog::IncludeWorkspace() const -> bool {
  return include_workspace_;
}

auto ProfileExportDialog::Protection() const -> ProfilePackageProtection {
  return ProfilePackageProtection::kPIN;
}

auto ProfileExportDialog::Passphrase() const -> GFBuffer {
  return entry_->Secret();
}

}  // namespace GpgFrontend::UI
