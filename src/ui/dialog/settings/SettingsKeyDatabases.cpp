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

#include "SettingsKeyDatabases.h"

#include "core/function/GlobalSettingStation.h"
#include "core/model/SettingsObject.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/dialog/KeyDatabaseEditDialog.h"

//
#include "ui_KeyDatabasesSettings.h"

namespace GpgFrontend::UI {

KeyDatabasesTab::KeyDatabasesTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabasesSettings>()),
      app_path_(GlobalSettingStation::GetInstance().GetAppDir()),
      is_sandbox_(IsFlatpakENV() || IsRunningInAppSandbox()) {
  ui_->setupUi(this);

  ui_->keyDatabaseTable->clear();

  QStringList column_titles;
  column_titles << tr("Name") << tr("Backend Type") << tr("Status")
                << tr("Path") << tr("Real Path");
  ui_->keyDatabaseTable->setColumnCount(static_cast<int>(column_titles.size()));
  ui_->keyDatabaseTable->setHorizontalHeaderLabels(column_titles);

  // no focus (rectangle around tableitems)
  // may be it should focus on whole row
  ui_->keyDatabaseTable->setFocusPolicy(Qt::NoFocus);
  ui_->keyDatabaseTable->setAlternatingRowColors(true);

  ui_->keyDatabaseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui_->keyDatabaseTable->setSelectionMode(QAbstractItemView::SingleSelection);
  ui_->keyDatabaseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

  popup_menu_ = new QMenu(this);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Up);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Down);
  popup_menu_->addAction(ui_->actionMove_Key_Database_To_Top);
  popup_menu_->addAction(ui_->actionOpen_Key_Database);
  if (!is_sandbox_) {
    popup_menu_->addAction(ui_->actionEdit_Key_Database);
  }
  popup_menu_->addAction(ui_->actionRemove_Selected_Key_Database);

  connect(ui_->actionRemove_Selected_Key_Database, &QAction::triggered, this,
          &KeyDatabasesTab::slot_remove_existing_key_database);

  connect(ui_->actionOpen_Key_Database, &QAction::triggered, this,
          &KeyDatabasesTab::slot_open_key_database);

  connect(ui_->actionMove_Key_Database_Up, &QAction::triggered, this,
          &KeyDatabasesTab::slot_move_up_key_database);

  connect(ui_->actionMove_Key_Database_To_Top, &QAction::triggered, this,
          &KeyDatabasesTab::slot_move_to_top_key_database);

  connect(ui_->actionMove_Key_Database_Down, &QAction::triggered, this,
          &KeyDatabasesTab::slot_move_down_key_database);

  connect(ui_->actionEdit_Key_Database, &QAction::triggered, this,
          &KeyDatabasesTab::slot_edit_key_database);

  connect(ui_->addNewKeyDatabaseButton, &QPushButton::clicked, this,
          &KeyDatabasesTab::slot_add_new_key_database);

  SetSettings();
}

void KeyDatabasesTab::SetSettings() {
  auto settings = GetSettings();

  key_db_infos_ = GetAllKeyDatabaseInfoBySettings();
  active_key_db_infos_ = GetGpgKeyDatabaseInfos();

  this->slot_refresh_key_database_table();
}

void KeyDatabasesTab::ApplySettings() {
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

void KeyDatabasesTab::contextMenuEvent(QContextMenuEvent* event) {
  if (ui_->keyDatabaseTable->selectedItems().isEmpty()) return;

  popup_menu_->exec(event->globalPos());
  QWidget::contextMenuEvent(event);
}

void KeyDatabasesTab::slot_refresh_key_database_table() {
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

    auto backend_type_display = key_db.backend_type.isEmpty()
                                    ? "GNUPG"
                                    : key_db.backend_type.toUpper().trimmed();
    auto* i_backend_type = new QTableWidgetItem(backend_type_display);
    i_backend_type->setTextAlignment(Qt::AlignCenter);
    ui_->keyDatabaseTable->setItem(index, 1, i_backend_type);

    auto is_active =
        std::find_if(active_key_db_infos_.begin(), active_key_db_infos_.end(),
                     [key_db](const KeyDatabaseInfo& i) -> bool {
                       return i.name == key_db.name;
                     }) != active_key_db_infos_.end();
    ui_->keyDatabaseTable->setItem(
        index, 2,
        new QTableWidgetItem(is_active ? tr("Active") : tr("Inactive")));

    ui_->keyDatabaseTable->setItem(index, 3,
                                   new QTableWidgetItem(key_db.origin_path));

    ui_->keyDatabaseTable->setItem(
        index, 4, new QTableWidgetItem(is_active ? key_db.path : tr("N/A")));
    index++;
  }
  ui_->keyDatabaseTable->resizeColumnsToContents();
}

void KeyDatabasesTab::slot_open_key_database() {
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

void KeyDatabasesTab::slot_move_up_key_database() {
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

  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_move_to_top_key_database() {
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

  emit SignalDeepRestartNeeded();
}
void KeyDatabasesTab::slot_move_down_key_database() {
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

  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_edit_key_database() {
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
  auto* dialog = new KeyDatabaseEditDialog(key_databases, selected_row, this);

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this, selected_row, selected_key_database](
              const QString& name, const QString& backend_type,
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
            key_database.backend_type = backend_type;
            key_database.path = key_db_fs_path;
            key_database.origin_path = path;

            slot_refresh_key_database_table();

            emit SignalDeepRestartNeeded();
          });

  dialog->show();
}

void KeyDatabasesTab::slot_add_new_key_database() {
  auto* dialog = new KeyDatabaseEditDialog(key_db_infos_, this);

  if (key_db_infos_.size() >= 8) {
    QMessageBox::critical(
        this, tr("Maximum Key Database Limit Reached"),
        tr("Currently, GpgFrontend supports a maximum of 8 key databases. "
           "Please remove an existing database to add a new one."));
    return;
  }

  connect(
      dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
      [this](const QString& name, const QString& backend_type,
             const QString& path) -> void {
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

        QFileInfo file_info(path);
        if (file_info.exists() && !file_info.isDir()) {
          QMessageBox::warning(
              this, tr("Invalid Key Database Path"),
              tr("The specified key database path points to an existing file. "
                 "Please specify a path that does not exist or points to a "
                 "directory."));
          return;
        }

        // if not exist, try to create an empty directory at the path.
        if (!file_info.exists()) {
          QDir dir;
          if (!QDir(file_info.absoluteFilePath()).mkpath(".")) {
            QMessageBox::warning(
                this, tr("Failed to Create Key Database Directory"),
                tr("GpgFrontend failed to create a directory at the specified "
                   "key database path. Please check the path and your "
                   "permissions."));
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

        LOG_D() << "new key database path, name: " << name << "path: " << path
                << "canonical path: " << key_db_fs_path;

        KeyDatabaseInfo key_database;
        key_database.name = name;
        key_database.backend_type = backend_type;
        key_database.path = key_db_fs_path;
        key_database.origin_path = path;
        key_database.channel = static_cast<int>(key_databases.size());
        key_databases.append(key_database);

        // refresh ui
        slot_refresh_key_database_table();

        emit SignalDeepRestartNeeded();
      });
  dialog->show();
}

void KeyDatabasesTab::slot_remove_existing_key_database() {
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

  emit SignalDeepRestartNeeded();
}

}  // namespace GpgFrontend::UI
