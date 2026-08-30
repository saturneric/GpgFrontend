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

/// Channels are a fixed-size resource in the core, and every one of them holds
/// an engine process open. Spelled once, and counted across all three kinds.
constexpr int kMaxKeyDatabases = 8;

// gpg-agent keeps its sockets inside the key database folder, and a unix socket
// address is length-capped. A folder past that cap leaves the agent unable to
// create its socket, so GnuPG never works there -- and nothing in the resulting
// failure points back at the path that caused it. Caught while the path is
// still being chosen, rather than at the next start.
//
// Only for gnupg-backed databases: rPGP opens no sockets, so the same path is
// perfectly usable for it.
auto RejectsGnuPGSocketPath(QWidget* parent, const QString& backend_type,
                            const QString& path) -> bool {
  if (backend_type != "gnupg") return false;
  if (GnuPGHomePathFitsSocketBudget(path)) return false;

  QMessageBox::warning(
      parent, QObject::tr("Key Database Path Too Long"),
      QObject::tr("This folder's path is too long for GnuPG's agent socket, so "
                  "GnuPG could not start against it. Choose a folder with a "
                  "shorter path.") +
          "\n\n" +
          QString("%1 bytes, max %2")
              .arg(path.toUtf8().size())
              .arg(GnuPGHomePathByteBudget()));
  return true;
}

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
  font.setBold(active);
  item->setFont(font);

  // Inactive is the state worth spotting -- the database is configured but was
  // not opened -- so it is the one that reads differently, greyed rather than
  // shouted about.
  if (!active) {
    item->setForeground(QBrush(QColor(150, 150, 150)));
    item->setToolTip(QObject::tr(
        "GpgFrontend could not open this key database at the last start."));
  }

  return item;
}

// Both tables show the same thing about a different list, so they are set up
// the same way from one place.
void SetUpKeyDatabaseTable(QTableWidget* table) {
  table->clear();

  QStringList column_titles;
  column_titles << QObject::tr("Channel") << QObject::tr("Name")
                << QObject::tr("Engine") << QObject::tr("Status")
                << QObject::tr("Folder");
  table->setColumnCount(static_cast<int>(column_titles.size()));
  table->setHorizontalHeaderLabels(column_titles);

  table->setFocusPolicy(Qt::NoFocus);
  table->setAlternatingRowColors(true);

  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::SingleSelection);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);

  table->setShowGrid(false);
  table->setWordWrap(false);
  table->setMouseTracking(true);
  table->setSortingEnabled(false);

  // The channel is a column now, so the row numbers beside it would be a second
  // count of the same rows disagreeing with the first: a managed database sits
  // at channel 1 when the default one is on, and a header reading "1" next to a
  // channel reading "2" is worse than no header at all.
  table->verticalHeader()->setVisible(false);
  table->verticalHeader()->setDefaultSectionSize(32);

  // A folder path is longer than any column can be. Elided in the middle, which
  // keeps the two ends that identify it, with the whole thing on the tooltip --
  // rather than pushed off the side behind a horizontal scrollbar.
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->setTextElideMode(Qt::ElideMiddle);

  auto* header = table->horizontalHeader();
  header->setHighlightSections(false);
  header->setStretchLastSection(true);
  header->setMinimumSectionSize(60);

  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(4, QHeaderView::Stretch);

  // Headers sit over their values rather than centred above left-aligned text.
  // It only shows on the two wide columns, which are the two that read as
  // detached from their heading when it does not.
  for (const auto column : {1, 4}) {
    if (auto* head = table->horizontalHeaderItem(column); head != nullptr) {
      head->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
  }
}

}  // namespace

KeyDatabasesTab::KeyDatabasesTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyDatabasesSettings>()),
      app_path_(GlobalSettingStation::GetInstance().GetAppDir()),
      is_sandbox_(IsRunningInSandBox()) {
  ui_->setupUi(this);

  SetUpKeyDatabaseTable(ui_->keyDatabaseTable);
  SetUpKeyDatabaseTable(ui_->externalKeyDatabaseTable);

  popup_menu_ = new QMenu(this);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Up);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Down);
  popup_menu_->addAction(ui_->actionMove_Key_Database_To_Top);
  popup_menu_->addAction(ui_->actionOpen_Key_Database);
  if (!is_sandbox_) {
    popup_menu_->addAction(ui_->actionEdit_Key_Database);
  }
  popup_menu_->addAction(ui_->actionRemove_Selected_Key_Database);

  ui_->defaultKeyDatabaseCheckBox->setText(
      tr("Use this computer's default key database"));
  ui_->addNewKeyDatabaseButton->setText(tr("Add Key Database"));
  ui_->addExternalKeyDatabaseButton->setText(
      tr("Add Key Database On This Computer"));

  // Muted, but not italic: these are full sentences meant to be read, and an
  // italic paragraph is harder to read than a plain one.
  ui_->defaultKeyDatabaseHintLabel->setStyleSheet("color: palette(mid);");
  ui_->profileHintLabel->setStyleSheet("color: palette(mid);");
  ui_->externalHintLabel->setStyleSheet("color: palette(mid);");

  // A key database outside the profile cannot survive here at all: the sandbox
  // reads its list back off the fixed dbs/ directory on every start, so an
  // entry pointing anywhere else would be shown once and then quietly dropped.
  //
  // Hidden rather than removed: removeTab() leaves the page with no parent, and
  // a parentless QWidget is a top-level window waiting to be shown.
  if (is_sandbox_) {
    ui_->keyDatabaseTabs->setTabVisible(
        ui_->keyDatabaseTabs->indexOf(ui_->externalTab), false);
  }

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

  connect(ui_->addExternalKeyDatabaseButton, &QPushButton::clicked, this,
          &KeyDatabasesTab::slot_add_external_key_database);

  connect(ui_->defaultKeyDatabaseCheckBox, &QCheckBox::toggled, this,
          &KeyDatabasesTab::slot_toggle_default_key_database);

  SetSettings();
}

void KeyDatabasesTab::SetSettings() {
  active_key_db_infos_ = GetGpgKeyDatabaseInfos();

  default_db_info_.reset();
  managed_db_infos_.clear();
  external_db_infos_.clear();

  // Split by what each database is recorded as. The core settles that once, on
  // read, deriving it from the path only for an entry written before the field
  // existed -- so this screen never has to guess, and never has to agree with
  // the packer about how the guessing is done.
  for (const auto& info : GetAllKeyDatabaseInfoBySettings()) {
    switch (info.kind) {
      case KeyDatabaseKind::kDEFAULT:
        default_db_info_ = info;
        break;
      case KeyDatabaseKind::kMANAGED:
        managed_db_infos_.append(info);
        break;
      case KeyDatabaseKind::kEXTERNAL:
        external_db_infos_.append(info);
        break;
    }
  }

  loading_ = true;
  refresh_default_key_database_ui();
  loading_ = false;

  slot_refresh_key_database_table();
}

void KeyDatabasesTab::ApplySettings() {
  auto so = SettingsObject(kKeyDatabaseListObject);
  auto key_database_list = KeyDatabaseListSO(so);

  const auto to_items = [](const QContainer<KeyDatabaseInfo>& infos)
      -> QContainer<KeyDatabaseItemSO> {
    QContainer<KeyDatabaseItemSO> items;
    items.reserve(infos.size());
    for (const auto& info : infos) items.append(KeyDatabaseItemSO(info));
    return items;
  };

  // The order is the rule rather than the user's arrangement: DEFAULT owns
  // channel 0, the profile's own databases come next, and this computer's come
  // last. ComposeKeyDatabaseList() is what says so, and it numbers the
  // channels.
  std::optional<KeyDatabaseItemSO> default_item;
  if (default_db_info_) default_item = KeyDatabaseItemSO(*default_db_info_);

  key_database_list.key_databases = ComposeKeyDatabaseList(
      default_item, to_items(managed_db_infos_), to_items(external_db_infos_));

  // The stored list is protected from being overwritten when it could not be
  // read, which would otherwise make this -- the one screen that can repair a
  // broken profile -- silently do nothing. Ask before replacing it.
  if (so.LoadFailed()) {
    const auto ret = QMessageBox::warning(
        this, tr("Unreadable Key Database Settings"),
        tr("Your saved key database list exists but could not be read. This "
           "usually means it was written by another installation, or with a "
           "different application key.") +
            "\n\n" +
            tr("Saving now replaces it with the list shown here. The previous "
               "list cannot be recovered afterwards."),
        QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Cancel);

    if (ret != QMessageBox::Save) {
      LOG_I() << "user declined to replace the unreadable key database list";
      return;
    }

    so.StoreOverridingUnreadable(key_database_list.ToJson());
    return;
  }

  so.Store(key_database_list.ToJson());
}

void KeyDatabasesTab::contextMenuEvent(QContextMenuEvent* event) {
  if (current_table()->selectedItems().isEmpty()) return;

  popup_menu_->exec(event->globalPos());
  QWidget::contextMenuEvent(event);
}

auto KeyDatabasesTab::current_table() const -> QTableWidget* {
  return ui_->keyDatabaseTabs->currentWidget() == ui_->externalTab
             ? ui_->externalKeyDatabaseTable
             : ui_->keyDatabaseTable;
}

auto KeyDatabasesTab::current_list() -> QContainer<KeyDatabaseInfo>* {
  return ui_->keyDatabaseTabs->currentWidget() == ui_->externalTab
             ? &external_db_infos_
             : &managed_db_infos_;
}

auto KeyDatabasesTab::current_row() const -> int {
  const auto selected_rows = current_table()->selectionModel()->selectedRows();
  if (selected_rows.isEmpty()) return -1;
  return selected_rows.first().row();
}

auto KeyDatabasesTab::total_count() const -> int {
  return (default_db_info_ ? 1 : 0) +
         static_cast<int>(managed_db_infos_.size()) +
         static_cast<int>(external_db_infos_.size());
}

void KeyDatabasesTab::refresh_default_key_database_ui() {
  const auto candidate = DefaultKeyDatabaseCandidate();

  // A build whose engine names no key database has nothing to offer under this
  // checkbox, and saying so is more use than an unexplained empty row.
  if (candidate.path.isEmpty()) {
    ui_->defaultKeyDatabaseCheckBox->setChecked(false);
    ui_->defaultKeyDatabaseCheckBox->setEnabled(false);
    ui_->defaultKeyDatabasePathLabel->setVisible(false);
    ui_->defaultKeyDatabaseHintLabel->setText(
        tr("The OpenPGP engine on this computer does not report a key "
           "database, so there is no default one to use."));
    return;
  }

  ui_->defaultKeyDatabaseCheckBox->setChecked(default_db_info_.has_value());

  // In the sandbox the list is rebuilt from disk on every start with the
  // default database always first, so the checkbox would not survive being
  // turned off. Shown as it is, and not offered.
  ui_->defaultKeyDatabaseCheckBox->setEnabled(!is_sandbox_);

  // The identity, the folder and the engine on one line, in the order the
  // question is asked: which database is this, where does it live, what opens
  // it. Separated by middots rather than laid out in a grid, because three
  // short facts in a row do not need a grid.
  const auto native_path = QDir::toNativeSeparators(candidate.path);
  ui_->defaultKeyDatabasePathLabel->setVisible(true);
  ui_->defaultKeyDatabasePathLabel->setText(
      QString("%1  ·  %2  ·  %3")
          .arg(candidate.name, native_path, candidate.backend_type.toUpper()));
  ui_->defaultKeyDatabasePathLabel->setToolTip(native_path);

  // Greyed along with the checkbox when it cannot be changed, so the block
  // reads as one disabled thing rather than as a live path under a dead box.
  ui_->defaultKeyDatabasePathLabel->setEnabled(!is_sandbox_);

  ui_->defaultKeyDatabaseHintLabel->setText(
      is_sandbox_
          ? tr("Always available on this computer, and always channel 0.")
          : tr("Always channel 0. Never included in a profile package: "
               "whoever opens your profile elsewhere gets their own."));
}

void KeyDatabasesTab::fill_table(
    QTableWidget* table, const QContainer<KeyDatabaseInfo>& key_databases,
    int first_channel, int selected_row) {
  table->setUpdatesEnabled(false);
  table->clearContents();
  table->setRowCount(static_cast<int>(key_databases.size()));

  for (int index = 0; index < key_databases.size(); index++) {
    const auto& key_db = key_databases[index];

    // The channel this row will actually get, not its position in this table.
    // The two differ by whatever comes before it -- the default database, and
    // for the external list every managed one -- and the channel is the number
    // the rest of the program speaks in.
    auto* i_channel = CreateTableItem(QString::number(first_channel + index),
                                      Qt::AlignCenter);
    table->setItem(index, 0, i_channel);

    table->setItem(
        index, 1,
        CreateTableItem(key_db.name, Qt::AlignLeft | Qt::AlignVCenter));

    const auto backend_type_display =
        key_db.backend_type.isEmpty() ? QStringLiteral("GNUPG")
                                      : key_db.backend_type.toUpper().trimmed();
    table->setItem(index, 2,
                   CreateTableItem(backend_type_display, Qt::AlignCenter));

    const auto is_active =
        std::find_if(active_key_db_infos_.begin(), active_key_db_infos_.end(),
                     [&key_db](const KeyDatabaseInfo& i) -> bool {
                       return i.name == key_db.name;
                     }) != active_key_db_infos_.end();

    table->setItem(index, 3, CreateStatusItem(is_active));

    // One folder column instead of the stored path beside the resolved one.
    // They were the same directory written two ways, both cut off, and neither
    // readable. The short form is shown and the resolved one is the tooltip,
    // which is the order of interest: where it is in the profile first, where
    // it is on this disk when you ask.
    auto* i_folder =
        CreateTableItem(QDir::toNativeSeparators(key_db.origin_path),
                        Qt::AlignLeft | Qt::AlignVCenter);
    i_folder->setToolTip(
        key_db.path.isEmpty()
            ? tr("This folder could not be resolved on this computer.")
            : QDir::toNativeSeparators(key_db.path));
    table->setItem(index, 4, i_folder);
  }

  if (selected_row >= 0 && selected_row < table->rowCount()) {
    table->selectRow(selected_row);
  } else {
    table->clearSelection();
  }

  table->setUpdatesEnabled(true);
}

void KeyDatabasesTab::slot_refresh_key_database_table(int selected_row) {
  // Only the visible table's selection is restored; the other one is being
  // rebuilt behind a tab nobody is looking at.
  const auto external_visible =
      ui_->keyDatabaseTabs->currentWidget() == ui_->externalTab;

  // Where each list starts in the channel numbering, which is the order
  // ComposeKeyDatabaseList() lays down: the default one, then the profile's
  // own, then this computer's.
  const auto managed_first_channel = default_db_info_ ? 1 : 0;
  const auto external_first_channel =
      managed_first_channel + static_cast<int>(managed_db_infos_.size());

  fill_table(ui_->keyDatabaseTable, managed_db_infos_, managed_first_channel,
             external_visible ? -1 : selected_row);
  fill_table(ui_->externalKeyDatabaseTable, external_db_infos_,
             external_first_channel, external_visible ? selected_row : -1);

  // The hint doubles as the empty state. A blank table with a heading over it
  // says only that something is missing; this says what would go there.
  ui_->profileHintLabel->setText(
      managed_db_infos_.isEmpty()
          ? tr("No key databases of your own yet. One you add here is kept "
               "inside your profile and travels with it.")
          : tr("Kept inside your profile, and carried by a profile package."));

  ui_->externalHintLabel->setText(
      external_db_infos_.isEmpty()
          ? tr("Nothing here yet. A key database you add on this tab stays on "
               "this computer: a profile package never carries it, and it is "
               "always opened after the ones inside your profile.")
          : tr("Key databases outside your profile. They belong to this "
               "computer alone: a profile package never carries them, and "
               "they are always opened after the ones inside your profile."));
}

void KeyDatabasesTab::slot_open_key_database() {
  const auto row = current_row();
  auto* list = current_list();
  if (row < 0 || row >= list->size()) return;

  const auto& key_database = (*list)[row];

  LOG_D() << "try to open key db at path: " << key_database.path;
  QDesktopServices::openUrl(QUrl::fromLocalFile(key_database.path));
}

void KeyDatabasesTab::slot_move_up_key_database() {
  const auto row = current_row();
  if (row <= 0) return;

  current_list()->swapItemsAt(row, row - 1);

  slot_refresh_key_database_table(row - 1);

  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_move_to_top_key_database() {
  const auto row = current_row();
  if (row <= 0) return;

  auto* list = current_list();
  auto selected_item = list->takeAt(row);
  list->insert(0, selected_item);

  slot_refresh_key_database_table(0);

  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_move_down_key_database() {
  const auto row = current_row();
  auto* list = current_list();
  if (row < 0 || row >= list->size() - 1) return;

  list->swapItemsAt(row, row + 1);

  slot_refresh_key_database_table(row + 1);

  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_toggle_default_key_database(bool checked) {
  if (loading_) return;

  if (checked) {
    auto candidate = DefaultKeyDatabaseCandidate();
    if (candidate.path.isEmpty()) return;

    if (total_count() >= kMaxKeyDatabases) {
      QMessageBox::critical(
          this, tr("Maximum Key Database Limit Reached"),
          tr("Currently, GpgFrontend supports a maximum of %1 key databases. "
             "Please remove an existing database to add a new one.")
              .arg(kMaxKeyDatabases));
      loading_ = true;
      ui_->defaultKeyDatabaseCheckBox->setChecked(false);
      loading_ = false;
      return;
    }

    KeyDatabaseInfo info;
    info.name = candidate.name;
    info.backend_type = candidate.backend_type;
    info.origin_path = candidate.path;
    info.path = GetCanonicalKeyDatabasePath(app_path_, candidate.path);
    info.channel = 0;
    info.kind = KeyDatabaseKind::kDEFAULT;
    default_db_info_ = info;

    refresh_default_key_database_ui();
    slot_refresh_key_database_table();
    emit SignalDeepRestartNeeded();
    return;
  }

  // Turning it off has to leave something behind. An empty list is not a state
  // the program has: GetKeyDatabasesBySettings() puts the default one back, so
  // the checkbox would come back on by itself at the next start.
  if (total_count() <= 1) {
    QMessageBox::warning(
        this, tr("Key Database Required"),
        tr("GpgFrontend needs at least one key database. Add another one "
           "first, then turn this off."));
    loading_ = true;
    ui_->defaultKeyDatabaseCheckBox->setChecked(true);
    loading_ = false;
    return;
  }

  default_db_info_.reset();
  refresh_default_key_database_ui();
  slot_refresh_key_database_table();
  emit SignalDeepRestartNeeded();
}

void KeyDatabasesTab::slot_edit_key_database() {
  const auto row = current_row();
  auto* list = current_list();

  if (row < 0 || row >= list->size()) {
    QMessageBox::warning(this, tr("No Key Database Selected"),
                         tr("Please select a key database to edit."));
    return;
  }

  const auto managed = list == &managed_db_infos_;
  const auto mode = managed ? KeyDatabaseEditDialog::Mode::kRENAME_MANAGED
                            : KeyDatabaseEditDialog::Mode::kEDIT_EXTERNAL;

  auto* dialog = new KeyDatabaseEditDialog(mode, *list, row, this);

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this, row, managed](const QString& name, const QString& backend_type,
                               const QString& path) {
            auto* list = managed ? &managed_db_infos_ : &external_db_infos_;
            if (row < 0 || row >= list->size()) return;

            // Renaming a managed database moves its folder. The name and the
            // folder are one fact -- the sandbox reads the list back out of the
            // filesystem -- so an entry that renamed only itself would be
            // reverted without a word at the next start.
            if (managed) {
              const auto old_path = (*list)[row].path;
              const auto action = DecideManagedRename(
                  QFileInfo(old_path).isDir(), QFileInfo(path).exists());

              if (action == ManagedRenameAction::kTARGET_TAKEN) {
                QMessageBox::warning(
                    this, tr("Key Database Folder Already Exists"),
                    tr("A folder named after '%1' is already there. Its "
                       "contents are not ours to replace, so choose another "
                       "name.")
                        .arg(name));
                return;
              }

              if (action == ManagedRenameAction::kRENAME &&
                  !QDir().rename(old_path, path)) {
                QMessageBox::warning(
                    this, tr("Failed to Rename Key Database Folder"),
                    tr("GpgFrontend could not rename the folder this key "
                       "database is kept in. It may still be in use; try "
                       "again after restarting GpgFrontend."));
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

            if (!managed) {
              for (int i = 0; i < list->size(); i++) {
                if (i != row &&
                    QFileInfo((*list)[i].path) == QFileInfo(key_db_fs_path)) {
                  QMessageBox::warning(
                      this, tr("Duplicate Key Database Paths"),
                      tr("The edited key database path duplicates a previously "
                         "existing one."));
                  return;
                }
              }
            }

            if (RejectsGnuPGSocketPath(this, backend_type, key_db_fs_path)) {
              return;
            }

            LOG_D() << "edit key database path, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            auto& key_database = (*list)[row];
            key_database.name = name;
            key_database.backend_type = backend_type;
            key_database.path = key_db_fs_path;
            key_database.origin_path = path;
            key_database.kind = managed ? KeyDatabaseKind::kMANAGED
                                        : KeyDatabaseKind::kEXTERNAL;

            slot_refresh_key_database_table(row);

            emit SignalDeepRestartNeeded();
          });

  dialog->show();
}

void KeyDatabasesTab::slot_add_new_key_database() {
  if (total_count() >= kMaxKeyDatabases) {
    QMessageBox::critical(
        this, tr("Maximum Key Database Limit Reached"),
        tr("Currently, GpgFrontend supports a maximum of %1 key databases. "
           "Please remove an existing database to add a new one.")
            .arg(kMaxKeyDatabases));
    return;
  }

  auto* dialog = new KeyDatabaseEditDialog(
      KeyDatabaseEditDialog::Mode::kADD_MANAGED, managed_db_infos_, this);

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this](const QString& name, const QString& backend_type,
                 const QString& path) -> void {
            for (const auto& key_database : managed_db_infos_) {
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
                  tr("The specified key database path points to an existing "
                     "file. Please specify a path that does not exist or "
                     "points to a directory."));
              return;
            }

            if (!file_info.exists() &&
                !QDir(file_info.absoluteFilePath()).mkpath(".")) {
              QMessageBox::warning(
                  this, tr("Failed to Create Key Database Directory"),
                  tr("GpgFrontend failed to create a directory at the "
                     "specified key database path. Please check the path and "
                     "your permissions."));
              return;
            }

            auto key_db_fs_path =
                GpgFrontend::GetCanonicalKeyDatabasePath(app_path_, path);
            if (key_db_fs_path.isEmpty()) {
              QMessageBox::warning(this, tr("Invalid Key Database Paths"),
                                   tr("The edited key database path is not a "
                                      "valid path that GpgFrontend can use"));
              return;
            }

            if (RejectsGnuPGSocketPath(this, backend_type, key_db_fs_path)) {
              return;
            }

            LOG_D() << "new key database path, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            KeyDatabaseInfo key_database;
            key_database.name = name;
            key_database.backend_type = backend_type;
            key_database.path = key_db_fs_path;
            key_database.origin_path = path;
            key_database.kind = KeyDatabaseKind::kMANAGED;
            managed_db_infos_.append(key_database);

            slot_refresh_key_database_table();

            emit SignalDeepRestartNeeded();
          });
  dialog->show();
}

void KeyDatabasesTab::slot_add_external_key_database() {
  if (total_count() >= kMaxKeyDatabases) {
    QMessageBox::critical(
        this, tr("Maximum Key Database Limit Reached"),
        tr("Currently, GpgFrontend supports a maximum of %1 key databases. "
           "Please remove an existing database to add a new one.")
            .arg(kMaxKeyDatabases));
    return;
  }

  // The dialog checks the new name against the databases it is given, and a
  // name has to be unique across the whole profile rather than within one tab.
  auto known = managed_db_infos_;
  known.append(external_db_infos_);

  auto* dialog = new KeyDatabaseEditDialog(
      KeyDatabaseEditDialog::Mode::kADD_EXTERNAL, known, this);

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this](const QString& name, const QString& backend_type,
                 const QString& path) -> void {
            auto key_db_fs_path =
                GpgFrontend::GetCanonicalKeyDatabasePath(app_path_, path);
            if (key_db_fs_path.isEmpty()) {
              QMessageBox::warning(this, tr("Invalid Key Database Paths"),
                                   tr("The edited key database path is not a "
                                      "valid path that GpgFrontend can use"));
              return;
            }

            for (const auto& key_database : managed_db_infos_) {
              if (QFileInfo(key_database.path) == QFileInfo(key_db_fs_path)) {
                QMessageBox::warning(
                    this, tr("Duplicate Key Database Paths"),
                    tr("The newly added key database path duplicates a "
                       "previously existing one."));
                return;
              }
            }
            for (const auto& key_database : external_db_infos_) {
              if (QFileInfo(key_database.path) == QFileInfo(key_db_fs_path)) {
                QMessageBox::warning(
                    this, tr("Duplicate Key Database Paths"),
                    tr("The newly added key database path duplicates a "
                       "previously existing one."));
                return;
              }
            }

            if (RejectsGnuPGSocketPath(this, backend_type, key_db_fs_path)) {
              return;
            }

            LOG_D() << "new external key database, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            KeyDatabaseInfo key_database;
            key_database.name = name;
            key_database.backend_type = backend_type;
            key_database.path = key_db_fs_path;
            key_database.origin_path = path;
            // Recorded rather than inferred later: this is the user saying the
            // database belongs to this computer, and it is what keeps a package
            // from carrying it even if it happens to sit under the profile.
            key_database.kind = KeyDatabaseKind::kEXTERNAL;
            external_db_infos_.append(key_database);

            slot_refresh_key_database_table();

            emit SignalDeepRestartNeeded();
          });
  dialog->show();
}

void KeyDatabasesTab::slot_remove_existing_key_database() {
  const auto row = current_row();
  auto* list = current_list();
  if (row < 0 || row >= list->size()) return;

  // Same rule as turning the default one off: something has to be left, or the
  // next start puts the default one back and the removal was theatre.
  if (total_count() <= 1) {
    QMessageBox::warning(
        this, tr("Key Database Required"),
        tr("GpgFrontend needs at least one key database, so the last one "
           "cannot be removed."));
    return;
  }

  QMessageBox::StandardButton reply =
      QMessageBox::question(this, tr("Confirm Deletion"),
                            tr("Are you sure you want to remove the selected "
                               "key database from the list?"),
                            QMessageBox::Yes | QMessageBox::No);

  if (reply != QMessageBox::Yes) return;

  list->removeAt(row);

  const int next_selected_row =
      list->isEmpty() ? -1 : std::min(row, static_cast<int>(list->size() - 1));

  slot_refresh_key_database_table(next_selected_row);

  emit SignalDeepRestartNeeded();
}

}  // namespace GpgFrontend::UI
