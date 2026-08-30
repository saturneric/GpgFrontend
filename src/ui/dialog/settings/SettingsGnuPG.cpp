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
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui_GnuPGSettings.h"

namespace GpgFrontend::UI {

GnuPGTab::GnuPGTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GnuPGSettings>()) {
  ui_->setupUi(this);

  // The forms under the repo-root "ui/" directory are not scanned by lupdate,
  // so every visible string has to be set from C++ to be translatable.
  ui_->generalGroupBox->setTitle(tr("General"));
  ui_->advancedGroupBox->setTitle(tr("Advanced"));

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

  // Sits right after "Terminate GnuPG background processes on exit": both
  // describe what GpgFrontend does to GnuPG when it closes.
  clear_password_cache_at_close_check_ =
      new QCheckBox(tr("Clear password cache on exit"), ui_->generalGroupBox);
  clear_password_cache_at_close_check_->setToolTip(
      tr("Ask gpg-agent to forget all cached passphrases when GpgFrontend "
         "closes."));
  ui_->verticalLayout_7->insertWidget(2, clear_password_cache_at_close_check_);

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

  // ---- maintenance -------------------------------------------------------
  // These act immediately, so they neither announce a restart nor take part in
  // SetSettings()/ApplySettings().
  auto* maintenance_box = new QGroupBox(tr("Maintenance"), this);
  auto* maintenance_layout = new QVBoxLayout(maintenance_box);

  auto* maintenance_note = new QLabel(
      tr("These operations take effect immediately and are not undone by "
         "cancelling this dialog. Restarting components briefly interrupts any "
         "in-flight GnuPG operation."),
      maintenance_box);
  maintenance_note->setWordWrap(true);

  auto* clear_cache_button =
      new QPushButton(tr("Clear Password Cache"), maintenance_box);
  clear_cache_button->setToolTip(tr("Clear Password Cache of GnuPG"));
  connect(clear_cache_button, &QPushButton::clicked, this, [this]() {
    run_advanced_operation(
        [](GpgAdvancedOperator& o) { return o.ClearGpgPasswordCache(); },
        tr("Clear password cache successfully"),
        tr("Failed to clear password cache of GnuPG"));
  });

  auto* reload_button =
      new QPushButton(tr("Reload Components"), maintenance_box);
  reload_button->setToolTip(tr("Reload All GnuPG's Components"));
  connect(reload_button, &QPushButton::clicked, this, [this]() {
    run_advanced_operation(
        [](GpgAdvancedOperator& o) { return o.ReloadAllGpgComponents(); },
        tr("Reload all the GnuPG's components successfully"),
        tr("Failed to reload all or one of the GnuPG's component(s)"));
  });

  auto* restart_button =
      new QPushButton(tr("Restart Components"), maintenance_box);
  restart_button->setToolTip(tr("Restart All GnuPG's Components"));
  connect(restart_button, &QPushButton::clicked, this, [this]() {
    // The disruptive one: it tears down every running GnuPG daemon.
    const auto reply = QMessageBox::question(
        this, tr("Confirm"),
        tr("Are you sure you want to restart all of GnuPG's components?\nAny "
           "GnuPG operation still running will be interrupted."),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    run_advanced_operation(
        [](GpgAdvancedOperator& o) { return o.RestartGpgComponents(); },
        tr("Restart all the GnuPG's components successfully"),
        tr("Failed to restart all or one of the GnuPG's component(s)"));
  });

  auto* button_row = new QVBoxLayout();
  button_row->setContentsMargins(0, 0, 0, 0);
  button_row->addStretch(1);
  button_row->addWidget(clear_cache_button);
  button_row->addWidget(reload_button);
  button_row->addWidget(restart_button);

  maintenance_layout->addWidget(maintenance_note);
  maintenance_layout->addLayout(button_row);

  // before the trailing vertical spacer
  ui_->verticalLayout_2->insertWidget(2, maintenance_box);

  // ---- home directory too long for the agent sockets ---------------------
  // Only shown when it actually happened. gpg-agent puts its sockets inside the
  // key database folder, and once that path is too long for a unix socket
  // address the agent exits without creating them -- GnuPG then simply does not
  // work, with nothing anywhere to say why. Inserted above the maintenance box
  // because none of those buttons can fix it.
  const auto home_path_detail = GpgContext::FirstUnusableHomeReason();
  if (!home_path_detail.isEmpty()) {
    auto* warning_box = new QGroupBox(tr("GnuPG Unavailable"), this);
    auto* warning_layout = new QVBoxLayout(warning_box);

    auto text =
        tr("GnuPG cannot start: this key database's folder path is too long "
           "for GnuPG's agent socket. Choose a key database in a shorter path "
           "under Settings, Key Databases.");

    // Untranslated on purpose: the measured length and the platform limit are
    // what a maintainer needs read back verbatim from a bug report.
    text += "\n\n" + home_path_detail;

    auto* warning_label = new QLabel(text, warning_box);
    warning_label->setWordWrap(true);
    warning_label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    warning_layout->addWidget(warning_label);
    ui_->verticalLayout_2->insertWidget(2, warning_box);
  }

  // These were previously reachable only through the "Advanced" menu, which is
  // hidden in sandbox; keep them unreachable there.
  if (IsRunningInSandBox()) {
    maintenance_box->setHidden(true);
    clear_password_cache_at_close_check_->setHidden(true);
  }

  SetSettings();
}

void GnuPGTab::run_advanced_operation(
    const std::function<bool(GpgAdvancedOperator&)>& op,
    const QString& success_text, const QString& failure_text) {
  bool ret = true;
  for (const auto& channel : OpenPGPContext::GetAllChannelId()) {
    // these operations are GnuPG-only; skip non-GnuPG channels (e.g. rPGP)
    if (OpenPGPContext::GetInstance(channel).Engine() !=
        OpenPGPEngine::kGNUPG) {
      continue;
    }
    ret = op(GpgAdvancedOperator::GetInstance(channel));
    if (!ret) break;
  }

  if (ret) {
    QMessageBox::information(this, tr("Successful Operation"), success_text);
  } else {
    QMessageBox::critical(this, tr("Failed Operation"), failure_text);
  }
}

void GnuPGTab::SetSettings() {
  QSignalBlocker blocker1(ui_->gpgmeDebugLogCheckBox);
  QSignalBlocker blocker2(ui_->killAllGnuPGDaemonCheckBox);
  QSignalBlocker blocker3(ui_->useCustomGnuPGInstallPathCheckBox);
  QSignalBlocker blocker4(clear_password_cache_at_close_check_);

  auto settings = GetSettings();

  clear_password_cache_at_close_check_->setChecked(
      settings.value("basic/clear_gpg_password_cache", true).toBool());

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
  settings.setValue("basic/clear_gpg_password_cache",
                    clear_password_cache_at_close_check_->isChecked());

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