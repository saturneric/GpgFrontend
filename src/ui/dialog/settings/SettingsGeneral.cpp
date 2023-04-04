/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "SettingsGeneral.h"

#include "core/GpgContext.h"

#ifdef MULTI_LANG_SUPPORT
#include "SettingsDialog.h"
#endif

#include "core/function/GlobalSettingStation.h"
#include "ui_GeneralSettings.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent), ui_(std::make_shared<Ui_GeneralSettings>()) {
  ui_->setupUi(this);

  ui_->cacheBox->setTitle(_("Cache"));
  ui_->saveCheckedKeysCheckBox->setText(
      _("Save checked private keys on exit and restore them on next start."));
  ui_->clearGpgPasswordCacheCheckBox->setText(
      _("Clear gpg password cache when closing GpgFrontend."));

  ui_->importConfirmationBox->setTitle(_("Operation"));
  ui_->longerKeyExpirationDateCheckBox->setText(
      _("Enable to use longer key expiration date."));
  ui_->importConfirmationCheckBox->setText(
      _("Import files dropped on the Key List without confirmation."));

  ui_->langBox->setTitle(_("Language"));
  ui_->langNoteLabel->setText(
      "<b>" + QString(_("NOTE")) + _(": ") + "</b>" +
      _("GpgFrontend will restart automatically if you change the language!"));

#ifdef MULTI_LANG_SUPPORT
  lang_ = SettingsDialog::ListLanguages();
  for (const auto& l : lang_) {
    ui_->langSelectBox->addItem(l);
  }
  connect(ui_->langSelectBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &GeneralTab::slot_language_changed);
#endif

  SetSettings();
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void GeneralTab::SetSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    bool save_key_checked = settings.lookup("general.save_key_checked");
    if (save_key_checked)
      ui_->saveCheckedKeysCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: save_key_checked");
  }

  try {
    bool clear_gpg_password_cache =
        settings.lookup("general.clear_gpg_password_cache");
    if (clear_gpg_password_cache)
      ui_->clearGpgPasswordCacheCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: clear_gpg_password_cache");
  }

  try {
    bool longer_expiration_date =
        settings.lookup("general.longer_expiration_date");
    SPDLOG_DEBUG("longer_expiration_date: {}", longer_expiration_date);
    if (longer_expiration_date)
      ui_->longerKeyExpirationDateCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: longer_expiration_date");
  }

#ifdef MULTI_LANG_SUPPORT
  try {
    std::string lang_key = settings.lookup("general.lang");
    QString lang_value = lang_.value(lang_key.c_str());
    SPDLOG_DEBUG("lang settings current: {}", lang_value.toStdString());
    if (!lang_.empty()) {
      ui_->langSelectBox->setCurrentIndex(
          ui_->langSelectBox->findText(lang_value));
    } else {
      ui_->langSelectBox->setCurrentIndex(0);
    }
  } catch (...) {
    SPDLOG_ERROR("setting operation error: lang");
  }
#endif

  try {
    bool confirm_import_keys = settings.lookup("general.confirm_import_keys");
    SPDLOG_DEBUG("confirm_import_keys: {}", confirm_import_keys);
    if (confirm_import_keys)
      ui_->importConfirmationCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: confirm_import_keys");
  }
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void GeneralTab::ApplySettings() {
  auto& settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
    settings.add("general", libconfig::Setting::TypeGroup);

  auto& general = settings["general"];

  if (!general.exists("longer_expiration_date"))
    general.add("longer_expiration_date", libconfig::Setting::TypeBoolean) =
        ui_->longerKeyExpirationDateCheckBox->isChecked();
  else {
    general["longer_expiration_date"] =
        ui_->longerKeyExpirationDateCheckBox->isChecked();
  }

  if (!general.exists("save_key_checked"))
    general.add("save_key_checked", libconfig::Setting::TypeBoolean) =
        ui_->saveCheckedKeysCheckBox->isChecked();
  else {
    general["save_key_checked"] = ui_->saveCheckedKeysCheckBox->isChecked();
  }

  if (!general.exists("clear_gpg_password_cache"))
    general.add("clear_gpg_password_cache", libconfig::Setting::TypeBoolean) =
        ui_->clearGpgPasswordCacheCheckBox->isChecked();
  else {
    general["clear_gpg_password_cache"] =
        ui_->saveCheckedKeysCheckBox->isChecked();
  }

#ifdef MULTI_LANG_SUPPORT
  if (!general.exists("lang"))
    general.add("lang", libconfig::Setting::TypeBoolean) =
        lang_.key(ui_->langSelectBox->currentText()).toStdString();
  else {
    general["lang"] =
        lang_.key(ui_->langSelectBox->currentText()).toStdString();
  }
#endif

  if (!general.exists("confirm_import_keys"))
    general.add("confirm_import_keys", libconfig::Setting::TypeBoolean) =
        ui_->importConfirmationCheckBox->isChecked();
  else {
    general["confirm_import_keys"] =
        ui_->importConfirmationCheckBox->isChecked();
  }
}

#ifdef MULTI_LANG_SUPPORT
void GeneralTab::slot_language_changed() { emit SignalRestartNeeded(true); }
#endif

}  // namespace GpgFrontend::UI
