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
#include "core/function/SettingCatalog.h"
#include "core/function/gpg/GnuPGHome.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/model/SettingsObject.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/KeyDatabaseEditDialog.h"
#include "ui/function/KeyDatabaseDisplayNames.h"
#include "ui/function/UIStyle.h"

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
// Asked of the resolver rather than measured here, so this cannot refuse a
// folder the engine would have rescued with a short link. Measuring it
// separately is exactly how the two came to disagree.
//
// Only for gnupg-backed databases: rPGP opens no sockets, so the same path is
// perfectly usable for it.
auto RejectsGnuPGSocketPath(QWidget* parent, const QString& backend_type,
                            const QString& path) -> bool {
  if (backend_type != "gnupg") return false;

  const auto home = GnuPGHomeResolver().Inspect(path);
  if (home.IsUsable()) return false;

  QMessageBox::warning(
      parent, QObject::tr("Key Database Path Too Long"),
      QObject::tr("This folder's path is too long for GnuPG's agent socket, "
                  "and no short enough link to it could be made, so GnuPG "
                  "could not start against it. Choose a folder with a shorter "
                  "path.") +
          "\n\n" + home.unusable_reason);
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

  // Told apart by weight alone. A colour would have to be picked against a
  // background the theme owns, and every role that looks like it means "dim
  // text" fails on one theme or the other -- which is how the hints on this
  // page came to be invisible on Windows dark mode.
  if (!active) {
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
  // The role has a column of its own: which database is the default changes
  // without the database changing, so neither its name nor its status may say
  // it.
  column_titles << QObject::tr("ID") << QObject::tr("Name")
                << QObject::tr("Engine") << QObject::tr("Status")
                << QObject::tr("Role") << QObject::tr("Folder");
  table->setColumnCount(static_cast<int>(column_titles.size()));
  table->setHorizontalHeaderLabels(column_titles);

  // Reachable from the keyboard: the row actions, Use as Default among them,
  // follow the selection, and the selection has to be movable without a mouse.
  table->setFocusPolicy(Qt::StrongFocus);
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
  header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(5, QHeaderView::Stretch);

  // Headers sit over their values rather than centred above left-aligned text.
  // It only shows on the two wide columns, which are the two that read as
  // detached from their heading when it does not.
  for (const auto column : {1, 5}) {
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

  const auto role_help =
      tr("Used when an operation does not explicitly select a key database.");

  use_as_default_action_ = new QAction(tr("Use as Default"), this);
  use_as_default_action_->setToolTip(role_help);
  copy_folder_path_action_ = new QAction(tr("Copy Folder Path"), this);

  ui_->actionMove_Key_Database_Up->setText(tr("Move Up"));
  ui_->actionMove_Key_Database_Up->setToolTip(
      tr("Move this key database up in the list"));
  ui_->actionMove_Key_Database_Down->setText(tr("Move Down"));
  ui_->actionMove_Key_Database_Down->setToolTip(
      tr("Move this key database down in the list"));
  ui_->actionMove_Key_Database_To_Top->setText(tr("Move to Top"));
  ui_->actionMove_Key_Database_To_Top->setToolTip(
      tr("Move this key database to the top of the list"));
  ui_->actionOpen_Key_Database->setText(tr("Open"));
  ui_->actionOpen_Key_Database->setToolTip(
      tr("Open this key database's folder in the file manager"));
  ui_->actionEdit_Key_Database->setText(tr("Edit"));
  ui_->actionEdit_Key_Database->setToolTip(tr("Edit this key database"));
  ui_->actionRemove_Selected_Key_Database->setText(tr("Remove"));
  ui_->actionRemove_Selected_Key_Database->setToolTip(
      tr("Remove this key database from the list"));

  popup_menu_ = new QMenu(this);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Up);
  popup_menu_->addAction(ui_->actionMove_Key_Database_Down);
  popup_menu_->addAction(ui_->actionMove_Key_Database_To_Top);
  popup_menu_->addSeparator();
  popup_menu_->addAction(use_as_default_action_);
  popup_menu_->addSeparator();
  popup_menu_->addAction(ui_->actionOpen_Key_Database);
  popup_menu_->addAction(copy_folder_path_action_);
  if (!is_sandbox_) {
    popup_menu_->addAction(ui_->actionEdit_Key_Database);
  }
  popup_menu_->addAction(ui_->actionRemove_Selected_Key_Database);

  // The built-in database's section. The engine selector is the one setting
  // that speaks for it, so it lives here rather than on the General page.
  // Turned on and off by the box's own title checkbox, which also greys
  // everything inside while it is off.
  ui_->builtInKeyDatabaseBox->setTitle(tr("Built-in Key Database"));
  // Short: the section's title already says whose engine it is.
  ui_->builtInEngineLabel->setText(tr("Key engine:"));
  ui_->builtInEngineLabel->setBuddy(ui_->builtInEngineComboBox);
  for (const auto& engine : GetGSS().AllSupportedEngines()) {
    ui_->builtInEngineComboBox->addItem(engine, engine.toUpper());
  }
  ui_->builtInEngineComboBox->setAccessibleName(
      tr("Key engine for the built-in database"));
  // Said on hover rather than under the selector: the section stays three
  // lines, and the label already says whose engine this is.
  const auto engine_help =
      tr("Applies only to the built-in database. Other databases always open "
         "with their own engine.");
  ui_->builtInEngineComboBox->setToolTip(engine_help);
  ui_->builtInEngineComboBox->setAccessibleDescription(engine_help);
  ui_->builtInLocationLabel->setText(tr("Location:"));

  // Read-only but selectable: a long path scrolls inside the field instead
  // of widening the page, and can be selected or copied from the keyboard.
  ui_->builtInLocationEdit->setAccessibleName(
      tr("Built-in key database location"));
  ui_->builtInLocationLabel->setBuddy(ui_->builtInLocationEdit);
  ui_->builtInCopyPathButton->setText(tr("Copy"));
  ui_->builtInCopyPathButton->setToolTip(tr("Copy the full path"));
  ui_->builtInCopyPathButton->setAccessibleName(
      tr("Copy the built-in key database location"));

  // "Default" in words and in weight, so it never depends on colour alone.
  auto marker_font = ui_->builtInDefaultMarkerLabel->font();
  marker_font.setBold(true);
  ui_->builtInDefaultMarkerLabel->setFont(marker_font);
  ui_->builtInDefaultMarkerLabel->setText(tr("Default"));
  ui_->builtInDefaultMarkerLabel->setToolTip(role_help);
  ui_->builtInDefaultMarkerLabel->setAccessibleDescription(role_help);
  ui_->builtInUseAsDefaultButton->setText(tr("Use as Default"));
  ui_->builtInUseAsDefaultButton->setToolTip(role_help);
  ui_->builtInUseAsDefaultButton->setAccessibleName(
      tr("Use the built-in key database as default"));
  ui_->builtInUseAsDefaultButton->setAccessibleDescription(role_help);

  ui_->keyDatabaseTabs->setTabText(
      ui_->keyDatabaseTabs->indexOf(ui_->profileTab), tr("In Profile"));
  ui_->keyDatabaseTabs->setTabText(
      ui_->keyDatabaseTabs->indexOf(ui_->externalTab), tr("Local Only"));

  ui_->addNewKeyDatabaseButton->setText(tr("Add Key Database"));
  ui_->addExternalKeyDatabaseButton->setText(
      tr("Add Key Database On This Computer"));
  for (auto* button :
       {ui_->profileUseAsDefaultButton, ui_->externalUseAsDefaultButton}) {
    button->setText(tr("Use as Default"));
    button->setToolTip(role_help);
    button->setAccessibleName(tr("Use the selected key database as default"));
  }

  ui_->smartCardNoticeLabel->setText(
      tr("Smart cards stay with the previous default database until "
         "GpgFrontend restarts. To move them sooner, apply this change, then "
         "use \"Restart All Gpg-Agents\" in the Smart Card Controller."));
  ui_->smartCardNoticeLabel->setVisible(false);

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

  connect(ui_->builtInKeyDatabaseBox, &QGroupBox::toggled, this,
          &KeyDatabasesTab::slot_toggle_default_key_database);

  connect(ui_->builtInEngineComboBox, &QComboBox::currentIndexChanged, this,
          [this]() {
            if (loading_) return;
            model_.engine =
                ui_->builtInEngineComboBox->currentData().toString();
            refresh_default_key_database_ui();
            // Takes effect when the key databases are opened again, which
            // applying does without a restart.
            emit SignalRuntimeApplyNeeded();
          });

  connect(ui_->builtInCopyPathButton, &QToolButton::clicked, this, [this]() {
    QGuiApplication::clipboard()->setText(ui_->builtInLocationEdit->text());
  });

  connect(
      ui_->builtInUseAsDefaultButton, &QPushButton::clicked, this,
      [this]() { use_as_default(QLatin1StringView(kDefaultKeyDatabaseId)); });
  connect(ui_->profileUseAsDefaultButton, &QPushButton::clicked, this,
          [this]() { use_as_default(selected_id()); });
  connect(ui_->externalUseAsDefaultButton, &QPushButton::clicked, this,
          [this]() { use_as_default(selected_id()); });
  connect(use_as_default_action_, &QAction::triggered, this,
          [this]() { use_as_default(selected_id()); });

  connect(copy_folder_path_action_, &QAction::triggered, this, [this]() {
    const auto row = current_row();
    auto* list = current_list();
    if (row < 0 || row >= list->size()) return;
    QGuiApplication::clipboard()->setText(
        QDir::toNativeSeparators((*list)[row].path));
  });

  // Whatever moves the selection -- mouse, arrow keys, a refill -- and
  // whichever tab is showing, the controls follow it.
  for (auto* table : {ui_->keyDatabaseTable, ui_->externalKeyDatabaseTable}) {
    connect(table, &QTableWidget::itemSelectionChanged, this,
            &KeyDatabasesTab::refresh_role_controls);
  }
  connect(ui_->keyDatabaseTabs, &QTabWidget::currentChanged, this,
          &KeyDatabasesTab::refresh_role_controls);

  SetSettings();

  // The Active column says which databases the Runtime in use opened, and a
  // new one may have opened others.
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRuntimeChanged, this, [this]() {
            active_key_db_infos_ = GetGpgKeyDatabaseInfos();
            slot_refresh_key_database_table(current_row());
          });
}

void KeyDatabasesTab::SetSettings() {
  active_key_db_infos_ = GetGpgKeyDatabaseInfos();
  pending_moves_.clear();

  // Split by what each database is recorded as. The core settles that once, on
  // read, deriving it from the path only for an entry written before the field
  // existed -- so this screen never has to guess, and never has to agree with
  // the packer about how the guessing is done.
  const auto all = GetAllKeyDatabaseInfoBySettings();
  model_.Load(all,
              KeyDatabaseListSO(SettingsObject(kKeyDatabaseListObject))
                  .default_database_id,
              SettingItems::kDefaultEngine.Read(GetSettings()));

  loading_ = true;
  const auto engine_index = ui_->builtInEngineComboBox->findData(model_.engine);
  ui_->builtInEngineComboBox->setCurrentIndex(std::max(0, engine_index));
  model_.engine = ui_->builtInEngineComboBox->currentData().toString();
  refresh_default_key_database_ui();
  loading_ = false;

  slot_refresh_key_database_table();
}

void KeyDatabasesTab::ApplySettings() {
  auto so = SettingsObject(kKeyDatabaseListObject);
  auto key_database_list = KeyDatabaseListSO(so);

  // The order is the rule rather than the user's arrangement: the built-in
  // database owns channel 0, the profile's own databases come next, and this
  // computer's come last (ComposeKeyDatabaseList). The default choice goes
  // with it, on either store path below.
  model_.ApplyTo(key_database_list);

  // Written here because the selector lives here: it says how the built-in
  // database is opened, and nothing else.
  {
    auto settings = GetSettings();
    SettingItems::kDefaultEngine.Write(settings, model_.engine);
  }

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

auto KeyDatabasesTab::current_list() -> QList<KeyDatabaseInfo>* {
  return ui_->keyDatabaseTabs->currentWidget() == ui_->externalTab
             ? &model_.external
             : &model_.managed;
}

auto KeyDatabasesTab::current_row() const -> int {
  const auto selected_rows = current_table()->selectionModel()->selectedRows();
  if (selected_rows.isEmpty()) return -1;
  return selected_rows.first().row();
}

auto KeyDatabasesTab::TakePendingMoves() -> QList<KeyDatabaseMove> {
  auto moves = std::move(pending_moves_);
  pending_moves_.clear();
  return moves;
}

auto KeyDatabasesTab::pending_move_for(const QString& id) const
    -> const KeyDatabaseMove* {
  for (const auto& move : pending_moves_) {
    if (move.database_id == id) return &move;
  }
  return nullptr;
}

void KeyDatabasesTab::drop_pending_move(const QString& id) {
  pending_moves_.erase(
      std::remove_if(pending_moves_.begin(), pending_moves_.end(),
                     [&](const KeyDatabaseMove& move) -> bool {
                       return move.database_id == id;
                     }),
      pending_moves_.end());
}

void KeyDatabasesTab::record_move(const KeyDatabaseInfo& info,
                                  const QString& to) {
  // Renamed again before applying: still one move, from where it is on disk
  // to where it should end up.
  for (auto& move : pending_moves_) {
    if (move.database_id != info.id) continue;
    move.to = to;
    return;
  }

  KeyDatabaseMove move;
  move.database_id = info.id;
  move.old_name = info.name;
  move.old_stored_path =
      info.origin_path.isEmpty() ? info.path : info.origin_path;
  move.from = info.path;
  move.to = to;
  pending_moves_.append(move);
}

auto KeyDatabasesTab::OperationsLeftRunning() const -> QStringList {
  const auto find = [&](const QString& id) -> const KeyDatabaseInfo* {
    if (model_.builtin && model_.builtin->id == id) {
      return &*model_.builtin;
    }
    for (const auto* list : {&model_.managed, &model_.external}) {
      for (const auto& info : *list) {
        if (info.id == id) return &info;
      }
    }
    return nullptr;
  };

  QStringList lines;
  for (const auto& channel : ListRuntimeChannels()) {
    if (channel.operations <= 0) continue;

    const auto* info = find(channel.database_id);
    const bool moved = std::any_of(
        pending_moves_.begin(), pending_moves_.end(),
        [&](const KeyDatabaseMove& m) {
          return m.database_id == channel.database_id;
        });
    const bool kept =
        info != nullptr && !moved &&
        IsSameKeyDatabaseLocation(info->path, channel.path) &&
        (info->backend_type.isEmpty() ? u"gnupg"_s
                                      : info->backend_type.toLower()) ==
            (channel.backend_type.isEmpty() ? u"gnupg"_s
                                            : channel.backend_type.toLower());
    if (kept) continue;

    lines.append(
        tr("%1: %n operation(s)", "", channel.operations)
            .arg(KeyDatabaseDisplayName(channel.database_id, channel.name)));
  }
  return lines;
}

auto KeyDatabasesTab::location_taken(const QString& path,
                                     const KeyDatabaseInfo* self) const
    -> bool {
  const auto taken = [&](const KeyDatabaseInfo& info) -> bool {
    return &info != self && IsSameKeyDatabaseLocation(info.path, path);
  };

  if (model_.builtin && taken(*model_.builtin)) return true;
  return std::any_of(model_.managed.begin(), model_.managed.end(), taken) ||
         std::any_of(model_.external.begin(), model_.external.end(), taken);
}

auto KeyDatabasesTab::total_count() const -> int {
  return (model_.builtin ? 1 : 0) + static_cast<int>(model_.managed.size()) +
         static_cast<int>(model_.external.size());
}

void KeyDatabasesTab::refresh_default_key_database_ui() {
  auto* box = ui_->builtInKeyDatabaseBox;
  const auto candidate = DefaultKeyDatabaseCandidate();
  const bool offered = !candidate.path.isEmpty();

  // In the sandbox the list is rebuilt from disk on every start with the
  // built-in database always first, so turning it off would not survive.
  // Shown as it is, with nothing to switch.
  // Only when it changes: QGroupBox::setCheckable(true) also checks the box,
  // which would undo turning it off.
  if (box->isCheckable() == is_sandbox_) box->setCheckable(!is_sandbox_);

  // A build whose engine names no key database has nothing to offer here,
  // and saying so is more use than an unexplained empty section. The one hint
  // that stays on the page: it explains why nothing here works.
  if (!offered) {
    if (box->isCheckable()) box->setChecked(false);
    box->setEnabled(false);
    ui_->defaultKeyDatabaseHintLabel->setText(
        tr("The OpenPGP engine on this computer does not report a key "
           "database, so there is no built-in one to use."));
    ui_->defaultKeyDatabaseHintLabel->setVisible(true);
  } else {
    box->setEnabled(true);
    if (box->isCheckable()) box->setChecked(model_.builtin.has_value());
    ui_->defaultKeyDatabaseHintLabel->setVisible(false);
  }

  // What turning it on means, on hover.
  const auto builtin_help =
      is_sandbox_
          ? tr("Always available on this computer, and always listed first.")
          : tr("Always listed first. Never included in a profile package: "
               "whoever opens your profile elsewhere gets their own.");
  box->setToolTip(builtin_help);
  box->setAccessibleDescription(builtin_help);

  // The engine it would be opened with, and what that means, follow the
  // selector before anything is applied. The engine is the core's decision
  // (ChooseChannelZeroEngine), so a choice this build cannot honour shows the
  // engine it falls back to.
  const auto choice =
      model_.BuiltInEngine(GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG),
                           GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP));
  ui_->builtInDescriptionLabel->setText(
      choice.ok ? BuiltInKeyDatabaseEngineDescription(choice.engine)
                : tr("No engine in this build can open it."));
  // Secondary text, but only while enabled: when the section is off it greys
  // with everything else rather than keep a colour of its own.
  {
    auto palette = ui_->builtInDescriptionLabel->palette();
    // From the page's own palette: the label's already carries the muted
    // colour, and muting it again on every refresh fades it further.
    const auto muted = MutedTextColor(this->palette());
    palette.setColor(QPalette::Active, QPalette::WindowText, muted);
    palette.setColor(QPalette::Inactive, QPalette::WindowText, muted);
    ui_->builtInDescriptionLabel->setPalette(palette);
  }

  // Where it is opened, by the rule the core opens it by. The same folder for
  // either engine: rPGP keeps its database beside GnuPG's keys rather than
  // somewhere else. Resolved without creating anything.
  const auto location = QDir::toNativeSeparators(
      ExpandKeyDatabasePath(app_path_, BuiltInKeyDatabaseStoredPath()));
  ui_->builtInLocationEdit->setText(location);
  ui_->builtInLocationEdit->setCursorPosition(0);
  ui_->builtInLocationEdit->setToolTip(location);

  refresh_role_controls();
}

auto KeyDatabasesTab::selected_id() const -> QString {
  const auto row = current_row();
  const auto& list = ui_->keyDatabaseTabs->currentWidget() == ui_->externalTab
                         ? model_.external
                         : model_.managed;
  if (row < 0 || row >= list.size()) return {};
  return list[row].id;
}

void KeyDatabasesTab::refresh_role_controls() {
  const auto effective = model_.EffectiveDefaultId();
  const auto role_help =
      tr("Used when an operation does not explicitly select a key database.");

  // The built-in section: the marker when it holds the role, the action when
  // it could, nothing about the role while it is turned off.
  const auto builtin_id = QString(QLatin1StringView(kDefaultKeyDatabaseId));
  const bool builtin_on = model_.builtin.has_value();
  const bool builtin_default = builtin_on && effective == builtin_id;
  ui_->builtInDefaultMarkerLabel->setVisible(builtin_default);
  ui_->builtInUseAsDefaultButton->setVisible(builtin_on && !builtin_default);
  ui_->builtInUseAsDefaultButton->setEnabled(
      model_.CanUseAsDefault(builtin_id));

  // The Role column of both tables. A name or a status never says it.
  const auto mark = [&](QTableWidget* table,
                        const QList<KeyDatabaseInfo>& list) {
    for (int row = 0; row < table->rowCount() && row < list.size(); row++) {
      auto* item = table->item(row, 4);
      if (item == nullptr) continue;
      const bool is_default = list[row].id == effective;
      item->setText(is_default ? tr("Default") : QString());
      item->setToolTip(is_default ? role_help : QString());
      auto font = item->font();
      font.setBold(is_default);
      item->setFont(font);
    }
  };
  mark(ui_->keyDatabaseTable, model_.managed);
  mark(ui_->externalKeyDatabaseTable, model_.external);

  // One rule for the visible button and the menu action of each table, so the
  // two never disagree about the selected row.
  const auto selected_of = [](QTableWidget* table,
                              const QList<KeyDatabaseInfo>& list) {
    const auto rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return QString();
    const auto row = rows.first().row();
    return row < list.size() ? list[row].id : QString();
  };
  ui_->profileUseAsDefaultButton->setEnabled(model_.CanUseAsDefault(
      selected_of(ui_->keyDatabaseTable, model_.managed)));
  ui_->externalUseAsDefaultButton->setEnabled(model_.CanUseAsDefault(
      selected_of(ui_->externalKeyDatabaseTable, model_.external)));

  const auto selected = selected_id();
  use_as_default_action_->setEnabled(model_.CanUseAsDefault(selected));
  copy_folder_path_action_->setEnabled(!selected.isEmpty());

  // A GnuPG agent's smart card role is fixed when it starts, and applying
  // keeps the agents running. Moving the default from one GnuPG database to
  // another therefore leaves smart cards with the previous one until its agents
  // are restarted. Said only when that is what the pending change would do.
  const auto running = LookupChannelHandle(DefaultChannelHandle());
  const auto is_gnupg = [](const QString& backend) {
    return backend.isEmpty() ||
           backend.compare("gnupg", Qt::CaseInsensitive) == 0;
  };
  bool new_default_gnupg = false;
  if (effective == builtin_id) {
    const auto choice =
        model_.BuiltInEngine(GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG),
                             GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP));
    new_default_gnupg = choice.ok && choice.engine == OpenPGPEngine::kGNUPG;
  } else {
    for (const auto* list : {&model_.managed, &model_.external}) {
      for (const auto& info : *list) {
        if (info.id == effective)
          new_default_gnupg = is_gnupg(info.backend_type);
      }
    }
  }
  const bool notice = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG) &&
                      !running.database_id.isEmpty() &&
                      running.database_id != effective &&
                      is_gnupg(running.backend_type) && new_default_gnupg;
  ui_->smartCardNoticeLabel->setVisible(notice);
}

void KeyDatabasesTab::use_as_default(const QString& id) {
  // Pending until OK, like everything else on the page; Cancel drops it.
  if (!model_.UseAsDefault(id)) return;
  refresh_role_controls();
  emit SignalRuntimeApplyNeeded();
}

void KeyDatabasesTab::fill_table(QTableWidget* table,
                                 const QList<KeyDatabaseInfo>& key_databases,
                                 int selected_row) {
  table->setUpdatesEnabled(false);
  table->clearContents();
  table->setRowCount(static_cast<int>(key_databases.size()));

  for (int index = 0; index < key_databases.size(); index++) {
    const auto& key_db = key_databases[index];

    // Whether the Runtime in use has this database open. Matched by id: the
    // channel it is open on changes with every apply, which is also why the
    // id rather than the channel is what the row shows.
    const auto active =
        std::find_if(active_key_db_infos_.begin(), active_key_db_infos_.end(),
                     [&key_db](const KeyDatabaseInfo& i) -> bool {
                       return !key_db.id.isEmpty() ? i.id == key_db.id
                                                   : i.name == key_db.name;
                     });
    auto* i_id =
        CreateTableItem(KeyDatabaseDisplayId(key_db.id), Qt::AlignCenter);
    i_id->setToolTip(key_db.id);
    table->setItem(index, 0, i_id);

    table->setItem(
        index, 1,
        CreateTableItem(key_db.name, Qt::AlignLeft | Qt::AlignVCenter));

    const auto backend_type_display =
        key_db.backend_type.isEmpty() ? u"GNUPG"_s
                                      : key_db.backend_type.toUpper().trimmed();
    table->setItem(index, 2,
                   CreateTableItem(backend_type_display, Qt::AlignCenter));

    const auto is_active = active != active_key_db_infos_.end();

    table->setItem(index, 3, CreateStatusItem(is_active));

    // Filled by refresh_role_controls(), which every change of the default
    // runs; left empty here so there is one place that decides it.
    table->setItem(index, 4, CreateTableItem(QString(), Qt::AlignCenter));

    // One folder column instead of the stored path beside the resolved one.
    // They were the same directory written two ways, both cut off, and neither
    // readable. The short form is shown and the resolved one is the tooltip,
    // which is the order of interest: where it is in the profile first, where
    // it is on this disk when you ask.
    auto* i_folder = CreateTableItem(
        KeyDatabaseDisplayPath(
            key_db.origin_path.isEmpty() ? key_db.path : key_db.origin_path,
            GlobalSettingStation::GetInstance().GetAppDataPath()),
        Qt::AlignLeft | Qt::AlignVCenter);
    i_folder->setToolTip(
        key_db.path.isEmpty()
            ? tr("This folder could not be resolved on this computer.")
            : QDir::toNativeSeparators(key_db.path));
    table->setItem(index, 5, i_folder);
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

  fill_table(ui_->keyDatabaseTable, model_.managed,
             external_visible ? -1 : selected_row);
  fill_table(ui_->externalKeyDatabaseTable, model_.external,
             external_visible ? selected_row : -1);

  // The hint doubles as the empty state. A blank table with a heading over it
  // says only that something is missing; this says what would go there.
  ui_->profileHintLabel->setText(
      model_.managed.isEmpty()
          ? tr("No key databases of your own yet. One you add here is kept "
               "inside your profile and travels with it.")
          : tr("Kept inside your profile, and carried by a profile package."));

  ui_->externalHintLabel->setText(
      model_.external.isEmpty()
          ? tr("Nothing here yet. A key database you add on this tab stays on "
               "this computer: a profile package never carries it, and it is "
               "always opened after the ones inside your profile.")
          : tr("Key databases outside your profile. They belong to this "
               "computer alone: a profile package never carries them, and "
               "they are always opened after the ones inside your profile."));

  refresh_role_controls();
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

  emit SignalRuntimeApplyNeeded();
}

void KeyDatabasesTab::slot_move_to_top_key_database() {
  const auto row = current_row();
  if (row <= 0) return;

  auto* list = current_list();
  auto selected_item = list->takeAt(row);
  list->insert(0, selected_item);

  slot_refresh_key_database_table(0);

  emit SignalRuntimeApplyNeeded();
}

void KeyDatabasesTab::slot_move_down_key_database() {
  const auto row = current_row();
  auto* list = current_list();
  if (row < 0 || row >= list->size() - 1) return;

  list->swapItemsAt(row, row + 1);

  slot_refresh_key_database_table(row + 1);

  emit SignalRuntimeApplyNeeded();
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
      ui_->builtInKeyDatabaseBox->setChecked(false);
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
    info.id = QLatin1StringView(kDefaultKeyDatabaseId);

    // An external entry may already point at this computer's own keyring.
    // Turning DEFAULT on beside it would open that directory twice.
    if (location_taken(info.path)) {
      QMessageBox::warning(
          this, tr("Duplicate Key Database Paths"),
          tr("A key database on this page already uses this computer's "
             "default key database folder. Remove it first, then turn this "
             "on."));
      loading_ = true;
      ui_->builtInKeyDatabaseBox->setChecked(false);
      loading_ = false;
      return;
    }

    model_.SetBuiltIn(info);

    refresh_default_key_database_ui();
    slot_refresh_key_database_table();
    emit SignalRuntimeApplyNeeded();
    return;
  }

  // Turning it off has to leave something behind. An empty list is not a state
  // the program has: ReconcileKeyDatabaseList() seeds the default one back
  // whenever nothing survives, so the checkbox would come back on by itself at
  // the next start.
  if (total_count() <= 1) {
    QMessageBox::warning(
        this, tr("Key Database Required"),
        tr("GpgFrontend needs at least one key database. Add another one "
           "first, then turn this off."));
    loading_ = true;
    ui_->builtInKeyDatabaseBox->setChecked(true);
    loading_ = false;
    return;
  }

  // Withdraws an explicit choice of it; turning it back on does not restore
  // that (KeyDatabaseSettingsModel::SetBuiltIn).
  model_.SetBuiltIn(std::nullopt);
  refresh_default_key_database_ui();
  slot_refresh_key_database_table();
  emit SignalRuntimeApplyNeeded();
}

void KeyDatabasesTab::slot_edit_key_database() {
  const auto row = current_row();
  auto* list = current_list();

  if (row < 0 || row >= list->size()) {
    QMessageBox::warning(this, tr("No Key Database Selected"),
                         tr("Please select a key database to edit."));
    return;
  }

  const auto managed = list == &model_.managed;
  const auto mode = managed ? KeyDatabaseEditDialog::Mode::kRENAME_MANAGED
                            : KeyDatabaseEditDialog::Mode::kEDIT_EXTERNAL;

  auto* dialog = new KeyDatabaseEditDialog(mode, *list, row, this);

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this, row, managed](const QString& name, const QString& backend_type,
                               const QString& path) {
            auto* list = managed ? &model_.managed : &model_.external;
            if (row < 0 || row >= list->size()) return;

            // Renaming a managed database moves its folder. The name and the
            // folder are one fact -- the sandbox reads the list back out of the
            // filesystem -- so an entry that renamed only itself would be
            // reverted without a word at the next start.
            bool move_needed = false;
            bool move_cancelled = false;
            if (managed) {
              const auto& id = (*list)[row].id;
              const auto* pending = pending_move_for(id);
              const auto clean_path =
                  QDir::cleanPath(QFileInfo(path).absoluteFilePath());

              // Renamed back to where its folder still is: the earlier move
              // is simply dropped, and there is nothing in the way.
              move_cancelled = pending != nullptr && clean_path == pending->from;

              // A folder still to be moved is not on disk under the name the
              // entry shows, yet there is one to move.
              const auto action =
                  move_cancelled
                      ? ManagedRenameAction::kNOTHING_TO_MOVE
                      : DecideManagedRename(pending != nullptr ||
                                                QFileInfo((*list)[row].path)
                                                    .isDir(),
                                            QFileInfo(path).exists());

              if (action == ManagedRenameAction::kTARGET_TAKEN) {
                QMessageBox::warning(
                    this, tr("Key Database Folder Already Exists"),
                    tr("A folder named after '%1' is already there. Its "
                       "contents are not ours to replace, so choose another "
                       "name.")
                        .arg(name));
                return;
              }

              // Moved when the change is applied, not now: the folder can
              // only move once nothing has it open, and only applying can wait
              // for that. Cancelling the dialog then leaves it untouched.
              // Recorded only once every check below has passed.
              move_needed = action == ManagedRenameAction::kRENAME;
            }

            // A managed folder that is still to be moved does not exist yet,
            // and resolving its path would create it in the move's way.
            auto key_db_fs_path =
                managed
                    ? QDir::cleanPath(QFileInfo(path).absoluteFilePath())
                    : GpgFrontend::GetCanonicalKeyDatabasePath(app_path_, path);
            if (key_db_fs_path.isEmpty()) {
              QMessageBox::warning(this, tr("Invalid Key Database Paths"),
                                   tr("The edited key database path is not a "
                                      "valid path that GpgFrontend can use"));
              return;
            }

            if (!managed && location_taken(key_db_fs_path, &(*list)[row])) {
              QMessageBox::warning(
                  this, tr("Duplicate Key Database Paths"),
                  tr("The edited key database path duplicates a previously "
                     "existing one."));
              return;
            }

            if (RejectsGnuPGSocketPath(this, backend_type, key_db_fs_path)) {
              return;
            }

            LOG_D() << "edit key database path, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            if (move_cancelled) drop_pending_move((*list)[row].id);
            if (move_needed) record_move((*list)[row], path);

            auto& key_database = (*list)[row];
            key_database.name = name;
            key_database.backend_type = backend_type;
            key_database.path = key_db_fs_path;
            // A managed folder is kept as `@profile/...`, like every other
            // one in the profile, so the stored list survives the profile
            // moving and travels in a package as it is.
            key_database.origin_path =
                managed ? ToProfileRelativeKeyDatabasePath(
                              path, GlobalSettingStation::GetInstance()
                                        .GetAppDataPath())
                        : path;
            key_database.kind = managed ? KeyDatabaseKind::kMANAGED
                                        : KeyDatabaseKind::kEXTERNAL;

            slot_refresh_key_database_table(row);

            emit SignalRuntimeApplyNeeded();
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
      KeyDatabaseEditDialog::Mode::kADD_MANAGED, model_.managed, this);

  connect(dialog, &KeyDatabaseEditDialog::SignalKeyDatabaseInfoAccepted, this,
          [this](const QString& name, const QString& backend_type,
                 const QString& path) -> void {
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

            // Asked of the resolved path, after the folder exists: the typed
            // one may be `@profile/...` or run through a link, and compared
            // as typed it never matched the stored, resolved spelling.
            if (location_taken(key_db_fs_path)) {
              QMessageBox::warning(
                  this, tr("Duplicate Key Database Paths"),
                  tr("The newly added key database path duplicates a "
                     "previously existing one."));
              return;
            }

            if (RejectsGnuPGSocketPath(this, backend_type, key_db_fs_path)) {
              return;
            }

            LOG_D() << "new key database path, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            KeyDatabaseInfo key_database;
            // Given its identity now rather than at the next read, so it is
            // the same database from the moment it exists.
            key_database.id = NewKeyDatabaseId();
            key_database.name = name;
            key_database.backend_type = backend_type;
            key_database.path = key_db_fs_path;
            key_database.origin_path = ToProfileRelativeKeyDatabasePath(
                path, GlobalSettingStation::GetInstance().GetAppDataPath());
            key_database.kind = KeyDatabaseKind::kMANAGED;
            model_.managed.append(key_database);

            slot_refresh_key_database_table();

            emit SignalRuntimeApplyNeeded();
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
  auto known = model_.managed;
  known.append(model_.external);

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

            if (location_taken(key_db_fs_path)) {
              QMessageBox::warning(
                  this, tr("Duplicate Key Database Paths"),
                  tr("The newly added key database path duplicates a "
                     "previously existing one."));
              return;
            }

            if (RejectsGnuPGSocketPath(this, backend_type, key_db_fs_path)) {
              return;
            }

            LOG_D() << "new external key database, name: " << name
                    << "path: " << path << "canonical path: " << key_db_fs_path;

            KeyDatabaseInfo key_database;
            key_database.id = NewKeyDatabaseId();
            key_database.name = name;
            key_database.backend_type = backend_type;
            key_database.path = key_db_fs_path;
            key_database.origin_path = path;
            // Recorded rather than inferred later: this is the user saying the
            // database belongs to this computer, and it is what keeps a package
            // from carrying it even if it happens to sit under the profile.
            key_database.kind = KeyDatabaseKind::kEXTERNAL;
            model_.external.append(key_database);

            slot_refresh_key_database_table();

            emit SignalRuntimeApplyNeeded();
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

  // A rename of it still waiting to be applied goes with it: left behind, it
  // would move the folder into a database added later under that name.
  drop_pending_move((*list)[row].id);
  // Removing the chosen default withdraws the choice; the marker moves to
  // whatever the fallback picks.
  model_.RemoveAt(*list, row);

  const int next_selected_row =
      list->isEmpty() ? -1 : std::min(row, static_cast<int>(list->size() - 1));

  slot_refresh_key_database_table(next_selected_row);

  emit SignalRuntimeApplyNeeded();
}

}  // namespace GpgFrontend::UI
