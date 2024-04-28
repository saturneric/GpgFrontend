/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "GnuPGControllerDialog.h"

#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/GeneralDialog.h"
#include "ui_GnuPGControllerDialog.h"

namespace GpgFrontend::UI {

GnuPGControllerDialog::GnuPGControllerDialog(QWidget* parent)
    : GeneralDialog("GnuPGControllerDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GnuPGControllerDialog>()) {
  ui_->setupUi(this);

  ui_->generalBox->setTitle(tr("General"));
  ui_->keyDatabaseGroupBox->setTitle(tr("Key Database"));
  ui_->advanceGroupBox->setTitle(tr("Advanced"));

  ui_->asciiModeCheckBox->setText(tr("Use Binary Mode for File Operations"));
  ui_->usePinentryAsPasswordInputDialogCheckBox->setText(
      tr("Use Pinentry as Password Input Dialog"));
  ui_->gpgmeDebugLogCheckBox->setText(tr("Enable GpgME Debug Log"));
  ui_->useCustomGnuPGInstallPathCheckBox->setText(tr("Use Custom GnuPG"));
  ui_->useCustomGnuPGInstallPathButton->setText(tr("Select GnuPG Path"));
  ui_->keyDatabseUseCustomCheckBox->setText(
      tr("Use Custom GnuPG Key Database Path"));
  ui_->customKeyDatabasePathSelectButton->setText(
      tr("Select Key Database Path"));
  ui_->restartGpgAgentOnStartCheckBox->setText(
      tr("Restart Gpg Agent on start"));

  // tips
  ui_->customGnuPGPathTipsLabel->setText(
      tr("Tips: please select a directroy where \"gpgconf\" is located in."));
  ui_->restartTipsLabel->setText(
      tr("Tips: notice that modify any of these settings will cause an "
         "Application restart."));

  // announce main window
  connect(this, &GnuPGControllerDialog::SignalRestartNeeded,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRestartApplication);

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
                this, tr("Open Directory"), {},
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        GF_UI_LOG_DEBUG("key databse path selected: {}",
                        selected_custom_key_database_path);

        custom_key_database_path_ = selected_custom_key_database_path;

        // announce the restart
        this->slot_set_restart_needed(kDeepRestartCode);

        // update ui
        this->slot_update_custom_key_database_path_label(
            this->ui_->keyDatabseUseCustomCheckBox->checkState());
      });

  connect(
      ui_->useCustomGnuPGInstallPathButton, &QPushButton::clicked, this, [=]() {
        QString selected_custom_gnupg_install_path =
            QFileDialog::getExistingDirectory(
                this, tr("Open Directory"), {},
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        GF_UI_LOG_DEBUG("gnupg install path selected: {}",
                        selected_custom_gnupg_install_path);

        custom_gnupg_path_ = selected_custom_gnupg_install_path;

        // announce the restart
        this->slot_set_restart_needed(kDeepRestartCode);

        // update ui
        this->slot_update_custom_gnupg_install_path_label(
            this->ui_->useCustomGnuPGInstallPathCheckBox->checkState());
      });

  connect(ui_->usePinentryAsPasswordInputDialogCheckBox,
          &QCheckBox::stateChanged, this, [=](int) {
            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
          });

  connect(ui_->gpgmeDebugLogCheckBox, &QCheckBox::stateChanged, this, [=](int) {
    // announce the restart
    this->slot_set_restart_needed(kDeepRestartCode);
  });

  connect(ui_->asciiModeCheckBox, &QCheckBox::stateChanged, this, [=](int) {
    // announce the restart
    this->slot_set_restart_needed(kDeepRestartCode);
  });

  connect(ui_->restartGpgAgentOnStartCheckBox, &QCheckBox::stateChanged, this,
          [=](int) {
            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
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

  setWindowTitle(tr("GnuPG Controller"));
  set_settings();
}

void GnuPGControllerDialog::SlotAccept() {
  apply_settings();

  GF_UI_LOG_DEBUG("gnupg controller apply done");
  GF_UI_LOG_DEBUG("restart needed: {}", get_restart_needed());
  if (get_restart_needed() != 0) {
    emit SignalRestartNeeded(get_restart_needed());
  }
  close();
}

void GnuPGControllerDialog::slot_update_custom_key_database_path_label(
    int state) {
  // hide label (not necessary to show the default path)
  this->ui_->currentKeyDatabasePathLabel->setHidden(state !=
                                                    Qt::CheckState::Checked);
  if (state == Qt::CheckState::Checked) {
    if (custom_key_database_path_.isEmpty()) {
      // read from settings file
      QString custom_key_database_path =
          GlobalSettingStation::GetInstance()
              .GetSettings()
              .value("gnupg/custom_key_database_path")
              .toString();
      GF_UI_LOG_DEBUG("selected_custom_key_database_path from settings: {}",
                      custom_key_database_path);
      custom_key_database_path_ = custom_key_database_path;
    }

    // notify the user
    if (!check_custom_gnupg_key_database_path(custom_key_database_path_)) {
      return;
    };

    // set label value
    if (!custom_key_database_path_.isEmpty()) {
      ui_->currentKeyDatabasePathLabel->setText(custom_key_database_path_);
    }
  }

  if (ui_->currentKeyDatabasePathLabel->text().isEmpty()) {
    const auto database_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.database_path", QString{});
    GF_UI_LOG_DEBUG("got gpgme.ctx.database_path from rt: {}", database_path);
    ui_->currentKeyDatabasePathLabel->setText(database_path);
  }
}

void GnuPGControllerDialog::slot_update_custom_gnupg_install_path_label(
    int state) {
  // hide label (not necessary to show the default path)
  this->ui_->currentCustomGnuPGInstallPathLabel->setHidden(
      state != Qt::CheckState::Checked);
  do {
    if (state == Qt::CheckState::Checked) {
      if (custom_gnupg_path_.isEmpty()) {
        // read from settings file
        QString custom_gnupg_install_path =
            GlobalSettingStation::GetInstance()
                .GetSettings()
                .value("gnupg/custom_gnupg_install_path")
                .toString();
        GF_UI_LOG_DEBUG("custom_gnupg_install_path from settings: {}",
                        custom_gnupg_install_path);
        custom_gnupg_path_ = custom_gnupg_install_path;
      }

      // notify the user
      if (!check_custom_gnupg_path(custom_gnupg_path_)) {
        break;
      }

      // set label value
      if (!custom_gnupg_path_.isEmpty()) {
        ui_->currentCustomGnuPGInstallPathLabel->setText(custom_gnupg_path_);
      }
    }
  } while (false);

  if (ui_->currentCustomGnuPGInstallPathLabel->text().isEmpty()) {
    const auto gnupg_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.app_path", QString{});
    GF_UI_LOG_DEBUG("got gnupg home path from rt: {}", gnupg_path);
    auto dir = QFileInfo(gnupg_path).path();
    ui_->currentCustomGnuPGInstallPathLabel->setText(dir);
  }
}

void GnuPGControllerDialog::set_settings() {
  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  bool non_ascii_at_file_operation =
      settings.value("gnupg/non_ascii_at_file_operation", true).toBool();
  if (non_ascii_at_file_operation)
    ui_->asciiModeCheckBox->setCheckState(Qt::Checked);

  bool const use_custom_key_database_path =
      settings.value("gnupg/use_custom_key_database_path", false).toBool();
  if (use_custom_key_database_path) {
    ui_->keyDatabseUseCustomCheckBox->setCheckState(Qt::Checked);
  }

  bool const enable_gpgme_debug_log =
      settings.value("gnupg/enable_gpgme_debug_log", false).toBool();
  if (enable_gpgme_debug_log) {
    ui_->gpgmeDebugLogCheckBox->setCheckState(Qt::Checked);
  }

  this->slot_update_custom_key_database_path_label(
      ui_->keyDatabseUseCustomCheckBox->checkState());

  bool const use_custom_gnupg_install_path =
      settings.value("gnupg/use_custom_gnupg_install_path", false).toBool();
  if (use_custom_gnupg_install_path) {
    ui_->useCustomGnuPGInstallPathCheckBox->setCheckState(Qt::Checked);
  }

  bool const use_pinentry_as_password_input_dialog =
      settings
          .value("gnupg/use_pinentry_as_password_input_dialog",
                 QString::fromLocal8Bit(qgetenv("container")) != "flatpak")
          .toBool();
  if (use_pinentry_as_password_input_dialog) {
    ui_->usePinentryAsPasswordInputDialogCheckBox->setCheckState(Qt::Checked);
  }

  bool const restart_gpg_agent_on_start =
      settings.value("gnupg/restart_gpg_agent_on_start", false).toBool();
  if (restart_gpg_agent_on_start) {
    ui_->restartGpgAgentOnStartCheckBox->setCheckState(Qt::Checked);
  }

  this->slot_update_custom_key_database_path_label(
      use_custom_key_database_path ? Qt::Checked : Qt::Unchecked);

  this->slot_update_custom_gnupg_install_path_label(
      use_custom_gnupg_install_path ? Qt::Checked : Qt::Unchecked);

  this->slot_set_restart_needed(kNonRestartCode);
}

void GnuPGControllerDialog::apply_settings() {
  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

  settings.setValue("gnupg/non_ascii_at_file_operation",
                    ui_->asciiModeCheckBox->isChecked());
  settings.setValue("gnupg/use_custom_key_database_path",
                    ui_->keyDatabseUseCustomCheckBox->isChecked());
  settings.setValue("gnupg/use_custom_gnupg_install_path",
                    ui_->useCustomGnuPGInstallPathCheckBox->isChecked());
  settings.setValue("gnupg/use_pinentry_as_password_input_dialog",
                    ui_->usePinentryAsPasswordInputDialogCheckBox->isChecked());
  settings.setValue("gnupg/enable_gpgme_debug_log",
                    ui_->gpgmeDebugLogCheckBox->isChecked());
  settings.setValue("gnupg/custom_key_database_path",
                    ui_->currentKeyDatabasePathLabel->text());
  settings.setValue("gnupg/custom_gnupg_install_path",
                    ui_->currentCustomGnuPGInstallPathLabel->text());
  settings.setValue("gnupg/restart_gpg_agent_on_start",
                    ui_->restartGpgAgentOnStartCheckBox->isChecked());
}

auto GnuPGControllerDialog::get_restart_needed() const -> int {
  return this->restart_needed_;
}

void GnuPGControllerDialog::slot_set_restart_needed(int mode) {
  GF_UI_LOG_INFO("announce restart needed, mode: {}", mode);
  this->restart_needed_ = mode;
}

auto GnuPGControllerDialog::check_custom_gnupg_path(QString path) -> bool {
  if (path.isEmpty()) return false;

  QFileInfo dir_info(path);
  if (!dir_info.exists() || !dir_info.isReadable() || !dir_info.isDir()) {
    QMessageBox::critical(
        this, tr("Illegal GnuPG Path"),
        tr("Target GnuPG Path is not an exists readable directory."));
    return false;
  }

  QDir dir(path);
  if (!dir.isAbsolute()) {
    QMessageBox::critical(this, tr("Illegal GnuPG Path"),
                          tr("Target GnuPG Path is not an absolute path."));
  }
#ifdef WINDOWS
  QFileInfo gpgconf_info(path + "/gpgconf.exe");
#else
  QFileInfo gpgconf_info(path + "/gpgconf");
#endif

  if (!gpgconf_info.exists() || !gpgconf_info.isFile() ||
      !gpgconf_info.isExecutable()) {
    QMessageBox::critical(
        this, tr("Illegal GnuPG Path"),
        tr("Target GnuPG Path contains no \"gpgconf\" executable."));
    return false;
  }

  return true;
}

auto GnuPGControllerDialog::check_custom_gnupg_key_database_path(QString path)
    -> bool {
  if (path.isEmpty()) return false;

  QFileInfo dir_info(path);
  if (!dir_info.exists() || !dir_info.isReadable() || !dir_info.isDir()) {
    QMessageBox::critical(this, tr("Illegal GnuPG Key Database Path"),
                          tr("Target GnuPG Key Database Path is not an "
                             "exists readable directory."));
    return false;
  }

  return true;
}

}  // namespace GpgFrontend::UI
