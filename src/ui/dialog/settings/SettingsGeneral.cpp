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

#include "SettingsGeneral.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>

#include "SettingsDialog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/module/ModuleInit.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/function/FilePanelPath.h"
#include "ui/function/LanguageOptions.h"
#include "ui_GeneralSettings.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GeneralSettings>()) {
  ui_->setupUi(this);

  ui_->baseBox->setTitle(tr("Base"));

  ui_->modulePolicyLabel->setText(tr("Module Discovery:"));
  ui_->modulePolicyComboBox->addItem(tr("Only Integrated Modules"),
                                     "only_integrated");
  ui_->modulePolicyComboBox->addItem(tr("Also Look For Modules I've Added"),
                                     "all");
  ui_->modulePolicyComboBox->addItem(tr("Disable"), "disable");
  // Worded as discovery, because that is all it decides. Finding a module the
  // user added does not run it: its publisher key has to be trusted and the
  // module enabled, one at a time, in the module controller. A label reading
  // "All Modules" promised something the approval gate then quietly refused.
  //
  // No "Signed Packages Only": every policy is that now, because a native is
  // named by a verified descriptor rather than found on disk. A profile that
  // still stores the old key is read as the same value (see ModuleInit.h) and
  // rewritten the next time this dialog saves.

  ui_->importConfirmationBox->setTitle(tr("Operation"));

  ui_->defaultWorkspaceAsLabel->setText(tr("Default Workspace As:"));
  ui_->filePanelRadioButton->setText(tr("File Panel"));
  ui_->textEditorRadioButton->setText(tr("Text Editor"));

  auto* workspace_button_group = new QButtonGroup(this);
  workspace_button_group->addButton(ui_->filePanelRadioButton);
  workspace_button_group->addButton(ui_->textEditorRadioButton);

  ui_->filePanelDefaultPathLabel->setText(tr("File Panel Opens At:"));

  ui_->restoreTextEditorPageCheckBox->setText(
      tr("Cache text editor contents."));

  ui_->importConfirmationCheckBox->setText(
      tr("Import files dropped on the Key List without confirmation."));

  ui_->rememberCheckedKeysCheckBox->setText(
      tr("Remember which keys are checked in the key list between restarts."));
  ui_->singleRecipientModeCheckBox->setText(
      tr("Allow only one checked key in the key list."));
  ui_->singleRecipientModeCheckBox->setToolTip(
      tr("Checking a key unchecks the others, so a recipient from an earlier "
         "message is not added to the next one by mistake."));
  ui_->confirmMultipleRecipientsCheckBox->setText(
      tr("Ask before encrypting to more than one recipient."));
  ui_->undoableOperationsCheckBox->setText(
      tr("Allow undoing encrypt, decrypt and other operations in the text "
         "editor."));
  ui_->undoableOperationsCheckBox->setToolTip(
      tr("Undo keeps the previous text in memory. After encrypting, that is "
         "the plaintext."));

  ui_->asciiModeCheckBox->setText(tr("Use Binary Mode for File Operations"));

  ui_->expiringSoonLabel->setText(tr("Warn about keys expiring within:"));
  ui_->expiringSoonDaysSpinBox->setSuffix(tr(" days"));

  ui_->langBox->setTitle(tr("Language"));

  // Without wrapping, this note is laid out on a single line and drags the
  // whole settings page wider than the dialog, which stretches every control
  // on it -- the language box above most of all.
  ui_->langNoteLabel->setWordWrap(true);
  ui_->langNoteLabel->setText(
      "<b>" + tr("NOTE") + tr(": ") + "</b>" +
      tr("System Default follows your operating system. Choose a language "
         "here to override it. GpgFrontend restarts automatically when you "
         "change it."));

  ui_->dataBox->setTitle(tr("Data"));
  ui_->clearAllDataObjectsButton->setText(
      tr("Clear All Data Objects (Total Size: %1)")
          .arg(GlobalSettingStation::GetInstance().GetDataObjectsFilesSize()));

  // Hide some options in a sandbox, since they are not useful there and may
  // confuse users.
  if (IsRunningInSandBox()) {
    ui_->dataBox->setHidden(true);
    ui_->modulePolicyComboBox->setHidden(true);
    ui_->modulePolicyLabel->setHidden(true);
    ui_->defaultWorkspaceAsLabel->setHidden(true);
    ui_->filePanelRadioButton->setHidden(true);
    ui_->textEditorRadioButton->setHidden(true);
    ui_->filePanelDefaultPathLabel->setHidden(true);
    ui_->filePanelDefaultPathComboBox->setHidden(true);
  }

  ui_->revealInFileExplorerButton->setText(tr("Reveal in File Explorer"));

  connect(ui_->langSelectBox, &QComboBox::currentIndexChanged, this,
          &GeneralTab::SignalRestartNeeded);

  connect(ui_->clearAllDataObjectsButton, &QPushButton::clicked, this, [=]() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this, tr("Confirm"),
        tr("Are you sure you want to clear all data objects?\nThis will result "
           "in the loss of all cached form positions, statuses, key servers, "
           "etc."),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      GlobalSettingStation::GetInstance().ClearAllDataObjects();
      ui_->clearAllDataObjectsButton->setText(
          tr("Clear All Data Objects (Total Size: %1)")
              .arg(GlobalSettingStation::GetInstance()
                       .GetDataObjectsFilesSize()));
    }
  });

  connect(ui_->revealInFileExplorerButton, &QPushButton::clicked, this, [=]() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(
        GlobalSettingStation::GetInstance().GetAppDataPath()));
  });

  SetSettings();
}

void GeneralTab::SetSettings() {
  auto settings = GetSettings();

  auto default_workspace_as = SettingItems::kDefaultWorkspaceAs.Read(settings);
  if (default_workspace_as == "file_panel") {
    ui_->filePanelRadioButton->setChecked(true);
  } else {
    ui_->textEditorRadioButton->setChecked(true);
  }
  // Populated here rather than in the .ui so the workspace entry can carry the
  // profile's actual folder in its tooltip, and so it can be hidden for a
  // classic profile, which has no workspace of its own.
  ui_->filePanelDefaultPathComboBox->clear();
  if (const auto workspace = ProfileSession::Instance().WorkspacePath();
      !workspace.isEmpty()) {
    ui_->filePanelDefaultPathComboBox->addItem(
        tr("Profile Workspace"),
        FilePanelDefaultPathModeToString(FilePanelDefaultPathMode::kWORKSPACE));
    ui_->filePanelDefaultPathComboBox->setItemData(0, workspace,
                                                   Qt::ToolTipRole);
  }
  ui_->filePanelDefaultPathComboBox->addItem(
      tr("Home Folder"),
      FilePanelDefaultPathModeToString(FilePanelDefaultPathMode::kHOME));
  ui_->filePanelDefaultPathComboBox->addItem(
      tr("Working Directory"),
      FilePanelDefaultPathModeToString(FilePanelDefaultPathMode::kCWD));

  const auto stored_mode =
      SettingItems::kFilePanelDefaultPathMode.Read(settings);
  const auto mode = FilePanelDefaultPathModeToString(
      FilePanelDefaultPathModeFromString(stored_mode));
  const auto index = ui_->filePanelDefaultPathComboBox->findData(mode);
  ui_->filePanelDefaultPathComboBox->setCurrentIndex(index < 0 ? 0 : index);

  auto restore_text_editor_page =
      SettingItems::kRestoreTextEditorPage.Read(settings);
  ui_->restoreTextEditorPageCheckBox->setCheckState(
      restore_text_editor_page ? Qt::Checked : Qt::Unchecked);

  auto confirm_import_keys = SettingItems::kConfirmImportKeys.Read(settings);
  ui_->importConfirmationCheckBox->setCheckState(
      confirm_import_keys ? Qt::Checked : Qt::Unchecked);

  auto remember_checked_keys =
      SettingItems::kRememberCheckedKeys.Read(settings);
  ui_->rememberCheckedKeysCheckBox->setCheckState(
      remember_checked_keys ? Qt::Checked : Qt::Unchecked);

  auto single_recipient_mode =
      SettingItems::kSingleRecipientMode.Read(settings);
  ui_->singleRecipientModeCheckBox->setCheckState(
      single_recipient_mode ? Qt::Checked : Qt::Unchecked);

  auto confirm_multiple_recipients =
      SettingItems::kConfirmMultipleRecipients.Read(settings);
  ui_->confirmMultipleRecipientsCheckBox->setCheckState(
      confirm_multiple_recipients ? Qt::Checked : Qt::Unchecked);

  auto undoable_operations = SettingItems::kUndoableOperations.Read(settings);
  ui_->undoableOperationsCheckBox->setCheckState(
      undoable_operations ? Qt::Checked : Qt::Unchecked);

  ui_->expiringSoonDaysSpinBox->setValue(GetKeyExpiringSoonDays());

  // One parse, shared with the loader. The five-branch chain this replaces
  // ended in an else that called findData() and threw the answer away, so an
  // unrecognised stored value left the box showing whichever policy happened
  // to be selected -- the one case where showing the truth matters most.
  const auto policy = Module::ParseModuleLoadingPolicy(
                          SettingItems::kModuleLoadingPolicy.Read(settings))
                          .policy;
  ui_->modulePolicyComboBox->setCurrentIndex(
      ui_->modulePolicyComboBox->findData(
          Module::ModuleLoadingPolicyKey(policy)));

  auto non_ascii_at_file_operation =
      SettingItems::kNonAsciiAtFileOperation.Read(settings);
  if (non_ascii_at_file_operation) {
    ui_->asciiModeCheckBox->setCheckState(Qt::Checked);
  }

  // Refilled rather than only reselected, so a revert cannot leave the box on a
  // language the settings no longer hold. Safe on both paths: the first call
  // comes from this tab's own constructor, before SettingsDialog connects
  // SignalRestartNeeded, and a revert runs with restart declarations
  // suppressed.
  PopulateLanguageComboBox(ui_->langSelectBox,
                           SettingItems::kLanguage.Read(settings));
}

void GeneralTab::ApplySettings() {
  auto settings = GpgFrontend::GetSettings();

  SettingItems::kDefaultWorkspaceAs.Write(
      settings,
      ui_->filePanelRadioButton->isChecked() ? "file_panel" : "text_editor");

  SettingItems::kFilePanelDefaultPathMode.Write(
      settings, ui_->filePanelDefaultPathComboBox->currentData().toString());
  SettingItems::kRestoreTextEditorPage.Write(
      settings, ui_->restoreTextEditorPageCheckBox->isChecked());
  SettingItems::kConfirmImportKeys.Write(
      settings, ui_->importConfirmationCheckBox->isChecked());
  SettingItems::kRememberCheckedKeys.Write(
      settings, ui_->rememberCheckedKeysCheckBox->isChecked());
  SettingItems::kSingleRecipientMode.Write(
      settings, ui_->singleRecipientModeCheckBox->isChecked());
  SettingItems::kConfirmMultipleRecipients.Write(
      settings, ui_->confirmMultipleRecipientsCheckBox->isChecked());
  SettingItems::kUndoableOperations.Write(
      settings, ui_->undoableOperationsCheckBox->isChecked());
  SettingItems::kExpiringSoonDays.Write(settings,
                                        ui_->expiringSoonDaysSpinBox->value());
  SettingItems::kLanguage.Write(settings,
                                ui_->langSelectBox->currentData().toString());
  SettingItems::kModuleLoadingPolicy.Write(
      settings, ui_->modulePolicyComboBox->currentData().toString());
  SettingItems::kNonAsciiAtFileOperation.Write(
      settings, ui_->asciiModeCheckBox->isChecked());

}

}  // namespace GpgFrontend::UI
