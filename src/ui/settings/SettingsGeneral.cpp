/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "SettingsGeneral.h"

#ifdef MULTI_LANG_SUPPORT
#include "SettingsDialog.h"
#endif

#include "GlobalSettingStation.h"
#include "ui_GeneralSettings.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent), ui(std::make_shared<Ui_GeneralSettings>()) {
  ui->setupUi(this);

  ui->saveCheckedKeysBox->setTitle(_("Save Checked Keys"));
  ui->saveCheckedKeysCheckBox->setText(
      _("Save checked private keys on exit and restore them on next start."));
  ui->longerKeyExpirationDateBox->setTitle(_("Longer Key Expiration Date"));
  ui->longerKeyExpirationDateCheckBox->setText(
      _("Unlock key expiration date setting up to 30 years."));
  ui->importConfirmationBox->setTitle(_("Confirm drag'n'drop key import"));
  ui->importConfirmationCheckBox->setText(
      _("Import files dropped on the Key List without confirmation."));

  ui->asciiModeBox->setTitle(_("ASCII Mode"));
  ui->asciiModeCheckBox->setText(
      _("ASCII encoding is not used when file encrypting and "
        "signing."));

  ui->langBox->setTitle(_("Language"));
  ui->langNoteLabel->setText(
      "<b>" + QString(_("NOTE")) + _(": ") + "</b>" +
      _("GpgFrontend will restart automatically if you change the language!"));

#ifdef MULTI_LANG_SUPPORT
  lang = SettingsDialog::listLanguages();
  for (const auto& l : lang) {
    ui->langSelectBox->addItem(l);
  }
  connect(ui->langSelectBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(slotLanguageChanged()));
#endif

  setSettings();
}

/**********************************
 * Read the settings from config
 * and set the buttons and checkboxes
 * appropriately
 **********************************/
void GeneralTab::setSettings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
  try {
    bool save_key_checked = settings.lookup("general.save_key_checked");
    if (save_key_checked)
      ui->saveCheckedKeysCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("save_key_checked");
  }

  try {
    bool longer_expiration_date =
        settings.lookup("general.longer_expiration_date");
    LOG(INFO) << "longer_expiration_date" << longer_expiration_date;
    if (longer_expiration_date)
      ui->longerKeyExpirationDateCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("longer_expiration_date");
  }

#ifdef MULTI_LANG_SUPPORT
  try {
    std::string lang_key = settings.lookup("general.lang");
    QString lang_value = lang.value(lang_key.c_str());
    LOG(INFO) << "lang settings current" << lang_value.toStdString();
    if (!lang.empty()) {
      ui->langSelectBox->setCurrentIndex(
          ui->langSelectBox->findText(lang_value));
    } else {
      ui->langSelectBox->setCurrentIndex(0);
    }
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("lang");
  }
#endif

  try {
    bool confirm_import_keys = settings.lookup("general.confirm_import_keys");
    LOG(INFO) << "confirm_import_keys" << confirm_import_keys;
    if (confirm_import_keys)
      ui->importConfirmationCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("confirm_import_keys");
  }

  try {
    bool non_ascii_when_export =
        settings.lookup("general.non_ascii_when_export");
    LOG(INFO) << "non_ascii_when_export" << non_ascii_when_export;
    if (non_ascii_when_export)
      ui->asciiModeCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("non_ascii_when_export");
  }
}

/***********************************
 * get the values of the buttons and
 * write them to settings-file
 *************************************/
void GeneralTab::applySettings() {
  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
    settings.add("general", libconfig::Setting::TypeGroup);

  auto& general = settings["general"];

  if (!general.exists("longer_expiration_date"))
    general.add("longer_expiration_date", libconfig::Setting::TypeBoolean) =
        ui->longerKeyExpirationDateCheckBox->isChecked();
  else {
    general["longer_expiration_date"] =
        ui->longerKeyExpirationDateCheckBox->isChecked();
  }

  if (!general.exists("save_key_checked"))
    general.add("save_key_checked", libconfig::Setting::TypeBoolean) =
        ui->saveCheckedKeysCheckBox->isChecked();
  else {
    general["save_key_checked"] = ui->saveCheckedKeysCheckBox->isChecked();
  }

  if (!general.exists("non_ascii_when_export"))
    general.add("non_ascii_when_export", libconfig::Setting::TypeBoolean) =
        ui->asciiModeCheckBox->isChecked();
  else {
    general["non_ascii_when_export"] = ui->asciiModeCheckBox->isChecked();
  }

#ifdef MULTI_LANG_SUPPORT
  if (!general.exists("lang"))
    general.add("lang", libconfig::Setting::TypeBoolean) =
        lang.key(ui->langSelectBox->currentText()).toStdString();
  else {
    general["lang"] = lang.key(ui->langSelectBox->currentText()).toStdString();
  }
#endif

  if (!general.exists("confirm_import_keys"))
    general.add("confirm_import_keys", libconfig::Setting::TypeBoolean) =
        ui->importConfirmationCheckBox->isChecked();
  else {
    general["confirm_import_keys"] =
        ui->importConfirmationCheckBox->isChecked();
  }
}

#ifdef MULTI_LANG_SUPPORT
void GeneralTab::slotLanguageChanged() { emit signalRestartNeeded(true); }
#endif

}  // namespace GpgFrontend::UI
