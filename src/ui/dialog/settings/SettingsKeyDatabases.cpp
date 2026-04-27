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

namespace {

auto CreateTableItem(const QString& text,
                     Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignLeft)
    -> QTableWidgetItem* {
  auto* item = new QTableWidgetItem(text);
  item->setTextAlignment(alignment);
  item->setToolTip(text);
  return item;
}

auto CreateStatusItem(bool active) -> QTableWidgetItem* {
  auto* item = new QTableWidgetItem(active ? QObject::tr("Active")
                                           : QObject::tr("Inactive"));
  item->setTextAlignment(Qt::AlignCenter);

  QFont font = item->font();
  font.setBold(true);
  item->setFont(font);

  return item;
}

}  // namespace

KeyDatabasesTab::KeyDatabasesTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabasesSettings>()),
      app_path_(GlobalSettingStation::GetInstance().GetAppDir()),
      is_sandbox_(IsRunningInSandBox()) {
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

  auto* table = ui_->keyDatabaseTable;

  table->setShowGrid(false);
  table->setWordWrap(false);
  table->setMouseTracking(true);
  table->setSortingEnabled(false);

  table->verticalHeader()->setVisible(true);
  table->verticalHeader()->setDefaultSectionSize(34);

  auto* header = table->horizontalHeader();
  header->setHighlightSections(false);
  header->setStretchLastSection(true);
  header->setMinimumSectionSize(90);

  table->setColumnWidth(0, 160);  // Name
  table->setColumnWidth(1, 120);  // Backend Type
  table->setColumnWidth(2, 100);  // Status
  table->setColumnWidth(3, 260);  // Path
  table->setColumnWidth(4, 320);  // Real Path

  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(3, QHeaderView::Stretch);
  header->setSectionResizeMode(4, QHeaderView::Stretch);

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

void KeyDatabasesTab::slot_refresh_key_database_table(int selected_row) {
  auto& key_databases = key_db_infos_;

  auto* table = ui_->keyDatabaseTable;
  table->setUpdatesEnabled(false);
  table->clearContents();
  table->setRowCount(static_cast<int>(key_databases.size()));

  for (int index = 0; index < key_databases.size(); index++) {
    const auto& key_db = key_databases[index];

    LOG_D() << "key database table item index: " << index
            << "name: " << key_db.name << "path: " << key_db.path;

    table->setVerticalHeaderItem(
        index, new QTableWidgetItem(QString::number(index + 1)));

    auto* i_name = CreateTableItem(key_db.name, Qt::AlignCenter);
    table->setItem(index, 0, i_name);

    auto backend_type_display = key_db.backend_type.isEmpty()
                                    ? QStringLiteral("GNUPG")
                                    : key_db.backend_type.toUpper().trimmed();

    auto* i_backend_type =
        CreateTableItem(backend_type_display, Qt::AlignCenter);
    table->setItem(index, 1, i_backend_type);

    const auto is_active =
        std::find_if(active_key_db_infos_.begin(), active_key_db_infos_.end(),
                     [&key_db](const KeyDatabaseInfo& i) -> bool {
                       return i.name == key_db.name;
                     }) != active_key_db_infos_.end();

    table->setItem(index, 2, CreateStatusItem(is_active));

    auto* i_origin_path = CreateTableItem(key_db.origin_path);
    table->setItem(index, 3, i_origin_path);

    const auto real_path = is_active ? key_db.path : tr("N/A");
    auto* i_real_path = CreateTableItem(real_path);
    if (!is_active) {
      i_real_path->setForeground(QBrush(QColor(150, 150, 150)));
    }
    table->setItem(index, 4, i_real_path);
  }

  if (selected_row >= 0 && selected_row < table->rowCount()) {
    table->selectRow(selected_row);
  } else {
    table->clearSelection();
  }

  table->setUpdatesEnabled(true);
}

void KeyDatabasesTab::slot_open_key_database() {
  const auto selected_rows =
      ui_->keyDatabaseTable->selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) return;

  const int row = selected_rows.first().row();
  if (row < 0 || row >= key_db_infos_.size()) return;

  const auto& key_database = key_db_infos_[row];

  LOG_D() << "try to open key db at path: " << key_database.path;
  QDesktopServices::openUrl(QUrl::fromLocalFile(key_database.path));
}

void KeyDatabasesTab::slot_move_up_key_database() {
  const auto selected_rows =
      ui_->keyDatabaseTable->selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) return;

  const int row = selected_rows.first().row();
  if (row <= 0) return;

  key_db_infos_.swapItemsAt(row, row - 1);

  slot_refresh_key_database_table(row - 1);

  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_move_to_top_key_database() {
  const auto selected_rows =
      ui_->keyDatabaseTable->selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) return;

  const int row = selected_rows.first().row();
  if (row <= 0) return;

  auto selected_item = key_db_infos_.takeAt(row);
  key_db_infos_.insert(0, selected_item);

  slot_refresh_key_database_table(0);

  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_move_down_key_database() {
  const auto selected_rows =
      ui_->keyDatabaseTable->selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) return;

  const int row = selected_rows.first().row();
  if (row < 0 || row >= key_db_infos_.size() - 1) return;

  key_db_infos_.swapItemsAt(row, row + 1);

  slot_refresh_key_database_table(row + 1);

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
  if (key_db_infos_.size() >= 8) {
    QMessageBox::critical(
        this, tr("Maximum Key Database Limit Reached"),
        tr("Currently, GpgFrontend supports a maximum of 8 key databases. "
           "Please remove an existing database to add a new one."));
    return;
  }

  auto* dialog = new KeyDatabaseEditDialog(key_db_infos_, this);

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
  const auto selected_rows =
      ui_->keyDatabaseTable->selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) return;

  const auto row = selected_rows.first().row();
  if (row < 0 || row >= key_db_infos_.size()) return;

  QMessageBox::StandardButton reply =
      QMessageBox::question(this, tr("Confirm Deletion"),
                            tr("Are you sure you want to remove the selected "
                               "key database from the list?"),
                            QMessageBox::Yes | QMessageBox::No);

  if (reply != QMessageBox::Yes) return;

  key_db_infos_.removeAt(row);

  const int next_selected_row =
      key_db_infos_.isEmpty()
          ? -1
          : std::min(row, static_cast<int>(key_db_infos_.size() - 1));

  slot_refresh_key_database_table(next_selected_row);

  emit SignalDeepRestartNeeded();
}

}  // namespace GpgFrontend::UI
