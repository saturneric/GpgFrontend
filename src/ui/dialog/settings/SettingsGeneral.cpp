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

#include "SettingsDialog.h"
#include "core/function/GlobalSettingStation.h"
#include "core/utils/CommonUtils.h"
#include "ui_GeneralSettings.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GeneralSettings>()) {
  ui_->setupUi(this);

  ui_->cacheBox->setTitle(tr("Base"));

  ui_->clearGpgPasswordCacheCheckBox->setText(
      tr("Clear gpg password cache when closing GpgFrontend."));

  // Hide the "Clear gpg password cache" option if GnuPG is not supported, since
  // this option is not useful without GnuPG and may cause confusion to users.
  auto if_gnupg_supported = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG);
  if (!if_gnupg_supported) {
    ui_->clearGpgPasswordCacheCheckBox->setHidden(true);
  }

  ui_->modulePolicyLabel->setText(tr("Module Loading Policy:"));
  ui_->modulePolicyComboBox->addItem(tr("Only Integrated Modules"),
                                     "only_integrated");
  ui_->modulePolicyComboBox->addItem(tr("All Modules"), "all");
  ui_->modulePolicyComboBox->addItem(tr("Disable"), "disable");

  ui_->importConfirmationBox->setTitle(tr("Operation"));

  ui_->defaultWorkspaceAsLabel->setText(tr("Default Workspace As:"));
  ui_->filePanelRadioButton->setText(tr("File Panel"));
  ui_->textEditorRadioButton->setText(tr("Text Editor"));

  auto* workspace_button_group = new QButtonGroup(this);
  workspace_button_group->addButton(ui_->filePanelRadioButton);
  workspace_button_group->addButton(ui_->textEditorRadioButton);

  ui_->homePathAsDefaultPathcheckBox->setText(
      tr("Use home path as the default path for FilePanel"));

  ui_->restoreTextEditorPageCheckBox->setText(
      tr("Cache text editor contents."));

  ui_->importConfirmationCheckBox->setText(
      tr("Import files dropped on the Key List without confirmation."));

  ui_->asciiModeCheckBox->setText(tr("Use Binary Mode for File Operations"));

  ui_->langBox->setTitle(tr("Language"));
  ui_->langNoteLabel->setText(
      "<b>" + tr("NOTE") + tr(": ") + "</b>" +
      tr("GpgFrontend will restart automatically if you change the language!"));

  ui_->dataBox->setTitle(tr("Data"));
  ui_->clearAllDataObjectsButton->setText(
      tr("Clear All Data Objects (Total Size: %1)")
          .arg(GlobalSettingStation::GetInstance().GetDataObjectsFilesSize()));

  // Hide some options if running in sandbox, since they are not useful in
  // sandbox and may cause confusion to users.
  if (IsRunningInSandBox()) {
    ui_->dataBox->setHidden(true);
    ui_->modulePolicyComboBox->setHidden(true);
    ui_->modulePolicyLabel->setHidden(true);
    ui_->defaultWorkspaceAsLabel->setHidden(true);
    ui_->filePanelRadioButton->setHidden(true);
    ui_->textEditorRadioButton->setHidden(true);
  }

  ui_->revealInFileExplorerButton->setText(tr("Reveal in File Explorer"));

  lang_ = SettingsDialog::ListLanguages();
  for (const auto& l : lang_) {
    ui_->langSelectBox->addItem(l);
  }
  connect(ui_->langSelectBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &GeneralTab::SignalRestartNeeded);

  connect(ui_->clearAllDataObjectsButton, &QPushButton::clicked, this, [=]() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this, tr("Confirm"),
        tr("Are you sure you want to clear all data objects?\nThis will result "
           "in loss of all cached form positions, statuses, key servers, etc."),
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

  auto default_workspace_as =
      settings.value("basic/default_workspace_as", "file_panel").toString();
  if (default_workspace_as == "file_panel") {
    ui_->filePanelRadioButton->setChecked(true);
  } else {
    ui_->textEditorRadioButton->setChecked(true);
  }
  auto home_path_as_file_panel_default_path =
      settings.value("basic/home_path_as_file_panel_default_path", true)
          .toBool();
  ui_->homePathAsDefaultPathcheckBox->setCheckState(
      home_path_as_file_panel_default_path ? Qt::Checked : Qt::Unchecked);

  auto clear_gpg_password_cache =
      settings.value("basic/clear_gpg_password_cache", true).toBool();
  ui_->clearGpgPasswordCacheCheckBox->setCheckState(
      clear_gpg_password_cache ? Qt::Checked : Qt::Unchecked);

  auto restore_text_editor_page =
      settings.value("basic/restore_text_editor_page", true).toBool();
  ui_->restoreTextEditorPageCheckBox->setCheckState(
      restore_text_editor_page ? Qt::Checked : Qt::Unchecked);

  auto confirm_import_keys =
      settings.value("basic/confirm_import_keys", false).toBool();
  ui_->importConfirmationCheckBox->setCheckState(
      confirm_import_keys ? Qt::Checked : Qt::Unchecked);

  auto module_loading_policy =
      settings.value("basic/module_loading_policy", "only_integrated")
          .toString();
  if (module_loading_policy == "all") {
    ui_->modulePolicyComboBox->setCurrentIndex(
        ui_->modulePolicyComboBox->findData("all"));
  } else if (module_loading_policy == "only_integrated") {
    ui_->modulePolicyComboBox->setCurrentIndex(
        ui_->modulePolicyComboBox->findData("only_integrated"));
  } else if (module_loading_policy == "disable") {
    ui_->modulePolicyComboBox->setCurrentIndex(
        ui_->modulePolicyComboBox->findData("disable"));
  } else {
    ui_->modulePolicyComboBox->findData("only_integrated");
  }

  auto non_ascii_at_file_operation =
      settings.value("gnupg/non_ascii_at_file_operation", true).toBool();
  if (non_ascii_at_file_operation) {
    ui_->asciiModeCheckBox->setCheckState(Qt::Checked);
  }

  auto lang_key = settings.value("basic/lang").toString();
  auto lang_value = lang_.value(lang_key);
  if (!lang_.empty()) {
    ui_->langSelectBox->setCurrentIndex(
        ui_->langSelectBox->findText(lang_value));
  } else {
    ui_->langSelectBox->setCurrentIndex(0);
  }
}

void GeneralTab::ApplySettings() {
  auto settings = GpgFrontend::GetSettings();

  settings.setValue(
      "basic/default_workspace_as",
      ui_->filePanelRadioButton->isChecked() ? "file_panel" : "text_editor");

  settings.setValue("basic/home_path_as_file_panel_default_path",
                    ui_->homePathAsDefaultPathcheckBox->isChecked());
  settings.setValue("basic/clear_gpg_password_cache",
                    ui_->clearGpgPasswordCacheCheckBox->isChecked());
  settings.setValue("basic/restore_text_editor_page",
                    ui_->restoreTextEditorPageCheckBox->isChecked());
  settings.setValue("basic/confirm_import_keys",
                    ui_->importConfirmationCheckBox->isChecked());
  settings.setValue("basic/lang", lang_.key(ui_->langSelectBox->currentText()));
  settings.setValue("basic/module_loading_policy",
                    ui_->modulePolicyComboBox->currentData().toString());
  settings.setValue("gnupg/non_ascii_at_file_operation",
                    ui_->asciiModeCheckBox->isChecked());
}

}  // namespace GpgFrontend::UI
