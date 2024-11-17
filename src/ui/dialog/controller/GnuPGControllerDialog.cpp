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

#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/GeneralDialog.h"
#include "ui/dialog/KeyDatabaseEditDialog.h"

//
#include "ui_GnuPGControllerDialog.h"

namespace GpgFrontend::UI {

GnuPGControllerDialog::GnuPGControllerDialog(QWidget* parent)
    : GeneralDialog("GnuPGControllerDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GnuPGControllerDialog>()) {
  ui_->setupUi(this);

  ui_->tab->setWindowTitle(tr("General"));
  ui_->tab_2->setWindowTitle(tr("Key Database"));
  ui_->tab_3->setWindowTitle(tr("Advanced"));

  ui_->asciiModeCheckBox->setText(tr("Use Binary Mode for File Operations"));
  ui_->usePinentryAsPasswordInputDialogCheckBox->setText(
      tr("Use Pinentry as Password Input Dialog"));
  ui_->gpgmeDebugLogCheckBox->setText(tr("Enable GpgME Debug Log"));
  ui_->useCustomGnuPGInstallPathCheckBox->setText(tr("Use Custom GnuPG"));
  ui_->useCustomGnuPGInstallPathButton->setText(tr("Select GnuPG Path"));
  ui_->restartGpgAgentOnStartCheckBox->setText(
      tr("Restart Gpg Agent on start"));
  ui_->killAllGnuPGDaemonCheckBox->setText(
      tr("Kill all gnupg daemon at close"));

  // tips
  ui_->customGnuPGPathTipsLabel->setText(
      tr("Tips: please select a directory where \"gpgconf\" is located in."));
  ui_->restartTipsLabel->setText(
      tr("Tips: notice that modify any of these settings will cause an "
         "Application restart."));

  popup_menu_ = new QMenu(this);
  popup_menu_->addAction(ui_->actionOpen_Key_Database);
  popup_menu_->addAction(ui_->actionRemove_Selected_Key_Database);

  // announce main window
  connect(this, &GnuPGControllerDialog::SignalRestartNeeded,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRestartApplication);

  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this, [=](int state) {
            ui_->useCustomGnuPGInstallPathButton->setDisabled(
                state != Qt::CheckState::Checked);
          });

  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this,
          &GnuPGControllerDialog::slot_update_custom_gnupg_install_path_label);

  connect(
      ui_->useCustomGnuPGInstallPathButton, &QPushButton::clicked, this, [=]() {
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

  connect(ui_->useCustomGnuPGInstallPathCheckBox, &QCheckBox::stateChanged,
          this, [=](int) {
            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
          });

  connect(ui_->addNewKeyDatabaseButton, &QPushButton::clicked, this,
          &GnuPGControllerDialog::slot_add_new_key_database);

  connect(ui_->actionRemove_Selected_Key_Database, &QAction::triggered, this,
          &GnuPGControllerDialog::slot_remove_existing_key_database);

  connect(ui_->actionOpen_Key_Database, &QAction::triggered, this,
          &GnuPGControllerDialog::slot_open_key_database);

#if defined(__APPLE__) && defined(__MACH__)
  // macOS style settings
  ui_->buttonBox->setDisabled(true);
  ui_->buttonBox->setHidden(true);

  connect(this, &QDialog::finished, this, &GnuPGControllerDialog::SlotAccept);
  connect(this, &QDialog::finished, this, &GnuPGControllerDialog::deleteLater);
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
            GlobalSettingStation::GetInstance()
                .GetSettings()
                .value("gnupg/custom_gnupg_install_path")
                .toString();
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
  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  auto non_ascii_at_file_operation =
      settings.value("gnupg/non_ascii_at_file_operation", true).toBool();
  if (non_ascii_at_file_operation) {
    ui_->asciiModeCheckBox->setCheckState(Qt::Checked);
  }

  auto enable_gpgme_debug_log =
      settings.value("gnupg/enable_gpgme_debug_log", false).toBool();
  if (enable_gpgme_debug_log) {
    ui_->gpgmeDebugLogCheckBox->setCheckState(Qt::Checked);
  }

  auto kill_all_gnupg_daemon_at_close =
      settings.value("gnupg/kill_all_gnupg_daemon_at_close", false).toBool();
  if (kill_all_gnupg_daemon_at_close) {
    ui_->killAllGnuPGDaemonCheckBox->setCheckState(Qt::Checked);
  }

  auto use_custom_gnupg_install_path =
      settings.value("gnupg/use_custom_gnupg_install_path", false).toBool();
  if (use_custom_gnupg_install_path) {
    ui_->useCustomGnuPGInstallPathCheckBox->setCheckState(Qt::Checked);
  }

  auto use_pinentry_as_password_input_dialog =
      settings
          .value("gnupg/use_pinentry_as_password_input_dialog",
                 QString::fromLocal8Bit(qgetenv("container")) != "flatpak")
          .toBool();
  if (use_pinentry_as_password_input_dialog) {
    ui_->usePinentryAsPasswordInputDialogCheckBox->setCheckState(Qt::Checked);
  }

  auto restart_gpg_agent_on_start =
      settings.value("gnupg/restart_gpg_agent_on_start", false).toBool();
  if (restart_gpg_agent_on_start) {
    ui_->restartGpgAgentOnStartCheckBox->setCheckState(Qt::Checked);
  }

  this->slot_update_custom_gnupg_install_path_label(
      use_custom_gnupg_install_path ? Qt::Checked : Qt::Unchecked);

  this->slot_set_restart_needed(kNonRestartCode);

  auto key_database_list =
      KeyDatabaseListSO(SettingsObject("key_database_list"));
  buffered_key_db_so_ = key_database_list.key_databases;

  this->slot_refresh_key_database_table();
}

void GnuPGControllerDialog::apply_settings() {
  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

  settings.setValue("gnupg/non_ascii_at_file_operation",
                    ui_->asciiModeCheckBox->isChecked());
  settings.setValue("gnupg/use_custom_gnupg_install_path",
                    ui_->useCustomGnuPGInstallPathCheckBox->isChecked());
  settings.setValue("gnupg/use_pinentry_as_password_input_dialog",
                    ui_->usePinentryAsPasswordInputDialogCheckBox->isChecked());
  settings.setValue("gnupg/enable_gpgme_debug_log",
                    ui_->gpgmeDebugLogCheckBox->isChecked());
  settings.setValue("gnupg/custom_gnupg_install_path",
                    ui_->currentCustomGnuPGInstallPathLabel->text());
  settings.setValue("gnupg/restart_gpg_agent_on_start",
                    ui_->restartGpgAgentOnStartCheckBox->isChecked());
  settings.setValue("gnupg/kill_all_gnupg_daemon_at_close",
                    ui_->killAllGnuPGDaemonCheckBox->isChecked());

  auto so = SettingsObject("key_database_list");
  auto key_database_list = KeyDatabaseListSO(so);
  key_database_list.key_databases = buffered_key_db_so_;
  so.Store(key_database_list.ToJson());
}

auto GnuPGControllerDialog::get_restart_needed() const -> int {
  return this->restart_mode_;
}

void GnuPGControllerDialog::slot_set_restart_needed(int mode) {
  this->restart_mode_ = mode;
}

auto GnuPGControllerDialog::check_custom_gnupg_path(QString path) -> bool {
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
#ifdef __MINGW32__
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

void GnuPGControllerDialog::slot_add_new_key_database() {
  auto* dialog = new KeyDatabaseEditDialog(this);
  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this](const QString& name, const QString& path) {
            auto& key_databases = buffered_key_db_so_;
            for (const auto& key_database : key_databases) {
              if (QFileInfo(key_database.path) == QFileInfo(path)) {
                QMessageBox::warning(
                    this, tr("Duplicate Key Database Paths"),
                    tr("The newly added key database path duplicates a "
                       "previously existing one."));
                return;
              }
            }

            LOG_D() << "new key database path, name: " << name
                    << "path: " << path;

            KeyDatabaseItemSO key_database;
            key_database.name = name;
            key_database.path = path;
            key_databases.append(key_database);

            // refresh ui
            slot_refresh_key_database_table();

            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
          });
  dialog->show();
}

void GnuPGControllerDialog::slot_refresh_key_database_table() {
  auto key_databases = buffered_key_db_so_;
  ui_->keyDatabaseTable->setRowCount(static_cast<int>(key_databases.size()));

  int index = 0;
  for (const auto& key_database : key_databases) {
    LOG_D() << "key database table item index: " << index
            << "name: " << key_database.name << "path: " << key_database.path;

    auto* i_name = new QTableWidgetItem(key_database.name);
    i_name->setTextAlignment(Qt::AlignCenter);
    ui_->keyDatabaseTable->setItem(index, 0, i_name);

    ui_->keyDatabaseTable->setItem(index, 1,
                                   new QTableWidgetItem(key_database.path));

    index++;
  }
  ui_->keyDatabaseTable->resizeColumnsToContents();
}

void GnuPGControllerDialog::contextMenuEvent(QContextMenuEvent* event) {
  QDialog::contextMenuEvent(event);
  if (ui_->keyDatabaseTable->selectedItems().length() > 0) {
    popup_menu_->exec(event->globalPos());
  }
}

void GnuPGControllerDialog::slot_remove_existing_key_database() {
  const auto row_size = ui_->keyDatabaseTable->rowCount();

  auto& key_databases = buffered_key_db_so_;
  for (int i = 0; i < row_size; i++) {
    auto* const item = ui_->keyDatabaseTable->item(i, 1);
    if (!item->isSelected()) continue;
    key_databases.remove(i);
    break;
  }

  this->slot_refresh_key_database_table();

  // announce the restart
  this->slot_set_restart_needed(kDeepRestartCode);
}

void GnuPGControllerDialog::slot_open_key_database() {
  const auto row_size = ui_->keyDatabaseTable->rowCount();

  auto& key_databases = buffered_key_db_so_;
  for (int i = 0; i < row_size; i++) {
    auto* const item = ui_->keyDatabaseTable->item(i, 1);
    if (!item->isSelected()) continue;
    LOG_D() << "try to open key db at path: " << key_databases[i].path;
    QDesktopServices::openUrl(QUrl::fromLocalFile(key_databases[i].path));
    break;
  }
}
}  // namespace GpgFrontend::UI
