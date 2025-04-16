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

  ui_->tabWidget->setTabText(0, tr("General"));
  ui_->tabWidget->setTabText(1, tr("Key Database"));
  ui_->tabWidget->setTabText(2, tr("Advanced"));

  ui_->keyDatabaseTable->clear();

  QStringList column_titles;
  column_titles << tr("Name") << tr("Status") << tr("Path") << tr("Real Path");
  ui_->keyDatabaseTable->setColumnCount(static_cast<int>(column_titles.size()));
  ui_->keyDatabaseTable->setHorizontalHeaderLabels(column_titles);

  popup_menu_ = new QMenu(this);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Up);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Down);
  popup_menu_->addAction(ui_->actionMove_Key_Database_To_Top);
  popup_menu_->addAction(ui_->actionOpen_Key_Database);
  popup_menu_->addAction(ui_->actionEdit_Key_Database);
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

  connect(ui_->actionMove_Key_Database_Up, &QAction::triggered, this,
          &GnuPGControllerDialog::slot_move_up_key_database);

  connect(ui_->actionMove_Key_Database_To_Top, &QAction::triggered, this,
          &GnuPGControllerDialog::slot_move_to_top_key_database);

  connect(ui_->actionMove_Key_Database_Down, &QAction::triggered, this,
          &GnuPGControllerDialog::slot_move_down_key_database);

  connect(ui_->actionEdit_Key_Database, &QAction::triggered, this,
          &GnuPGControllerDialog::slot_edit_key_database);

#if defined(__APPLE__) && defined(__MACH__)
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

  key_db_infos_ = GetKeyDatabaseInfoBySettings();
  active_key_db_infos_ = GetGpgKeyDatabaseInfos();

  this->slot_refresh_key_database_table();
}

void GnuPGControllerDialog::apply_settings() {
  auto settings = GpgFrontend::GetSettings();

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
  key_database_list.key_databases.clear();

  int index = 0;
  for (auto& key_db_info : key_db_infos_) {
    key_db_info.channel = index++;
    key_database_list.key_databases.append(KeyDatabaseItemSO(key_db_info));
  }
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
  auto* dialog = new KeyDatabaseEditDialog(key_db_infos_, this);

  if (key_db_infos_.size() >= 8) {
    QMessageBox::critical(
        this, tr("Maximum Key Database Limit Reached"),
        tr("Currently, GpgFrontend supports a maximum of 8 key databases. "
           "Please remove an existing database to add a new one."));
    return;
  }

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this](const QString& name, const QString& path) {
            auto& key_databases = key_db_infos_;
            for (const auto& key_database : key_databases) {
              if (QFileInfo(key_database.path) == QFileInfo(path)) {
                QMessageBox::warning(
                    this, tr("Duplicate Key Database Paths"),
                    tr("The newly added key database path duplicates a "
                       "previously existing one."));
                return;
              }
            }

            auto key_db_fs_path =
                GpgFrontend::GetCanonicalKeyDatabasePath(app_path_, path);
            if (key_db_fs_path.isEmpty()) {
              QMessageBox::warning(this, tr("Invalid Key Database Paths"),
                                   tr("The edited key database path is not a "
                                      "valid path that GpgFrontend can use"));
              return;
            }

            LOG_D() << "new key database path, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            KeyDatabaseInfo key_database;
            key_database.name = name;
            key_database.path = key_db_fs_path;
            key_database.origin_path = path;
            key_database.channel = static_cast<int>(key_databases.size());
            key_databases.append(key_database);

            // refresh ui
            slot_refresh_key_database_table();

            // announce the restart
            this->slot_set_restart_needed(kDeepRestartCode);
          });
  dialog->show();
}

void GnuPGControllerDialog::slot_refresh_key_database_table() {
  auto& key_databases = key_db_infos_;
  ui_->keyDatabaseTable->setRowCount(static_cast<int>(key_databases.size()));

  int index = 0;
  for (const auto& key_db : key_databases) {
    LOG_D() << "key database table item index: " << index
            << "name: " << key_db.name << "path: " << key_db.path;

    auto* i_name = new QTableWidgetItem(key_db.name);
    i_name->setTextAlignment(Qt::AlignCenter);

    ui_->keyDatabaseTable->setVerticalHeaderItem(
        index, new QTableWidgetItem(QString::number(index + 1)));
    ui_->keyDatabaseTable->setItem(index, 0, i_name);

    auto is_active =
        std::find_if(active_key_db_infos_.begin(), active_key_db_infos_.end(),
                     [key_db](const KeyDatabaseInfo& i) {
                       return i.name == key_db.name;
                     }) != active_key_db_infos_.end();
    ui_->keyDatabaseTable->setItem(
        index, 1,
        new QTableWidgetItem(is_active ? tr("Active") : tr("Inactive")));

    ui_->keyDatabaseTable->setItem(index, 2,
                                   new QTableWidgetItem(key_db.origin_path));

    ui_->keyDatabaseTable->setItem(index, 3, new QTableWidgetItem(key_db.path));

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

  auto& key_databases = key_db_infos_;
  for (int i = 0; i < row_size; i++) {
    auto* const item = ui_->keyDatabaseTable->item(i, 1);
    if (!item->isSelected()) continue;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Confirm Deletion"),
        tr("Are you sure you want to delete the selected key database?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
      return;
    }

    key_databases.removeAt(i);
    break;
  }

  this->slot_refresh_key_database_table();

  // announce the restart
  this->slot_set_restart_needed(kDeepRestartCode);
}

void GnuPGControllerDialog::slot_open_key_database() {
  const auto row_size = ui_->keyDatabaseTable->rowCount();

  auto& key_databases = key_db_infos_;
  for (int i = 0; i < row_size; i++) {
    auto* const item = ui_->keyDatabaseTable->item(i, 1);
    if (!item->isSelected()) continue;
    LOG_D() << "try to open key db at path: " << key_databases[i].path;
    QDesktopServices::openUrl(QUrl::fromLocalFile(key_databases[i].path));
    break;
  }
}

void GnuPGControllerDialog::slot_move_up_key_database() {
  const auto row_size = ui_->keyDatabaseTable->rowCount();

  if (row_size <= 0) return;

  auto& key_databases = key_db_infos_;

  for (int i = 0; i < row_size; i++) {
    auto* const item = ui_->keyDatabaseTable->item(i, 1);
    if (!item->isSelected()) continue;

    if (i == 0) {
      return;
    }

    key_databases.swapItemsAt(i, i - 1);

    for (int k = 0; k < ui_->keyDatabaseTable->columnCount(); k++) {
      ui_->keyDatabaseTable->item(i, k)->setSelected(false);
      ui_->keyDatabaseTable->item(i - 1, k)->setSelected(true);
    }

    break;
  }

  this->slot_refresh_key_database_table();

  this->slot_set_restart_needed(kDeepRestartCode);
}

void GnuPGControllerDialog::slot_move_to_top_key_database() {
  const auto row_size = ui_->keyDatabaseTable->rowCount();

  if (row_size <= 0) return;

  auto& key_databases = key_db_infos_;

  for (int i = 0; i < row_size; i++) {
    auto* const item = ui_->keyDatabaseTable->item(i, 1);
    if (!item->isSelected()) continue;

    if (i == 0) {
      return;
    }

    auto selected_item = key_databases.takeAt(i);
    key_databases.insert(0, selected_item);

    for (int k = 0; k < ui_->keyDatabaseTable->columnCount(); k++) {
      ui_->keyDatabaseTable->item(i, k)->setSelected(false);
    }
    for (int k = 0; k < ui_->keyDatabaseTable->columnCount(); k++) {
      ui_->keyDatabaseTable->item(0, k)->setSelected(true);
    }

    break;
  }

  this->slot_refresh_key_database_table();

  this->slot_set_restart_needed(kDeepRestartCode);
}

void GnuPGControllerDialog::slot_move_down_key_database() {
  const auto row_size = ui_->keyDatabaseTable->rowCount();

  if (row_size <= 0) return;

  auto& key_databases = key_db_infos_;

  for (int i = row_size - 1; i >= 0; i--) {
    auto* const item = ui_->keyDatabaseTable->item(i, 1);
    if (!item->isSelected()) continue;

    if (i == row_size - 1) {
      return;
    }

    key_databases.swapItemsAt(i, i + 1);

    for (int k = 0; k < ui_->keyDatabaseTable->columnCount(); k++) {
      ui_->keyDatabaseTable->item(i, k)->setSelected(false);
      ui_->keyDatabaseTable->item(i + 1, k)->setSelected(true);
    }

    break;
  }

  this->slot_refresh_key_database_table();

  this->slot_set_restart_needed(kDeepRestartCode);
}

void GnuPGControllerDialog::slot_edit_key_database() {
  const auto row_size = ui_->keyDatabaseTable->rowCount();
  if (row_size <= 0) {
    return;
  }

  int selected_row = -1;
  for (int i = 0; i < row_size; i++) {
    if (ui_->keyDatabaseTable->item(i, 0)->isSelected()) {
      selected_row = i;
      break;
    }
  }

  if (selected_row == -1) {
    QMessageBox::warning(this, tr("No Key Database Selected"),
                         tr("Please select a key database to edit."));
    return;
  }

  auto& key_databases = key_db_infos_;
  KeyDatabaseInfo& selected_key_database = key_databases[selected_row];
  auto* dialog = new KeyDatabaseEditDialog(key_databases, this);
  dialog->SetDefaultName(selected_key_database.name);
  dialog->SetDefaultPath(selected_key_database.path);

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this, selected_row, selected_key_database](const QString& name,
                                                      const QString& path) {
            auto& all_key_databases = key_db_infos_;

            if (selected_key_database.path != path) {
              for (int i = 0; i < all_key_databases.size(); i++) {
                if (i != selected_row &&
                    QFileInfo(all_key_databases[i].path) == QFileInfo(path)) {
                  QMessageBox::warning(
                      this, tr("Duplicate Key Database Paths"),
                      tr("The edited key database path duplicates a previously "
                         "existing one."));
                  return;
                }
              }
            }

            auto key_db_fs_path =
                GpgFrontend::GetCanonicalKeyDatabasePath(app_path_, path);
            if (key_db_fs_path.isEmpty()) {
              QMessageBox::warning(this, tr("Invalid Key Database Paths"),
                                   tr("The edited key database path is not a "
                                      "valid path that GpgFrontend can use"));
              return;
            }

            LOG_D() << "edit key database path, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            KeyDatabaseInfo& key_database = key_db_infos_[selected_row];
            key_database.name = name;
            key_database.path = key_db_fs_path;
            key_database.origin_path = path;

            slot_refresh_key_database_table();

            this->slot_set_restart_needed(kDeepRestartCode);
          });

  dialog->show();
}
}  // namespace GpgFrontend::UI
