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

#include "SettingsGnuPG.h"

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "ui_GnuPGSettings.h"

namespace GpgFrontend::UI {

GnuPGTab::GnuPGTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GnuPGSettings>()) {
  ui_->setupUi(this);

  ui_->gpgmeDebugLogCheckBox->setText(tr("Enable GpgME Debug Log"));
  ui_->gpgmeDebugLogCheckBox->setToolTip(
      tr("Enable verbose GpgME logs for troubleshooting. This may include "
         "technical details about GnuPG operations."));

  ui_->useCustomGnuPGInstallPathCheckBox->setText(tr("Use Custom GnuPG"));
  ui_->useCustomGnuPGInstallPathButton->setText(tr("Select GnuPG Path"));
  ui_->killAllGnuPGDaemonCheckBox->setText(
      tr("Terminate GnuPG background processes on exit"));
  ui_->killAllGnuPGDaemonCheckBox->setToolTip(
      tr("This may affect other applications that are using GnuPG."));
  ui_->forbidALLGnuPGNetworkConnectionCheckBox->setText(
      tr("Forbid all GnuPG network connection."));

  // tips
  ui_->customGnuPGPathTipsLabel->setText(
      tr("Select the directory that contains the \"gpgconf\" executable."));

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
          this, &GnuPGTab::slot_update_custom_gnupg_install_path_label);
#else
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this, &GnuPGTab::slot_update_custom_gnupg_install_path_label);
#endif

  connect(
      ui_->useCustomGnuPGInstallPathButton, &QPushButton::clicked, this,
      [this]() -> void {
        const auto selected_path = QFileDialog::getExistingDirectory(
            this, tr("Open Directory"), custom_gnupg_path_,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (selected_path.isEmpty()) {
          return;
        }

        if (!check_custom_gnupg_path(selected_path)) {
          return;
        }

        if (custom_gnupg_path_ == selected_path) {
          return;
        }

        custom_gnupg_path_ = selected_path;
        ui_->currentCustomGnuPGInstallPathLabel->setText(custom_gnupg_path_);

        emit SignalDeepRestartNeeded();

        slot_update_custom_gnupg_install_path_label(
            ui_->useCustomGnuPGInstallPathCheckBox->checkState());
      });

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->gpgmeDebugLogCheckBox, &QCheckBox::checkStateChanged, this,
          [=](Qt::CheckState) {
            // announce the restart
            SignalDeepRestartNeeded();
          });
#else
  connect(ui_->gpgmeDebugLogCheckBox, &QCheckBox::stateChanged, this, [=](int) {
    // announce the restart
    SignalDeepRestartNeeded();
  });
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::checkStateChanged,
          this, [=](Qt::CheckState) {
            // announce the restart
            SignalDeepRestartNeeded();
          });
#else
  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this, [=](int) {
            // announce the restart
            SignalDeepRestartNeeded();
          });
#endif

  SetSettings();
}

void GnuPGTab::SetSettings() {
  QSignalBlocker blocker1(ui_->gpgmeDebugLogCheckBox);
  QSignalBlocker blocker2(ui_->killAllGnuPGDaemonCheckBox);
  QSignalBlocker blocker3(ui_->useCustomGnuPGInstallPathCheckBox);

  auto settings = GetSettings();

  ui_->gpgmeDebugLogCheckBox->setChecked(
      settings.value("gnupg/enable_gpgme_debug_log", false).toBool());

  ui_->killAllGnuPGDaemonCheckBox->setChecked(
      settings.value("gnupg/kill_all_gnupg_daemon_at_close", true).toBool());

  auto forbid_all_gnupg_connection =
      settings.value("network/forbid_all_gnupg_connection").toBool();
  ui_->forbidALLGnuPGNetworkConnectionCheckBox->setCheckState(
      forbid_all_gnupg_connection ? Qt::Checked : Qt::Unchecked);

  const auto use_custom_gnupg_install_path =
      settings.value("gnupg/use_custom_gnupg_install_path", false).toBool();

  ui_->useCustomGnuPGInstallPathCheckBox->setChecked(
      use_custom_gnupg_install_path);

  custom_gnupg_path_ =
      settings.value("gnupg/custom_gnupg_install_path").toString();

  slot_update_custom_gnupg_install_path_label(
      use_custom_gnupg_install_path ? Qt::Checked : Qt::Unchecked);
}

void GnuPGTab::ApplySettings() {
  auto settings = GpgFrontend::GetSettings();

  const auto use_custom = ui_->useCustomGnuPGInstallPathCheckBox->isChecked();

  settings.setValue("gnupg/use_custom_gnupg_install_path", use_custom);
  settings.setValue("gnupg/enable_gpgme_debug_log",
                    ui_->gpgmeDebugLogCheckBox->isChecked());
  settings.setValue("gnupg/kill_all_gnupg_daemon_at_close",
                    ui_->killAllGnuPGDaemonCheckBox->isChecked());
  settings.setValue("network/forbid_all_gnupg_connection",
                    ui_->forbidALLGnuPGNetworkConnectionCheckBox->isChecked());

  if (use_custom) {
    settings.setValue("gnupg/custom_gnupg_install_path", custom_gnupg_path_);
  }
}

void GnuPGTab::slot_update_custom_gnupg_install_path_label(int state) {
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

auto GnuPGTab::check_custom_gnupg_path(const QString& path) -> bool {
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
    return false;
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