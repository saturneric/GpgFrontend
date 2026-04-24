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

#include "GnuPGControllerDialog.h"

#include "core/function/GlobalSettingStation.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/GeneralDialog.h"
#include "ui/dialog/KeyDatabaseEditDialog.h"

//
#include "ui_GnuPGControllerDialog.h"

namespace GpgFrontend::UI {

GnuPGControllerDialog::GnuPGControllerDialog(QWidget* parent)
    : GeneralDialog("GnuPGControllerDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GnuPGControllerDialog>()),
      app_path_(GlobalSettingStation::GetInstance().GetAppDir()) {
  ui_->setupUi(this);

  ui_->gpgmeDebugLogCheckBox->setText(tr("Enable GpgME Debug Log"));
  ui_->useCustomGnuPGInstallPathCheckBox->setText(tr("Use Custom GnuPG"));
  ui_->useCustomGnuPGInstallPathButton->setText(tr("Select GnuPG Path"));
  ui_->killAllGnuPGDaemonCheckBox->setText(
      tr("Kill all gnupg daemon at close"));

  // tips
  ui_->customGnuPGPathTipsLabel->setText(
      tr("Tips: please select a directory where \"gpgconf\" is located in."));
  ui_->restartTipsLabel->setText(
      tr("Tips: notice that modify any of these settings will cause an "
         "Application restart."));

  ui_->tabWidget->setTabText(0, tr("General"));
  ui_->tabWidget->setTabText(1, tr("Key Database"));
  ui_->tabWidget->setTabText(2, tr("Advanced"));

  // announce main window
  connect(this, &GnuPGControllerDialog::SignalRestartNeeded,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRestartApplication);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(
      ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::checkStateChanged,
      this, [=](Qt::CheckState state) {
        ui_->useCustomGnuPGInstallPathButton->setDisabled(state != Qt::Checked);
      });
#else
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this, [=](int state) -> void {
            ui_->useCustomGnuPGInstallPathButton->setDisabled(
                state != Qt::CheckState::Checked);
          });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::checkStateChanged,
          this,
          &GnuPGControllerDialog::slot_update_custom_gnupg_install_path_label);
#else
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this,
          &GnuPGControllerDialog::slot_update_custom_gnupg_install_path_label);
#endif

  connect(
      ui_->useCustomGnuPGInstallPathButton, &QPushButton::clicked, this,
      [=]() -> void {
        QString selected_custom_gnupg_install_path =
            QFileDialog::getExistingDirectory(
                this, tr("Open Directory"), {},
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        custom_gnupg_path_ = selected_custom_gnupg_install_path;

        // announce the restart
        this->slot_set_restart_needed(kDeepRestartCode);

        // update ui
        this->slot_update_custom_gnupg_install_path_label(
            this->ui_->useCustomGnuPGInstallPathCheckBox->checkState());
      });

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->gpgmeDebugLogCheckBox, &QCheckBox::checkStateChanged, this,
          [=](Qt::CheckState) {
            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
          });
#else
  connect(ui_->gpgmeDebugLogCheckBox, &QCheckBox::stateChanged, this, [=](int) {
    // announce the restart
    this->slot_set_restart_needed(kDeepRestartCode);
  });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::checkStateChanged,
          this, [=](Qt::CheckState) {
            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
          });
#else
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this, [=](int) {
            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
          });
#endif

#ifdef Q_OS_MACOS
  // macOS style settings
  ui_->buttonBox->setDisabled(true);
  ui_->buttonBox->setHidden(true);

  connect(this, &QDialog::finished, this, &GnuPGControllerDialog::SlotAccept);
#else
  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this,
          &GnuPGControllerDialog::SlotAccept);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this,
          &GnuPGControllerDialog::reject);
#endif

  setWindowTitle(tr("GnuPG Controller"));
  set_settings();
}

void GnuPGControllerDialog::SlotAccept() {
  apply_settings();

  if (get_restart_needed() != 0) {
    emit SignalRestartNeeded(get_restart_needed());
  }
  close();
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
            GetSettings().value("gnupg/custom_gnupg_install_path").toString();
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
    auto dir = QFileInfo(gnupg_path).path();
    ui_->currentCustomGnuPGInstallPathLabel->setText(dir);
  }
}

void GnuPGControllerDialog::set_settings() {
  auto settings = GetSettings();

  auto enable_gpgme_debug_log =
      settings.value("gnupg/enable_gpgme_debug_log", false).toBool();
  if (enable_gpgme_debug_log) {
    ui_->gpgmeDebugLogCheckBox->setCheckState(Qt::Checked);
  }

  auto kill_all_gnupg_daemon_at_close =
      settings.value("gnupg/kill_all_gnupg_daemon_at_close", true).toBool();
  if (kill_all_gnupg_daemon_at_close) {
    ui_->killAllGnuPGDaemonCheckBox->setCheckState(Qt::Checked);
  }

  auto use_custom_gnupg_install_path =
      settings.value("gnupg/use_custom_gnupg_install_path", false).toBool();
  if (use_custom_gnupg_install_path) {
    ui_->useCustomGnuPGInstallPathCheckBox->setCheckState(Qt::Checked);
  }

  this->slot_update_custom_gnupg_install_path_label(
      use_custom_gnupg_install_path ? Qt::Checked : Qt::Unchecked);

  this->slot_set_restart_needed(kNonRestartCode);
}

void GnuPGControllerDialog::apply_settings() {
  auto settings = GpgFrontend::GetSettings();

  settings.setValue("gnupg/use_custom_gnupg_install_path",
                    ui_->useCustomGnuPGInstallPathCheckBox->isChecked());
  settings.setValue("gnupg/enable_gpgme_debug_log",
                    ui_->gpgmeDebugLogCheckBox->isChecked());
  settings.setValue("gnupg/custom_gnupg_install_path",
                    ui_->currentCustomGnuPGInstallPathLabel->text());
  settings.setValue("gnupg/kill_all_gnupg_daemon_at_close",
                    ui_->killAllGnuPGDaemonCheckBox->isChecked());
}

auto GnuPGControllerDialog::get_restart_needed() const -> int {
  return this->restart_mode_;
}

void GnuPGControllerDialog::slot_set_restart_needed(int mode) {
  this->restart_mode_ = mode;
}

auto GnuPGControllerDialog::check_custom_gnupg_path(const QString& path)
    -> bool {
  if (path.isEmpty()) return false;

  QFileInfo const dir_info(path);
  if (!dir_info.exists() || !dir_info.isReadable() || !dir_info.isDir()) {
    QMessageBox::critical(
        this, tr("Illegal GnuPG Path"),
        tr("Target GnuPG Path is not an exists readable directory."));
    return false;
  }

  QDir const dir(path);
  if (!dir.isAbsolute()) {
    QMessageBox::critical(this, tr("Illegal GnuPG Path"),
                          tr("Target GnuPG Path is not an absolute path."));
  }

#ifdef Q_OS_WINDOWS
  QFileInfo const gpgconf_info(path + "/gpgconf.exe");
#else
  QFileInfo const gpgconf_info(path + "/gpgconf");
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

}  // namespace GpgFrontend::UI
