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
#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "ui_GeneralSettings.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GeneralSettings>()) {
  ui_->setupUi(this);

  ui_->cacheBox->setTitle(tr("Cache"));
  ui_->clearGpgPasswordCacheCheckBox->setText(
      tr("Clear gpg password cache when closing GpgFrontend."));
  ui_->restoreTextEditorPageCheckBox->setText(
      tr("Automatically restore unsaved Text Editor pages after an application "
         "crash."));

  ui_->importConfirmationBox->setTitle(tr("Operation"));
  ui_->longerKeyExpirationDateCheckBox->setText(
      tr("Enable to use longer key expiration date."));
  ui_->importConfirmationCheckBox->setText(
      tr("Import files dropped on the Key List without confirmation."));
  ui_->disableLoadingModulesCheckBox->setText(
      tr("Disable loading of all modules (including integrated modules)"));

  ui_->langBox->setTitle(tr("Language"));
  ui_->langNoteLabel->setText(
      "<b>" + tr("NOTE") + tr(": ") + "</b>" +
      tr("GpgFrontend will restart automatically if you change the language!"));

  ui_->dataBox->setTitle(tr("Data"));
  ui_->clearAllLogFilesButton->setText(
      tr("Clear All Log (Total Size: %1)")
          .arg(GlobalSettingStation::GetInstance().GetLogFilesSize()));
  ui_->clearAllDataObjectsButton->setText(
      tr("Clear All Data Objects (Total Size: %1)")
          .arg(GlobalSettingStation::GetInstance().GetDataObjectsFilesSize()));

  ui_->revealInFileExplorerButton->setText(tr("Reveal in File Explorer"));

  lang_ = SettingsDialog::ListLanguages();
  for (const auto& l : lang_) {
    ui_->langSelectBox->addItem(l);
  }
  connect(ui_->langSelectBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &GeneralTab::SignalRestartNeeded);

  connect(ui_->clearAllLogFilesButton, &QPushButton::clicked, this, [=]() {
    GlobalSettingStation::GetInstance().ClearAllLogFiles();
    ui_->clearAllLogFilesButton->setText(
        tr("Clear All Log (Total Size: %1)")
            .arg(GlobalSettingStation::GetInstance().GetLogFilesSize()));
  });

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
  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  auto clear_gpg_password_cache =
      settings.value("basic/clear_gpg_password_cache", true).toBool();
  ui_->clearGpgPasswordCacheCheckBox->setCheckState(
      clear_gpg_password_cache ? Qt::Checked : Qt::Unchecked);

  auto restore_text_editor_page =
      settings.value("basic/restore_text_editor_page", true).toBool();
  ui_->restoreTextEditorPageCheckBox->setCheckState(
      restore_text_editor_page ? Qt::Checked : Qt::Unchecked);

  auto longer_expiration_date =
      settings.value("basic/longer_expiration_date", false).toBool();
  ui_->longerKeyExpirationDateCheckBox->setCheckState(
      longer_expiration_date ? Qt::Checked : Qt::Unchecked);

  auto confirm_import_keys =
      settings.value("basic/confirm_import_keys", false).toBool();
  ui_->importConfirmationCheckBox->setCheckState(
      confirm_import_keys ? Qt::Checked : Qt::Unchecked);

  auto disable_loading_all_modules =
      settings.value("basic/disable_loading_all_modules", false).toBool();
  ui_->disableLoadingModulesCheckBox->setCheckState(
      disable_loading_all_modules ? Qt::Checked : Qt::Unchecked);

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
  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

  settings.setValue("basic/longer_expiration_date",
                    ui_->longerKeyExpirationDateCheckBox->isChecked());
  settings.setValue("basic/clear_gpg_password_cache",
                    ui_->clearGpgPasswordCacheCheckBox->isChecked());
  settings.setValue("basic/restore_text_editor_page",
                    ui_->restoreTextEditorPageCheckBox->isChecked());
  settings.setValue("basic/confirm_import_keys",
                    ui_->importConfirmationCheckBox->isChecked());
  settings.setValue("basic/disable_loading_all_modules",
                    ui_->disableLoadingModulesCheckBox->isChecked());
  settings.setValue("basic/lang", lang_.key(ui_->langSelectBox->currentText()));
}

}  // namespace GpgFrontend::UI
