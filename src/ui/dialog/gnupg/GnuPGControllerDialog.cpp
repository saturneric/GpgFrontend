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

#include "GnuPGControllerDialog.h"

#include "SignalStation.h"
#include "UserInterfaceUtils.h"
#include "core/function/GlobalSettingStation.h"
#include "ui/dialog/GeneralDialog.h"
#include "ui/main_window/MainWindow.h"
#include "ui_GnuPGControllerDialog.h"

namespace GpgFrontend::UI {

GnuPGControllerDialog::GnuPGControllerDialog(QWidget* parent)
    : GeneralDialog("GnuPGControllerDialog", parent),
      ui_(std::make_shared<Ui_GnuPGControllerDialog>()) {
  ui_->setupUi(this);

  ui_->generalBox->setTitle(_("General"));
  ui_->keyDatabaseGroupBox->setTitle(_("Key Database"));
  ui_->advanceGroupBox->setTitle(_("Advanced"));

  ui_->asciiModeCheckBox->setText(_("No ASCII Mode"));
  ui_->usePinentryAsPasswordInputDialogCheckBox->setText(
      _("Use Pinentry as Password Input Dialog"));
  ui_->useCustomGnuPGInstallPathCheckBox->setText(_("Use Custom GnuPG"));
  ui_->useCustomGnuPGInstallPathButton->setText(_("Select GnuPG Path"));
  ui_->keyDatabseUseCustomCheckBox->setText(
      _("Use Custom GnuPG Key Database Path"));
  ui_->customKeyDatabasePathSelectButton->setText(
      _("Select Key Database Path"));

  // announce main window
  connect(this, &GnuPGControllerDialog::SignalRestartNeeded,
          SignalStation::GetInstance(),
          &SignalStation::SignalRestartApplication);

  connect(ui_->keyDatabseUseCustomCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) {
            ui_->customKeyDatabasePathSelectButton->setDisabled(
                state != Qt::CheckState::Checked);
          });

  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this, [=](int state) {
            ui_->useCustomGnuPGInstallPathButton->setDisabled(
                state != Qt::CheckState::Checked);
          });

  connect(ui_->keyDatabseUseCustomCheckBox, &QCheckBox::stateChanged, this,
          &GnuPGControllerDialog::slot_update_custom_key_database_path_label);

  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this,
          &GnuPGControllerDialog::slot_update_custom_gnupg_install_path_label);

  connect(
      ui_->customKeyDatabasePathSelectButton, &QPushButton::clicked, this,
      [=]() {
        QString selected_custom_key_database_path =
            QFileDialog::getExistingDirectory(
                this, _("Open Directory"), {},
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        SPDLOG_DEBUG("key databse path selected: {}",
                     selected_custom_key_database_path.toStdString());

        if (!selected_custom_key_database_path.isEmpty()) {
          auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
          auto& general = settings["general"];

          // update settings
          if (!general.exists("custom_key_database_path"))
            general.add("custom_key_database_path",
                        libconfig::Setting::TypeString) =
                selected_custom_key_database_path.toStdString();
          else {
            general["custom_key_database_path"] =
                selected_custom_key_database_path.toStdString();
          }

          // announce the restart
          this->slot_set_restart_needed(DEEP_RESTART_CODE);

          // update ui
          this->slot_update_custom_key_database_path_label(
              this->ui_->keyDatabseUseCustomCheckBox->checkState());
        }
      });

  connect(
      ui_->useCustomGnuPGInstallPathButton, &QPushButton::clicked, this, [=]() {
        QString selected_custom_gnupg_install_path =
            QFileDialog::getExistingDirectory(
                this, _("Open Directory"), {},
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        SPDLOG_DEBUG("gnupg install path selected: {}",
                     selected_custom_gnupg_install_path.toStdString());

        if (!selected_custom_gnupg_install_path.isEmpty()) {
          auto& settings = GlobalSettingStation::GetInstance().GetUISettings();
          auto& general = settings["general"];

          // update settings
          if (!general.exists("custom_gnupg_install_path"))
            general.add("custom_gnupg_install_path",
                        libconfig::Setting::TypeString) =
                selected_custom_gnupg_install_path.toStdString();
          else {
            general["custom_gnupg_install_path"] =
                selected_custom_gnupg_install_path.toStdString();
          }

          // announce the restart
          this->slot_set_restart_needed(DEEP_RESTART_CODE);

          // update ui
          this->slot_update_custom_gnupg_install_path_label(
              this->ui_->useCustomGnuPGInstallPathCheckBox->checkState());
        }
      });

  connect(ui_->usePinentryAsPasswordInputDialogCheckBox,
          &QCheckBox::stateChanged, this, [=](int state) {
            // announce the restart
            this->slot_set_restart_needed(DEEP_RESTART_CODE);
          });

#ifndef MACOS
  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this,
          &GnuPGControllerDialog::SlotAccept);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this,
          &GnuPGControllerDialog::reject);
#else

  // macOS style settings
  ui_->buttonBox->setDisabled(true);
  ui_->buttonBox->setHidden(true);

  connect(this, &QDialog::finished, this, &GnuPGControllerDialog::SlotAccept);
  connect(this, &QDialog::finished, this, &GnuPGControllerDialog::deleteLater);
#endif

  setWindowTitle(_("GnuPG Controller"));
  set_settings();
}

void GnuPGControllerDialog::SlotAccept() {
  apply_settings();

  SPDLOG_DEBUG("gnupg controller apply done");

  // write settings to filesystem
  GlobalSettingStation::GetInstance().SyncSettings();

  SPDLOG_DEBUG("restart needed: {}", get_restart_needed());
  if (get_restart_needed()) {
    emit SignalRestartNeeded(get_restart_needed());
  }
  close();
}

void GnuPGControllerDialog::slot_update_custom_key_database_path_label(
    int state) {
  if (state != Qt::CheckState::Checked) {
    ui_->currentKeyDatabasePathLabel->setText(QString::fromStdString(
        GpgContext::GetInstance().GetInfo(false).DatabasePath));

    // hide label (not necessary to show the default path)
    this->ui_->currentKeyDatabasePathLabel->setHidden(true);
  } else {
    // read from settings file
    std::string custom_key_database_path;
    try {
      auto& settings =
          GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();
      custom_key_database_path = static_cast<std::string>(
          settings.lookup("general.custom_key_database_path"));

    } catch (...) {
      SPDLOG_ERROR("setting operation error: custom_key_database_path");
    }

    SPDLOG_DEBUG("selected_custom_key_database_path from settings: {}",
                 custom_key_database_path);

    // set label value
    if (!custom_key_database_path.empty()) {
      ui_->currentKeyDatabasePathLabel->setText(
          QString::fromStdString(custom_key_database_path));
      this->ui_->currentKeyDatabasePathLabel->setHidden(false);
    } else {
      this->ui_->currentKeyDatabasePathLabel->setHidden(true);
    }
  }
}

void GnuPGControllerDialog::slot_update_custom_gnupg_install_path_label(
    int state) {
  if (state != Qt::CheckState::Checked) {
    ui_->currentCustomGnuPGInstallPathLabel->setText(QString::fromStdString(
        GpgContext::GetInstance().GetInfo(false).GnuPGHomePath));

    // hide label (not necessary to show the default path)
    this->ui_->currentCustomGnuPGInstallPathLabel->setHidden(true);
  } else {
    // read from settings file
    std::string custom_gnupg_install_path;
    try {
      auto& settings =
          GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();
      custom_gnupg_install_path = static_cast<std::string>(
          settings.lookup("general.custom_gnupg_install_path"));

    } catch (...) {
      SPDLOG_ERROR("setting operation error: custom_gnupg_install_path");
    }

    SPDLOG_DEBUG("custom_gnupg_install_path from settings: {}",
                 custom_gnupg_install_path);

    // set label value
    if (!custom_gnupg_install_path.empty()) {
      ui_->currentCustomGnuPGInstallPathLabel->setText(
          QString::fromStdString(custom_gnupg_install_path));
      this->ui_->currentCustomGnuPGInstallPathLabel->setHidden(false);
    } else {
      this->ui_->currentCustomGnuPGInstallPathLabel->setHidden(true);
    }
  }
}

void GnuPGControllerDialog::set_settings() {
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  try {
    bool non_ascii_when_export =
        settings.lookup("general.non_ascii_when_export");
    SPDLOG_DEBUG("non_ascii_when_export: {}", non_ascii_when_export);
    if (non_ascii_when_export)
      ui_->asciiModeCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: non_ascii_when_export");
  }

  try {
    bool use_custom_key_database_path =
        settings.lookup("general.use_custom_key_database_path");
    if (use_custom_key_database_path)
      ui_->keyDatabseUseCustomCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: use_custom_key_database_path");
  }

  this->slot_update_custom_key_database_path_label(
      ui_->keyDatabseUseCustomCheckBox->checkState());

  try {
    bool use_custom_gnupg_install_path =
        settings.lookup("general.use_custom_gnupg_install_path");
    if (use_custom_gnupg_install_path)
      ui_->useCustomGnuPGInstallPathCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR("setting operation error: use_custom_gnupg_install_path");
  }

  try {
    bool use_pinentry_as_password_input_dialog =
        settings.lookup("general.use_pinentry_as_password_input_dialog");
    if (use_pinentry_as_password_input_dialog)
      ui_->usePinentryAsPasswordInputDialogCheckBox->setCheckState(Qt::Checked);
  } catch (...) {
    SPDLOG_ERROR(
        "setting operation error: use_pinentry_as_password_input_dialog");
  }

  this->slot_update_custom_gnupg_install_path_label(
      ui_->useCustomGnuPGInstallPathCheckBox->checkState());
}

void GnuPGControllerDialog::apply_settings() {
  auto& settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
    settings.add("general", libconfig::Setting::TypeGroup);

  auto& general = settings["general"];

  if (!general.exists("non_ascii_when_export"))
    general.add("non_ascii_when_export", libconfig::Setting::TypeBoolean) =
        ui_->asciiModeCheckBox->isChecked();
  else {
    general["non_ascii_when_export"] = ui_->asciiModeCheckBox->isChecked();
  }

  if (!general.exists("use_custom_key_database_path"))
    general.add("use_custom_key_database_path",
                libconfig::Setting::TypeBoolean) =
        ui_->keyDatabseUseCustomCheckBox->isChecked();
  else {
    general["use_custom_key_database_path"] =
        ui_->keyDatabseUseCustomCheckBox->isChecked();
  }

  if (!general.exists("use_custom_gnupg_install_path"))
    general.add("use_custom_gnupg_install_path",
                libconfig::Setting::TypeBoolean) =
        ui_->useCustomGnuPGInstallPathCheckBox->isChecked();
  else {
    general["use_custom_gnupg_install_path"] =
        ui_->useCustomGnuPGInstallPathCheckBox->isChecked();
  }

  if (!general.exists("use_pinentry_as_password_input_dialog"))
    general.add("use_pinentry_as_password_input_dialog",
                libconfig::Setting::TypeBoolean) =
        ui_->usePinentryAsPasswordInputDialogCheckBox->isChecked();
  else {
    general["use_pinentry_as_password_input_dialog"] =
        ui_->usePinentryAsPasswordInputDialogCheckBox->isChecked();
  }
}

int GnuPGControllerDialog::get_restart_needed() const {
  return this->restart_needed_;
}

void GnuPGControllerDialog::slot_set_restart_needed(int mode) {
  this->restart_needed_ = mode;
}

}  // namespace GpgFrontend::UI
