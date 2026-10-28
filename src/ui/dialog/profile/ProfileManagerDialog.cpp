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

#include "ui/dialog/profile/ProfileManagerDialog.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <array>
#include <optional>

#include "core/function/AESCryptoHelper.h"
#include "core/function/DataObjectOperator.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SystemSecretStore.h"
#include "core/model/SettingsObject.h"
#include "core/profile/ProfileLock.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileSecureKeyManager.h"
#include "core/profile/ProfileSession.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/FilesystemUtils.h"
#include "ui/dialog/profile/ProfileCreateDialog.h"
#include "ui/dialog/profile/ProfileExportDialog.h"
#include "ui/dialog/profile/ProfileListSummary.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/ProfileController.h"
#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kColumnName = 0;
constexpr int kColumnKind = 1;
constexpr int kColumnLastOpened = 2;
constexpr int kColumnStatus = 3;
constexpr int kColumnCount = 4;

auto StatusColor(const QPalette& palette, ProfileRowStatus status)
    -> std::optional<QColor> {
  switch (status) {
    case ProfileRowStatus::kThisWindow:
      return AccentColor(palette, true);
    case ProfileRowStatus::kOpenElsewhere:
      return WarningColor(palette);
    case ProfileRowStatus::kNotCreated:
      return MutedTextColor(palette);
    case ProfileRowStatus::kNone:
      break;
  }
  return {};
}

}  // namespace

ProfileManagerDialog::ProfileManagerDialog(QWidget* parent)
    : GeneralDialog("profile_manager_dialog", parent) {
  init_ui();
  reload();
  setWindowTitle(tr("Profiles On This Computer"));
  setModal(true);

  movePosition2CenterOfParent();
}

void ProfileManagerDialog::init_ui() {
  auto* layout = new QVBoxLayout(this);

  // Says what the list *is* as well as what opening does. Everything here is a
  // profile this computer keeps; a profile file opened directly never appears,
  // which is otherwise only discoverable by not finding it.
  layout->addLayout(CreateDialogHeader(
      u":/icons/workspace.png"_s, tr("Profiles On This Computer"),
      tr("The profiles kept on this computer. Each one has its own settings, "
         "keys and saved state. Opening one starts a new window; this window "
         "stays exactly as it is."),
      this));

  table_ = new QTableWidget(this);
  table_->setColumnCount(kColumnCount);
  table_->setHorizontalHeaderLabels(
      {tr("Name"), tr("Type"), tr("Last Opened"), tr("Status")});
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setContextMenuPolicy(Qt::CustomContextMenu);
  table_->setShowGrid(false);
  table_->setAlternatingRowColors(true);
  table_->setWordWrap(false);
  table_->verticalHeader()->hide();
  table_->verticalHeader()->setDefaultSectionSize(
      table_->fontMetrics().height() + 12);

  // The name takes whatever width is left; everything else is sized to what it
  // says in reload(), so no column is a stretch of nothing.
  auto* header = table_->horizontalHeader();
  header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  header->setHighlightSections(false);
  header->setStretchLastSection(false);
  header->setSectionResizeMode(kColumnName, QHeaderView::Stretch);

  table_->setMinimumSize(560, 220);
  layout->addWidget(table_);

  open_action_ = new QAction(tr("Open"), this);
  open_action_->setToolTip(tr("Open the selected profile in a new window"));
  reveal_action_ = new QAction(tr("Show in Folder"), this);
  reveal_action_->setToolTip(
      tr("Show the selected profile's folder in the file manager"));
  delete_action_ = new QAction(tr("Delete..."), this);
  delete_action_->setToolTip(
      tr("Remove the selected profile and everything in it, permanently"));
  delete_action_->setShortcut(QKeySequence::Delete);
  delete_action_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  table_->addAction(delete_action_);

  row_menu_ = new QMenu(this);
  row_menu_->setToolTipsVisible(true);
  row_menu_->addAction(open_action_);
  row_menu_->addAction(reveal_action_);
  row_menu_->addSeparator();
  row_menu_->addAction(delete_action_);

  // Everything that makes a profile rather than acts on one sits behind a
  // single button: three of them side by side read as three unrelated things.
  auto* new_menu = new QMenu(this);
  new_menu->setToolTipsVisible(true);
  auto* create_action = new_menu->addAction(tr("New Profile..."));
  create_action->setToolTip(tr("Start an empty profile on this computer"));
  auto* temp_action = new_menu->addAction(tr("New Temporary Profile"));
  temp_action->setToolTip(
      tr("Start an empty, throwaway profile in a new window. Everything in it "
         "is destroyed when that window closes."));
  new_menu->addSeparator();
  auto* import_action = new_menu->addAction(tr("Import Profile File..."));
  // Deliberately the same words as Profile -> Import Profile File: it is the
  // same act, and a second phrasing for it is how a user concludes there are
  // two.
  import_action->setToolTip(
      tr("Copy a profile file into a new profile kept on this computer. The "
         "file is not used again afterwards."));

  auto* new_button = new QPushButton(tr("New"), this);
  new_button->setMenu(new_menu);

  // An overflow glyph, so only as wide as one: at a full button's width it
  // reads as a button that forgot its label.
  more_button_ = new QPushButton(u"\u22EF"_s, this);
  more_button_->setToolTip(tr("More actions for the selected profile"));
  more_button_->setFixedWidth(more_button_->sizeHint().height() * 3 / 2);

  open_button_ = new QPushButton(tr("Open"), this);
  open_button_->setDefault(true);
  open_button_->setToolTip(open_action_->toolTip());
  auto* close = new QPushButton(tr("Close"), this);

  auto* buttons = new QHBoxLayout();
  buttons->addWidget(new_button);
  buttons->addWidget(more_button_);
  buttons->addStretch();
  buttons->addWidget(close);
  buttons->addWidget(open_button_);
  layout->addLayout(buttons);

  connect(open_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_open);
  connect(open_action_, &QAction::triggered, this,
          &ProfileManagerDialog::slot_open);
  connect(reveal_action_, &QAction::triggered, this,
          &ProfileManagerDialog::slot_reveal);
  connect(delete_action_, &QAction::triggered, this,
          &ProfileManagerDialog::slot_delete);
  connect(import_action, &QAction::triggered, this, [this]() {
    ImportProfileInteractive(
        this, [this]() { reload(); }, [this]() { accept(); });
  });
  connect(create_action, &QAction::triggered, this, [this]() {
    CreateProfileInteractive(
        this, [this]() { reload(); }, [this]() { accept(); });
  });
  connect(temp_action, &QAction::triggered, this, [this]() {
    OpenTemporaryProfileInteractive(this, [this]() { accept(); });
  });
  connect(more_button_, &QPushButton::clicked, this, [this]() {
    row_menu_->exec(
        more_button_->mapToGlobal(QPoint(0, more_button_->height())));
  });
  connect(close, &QPushButton::clicked, this, &ProfileManagerDialog::accept);
  connect(table_, &QTableWidget::itemSelectionChanged, this,
          &ProfileManagerDialog::slot_selection_changed);
  connect(table_, &QTableWidget::itemDoubleClicked, this,
          &ProfileManagerDialog::slot_open);
  connect(table_, &QTableWidget::customContextMenuRequested, this,
          &ProfileManagerDialog::slot_context_menu);
}

void ProfileManagerDialog::reload() {
  // Keep the row the user was on; on first load, start on this window's own
  // profile so the list opens on something rather than on nothing.
  const auto previous = selected();
  const auto current = ProfileSession::Instance().Profile().Id();
  const auto keep = previous.has_value() ? previous->id : current;

  data_ = LoadProfiles();
  const auto palette = table_->palette();

  table_->setRowCount(static_cast<int>(data_.profiles.size()));
  int keep_row = -1;
  for (int row = 0; row < data_.profiles.size(); ++row) {
    const auto& e = data_.profiles.at(row);
    const auto exists = QFileInfo::exists(e.root);
    const auto view =
        ProfileRowViewFor(e, current, exists,
                          exists && ProfileLock::Probe(e.root).status ==
                                        ProfileLockStatus::kHELD_ELSEWHERE);

    auto* name = new QTableWidgetItem(view.name);
    name->setToolTip(e.root);
    if (view.is_current) {
      auto font = name->font();
      font.setBold(true);
      name->setFont(font);
    }
    auto* status = new QTableWidgetItem(view.status);
    if (const auto color = StatusColor(palette, view.status_kind)) {
      status->setForeground(*color);
    }

    const std::array<QTableWidgetItem*, kColumnCount> items{
        name, new QTableWidgetItem(view.type),
        new QTableWidgetItem(view.last_opened), status};
    for (int column = 0; column < kColumnCount; ++column) {
      table_->setItem(row, column, items.at(column));
    }

    if (e.id == keep) keep_row = row;
  }

  // ResizeToContents packs cells edge to edge, with no gutter between one
  // column's text and the next one's.
  auto* header = table_->horizontalHeader();
  for (int column = 0; column < kColumnCount; ++column) {
    if (column == kColumnName) continue;
    table_->resizeColumnToContents(column);
    header->resizeSection(column, header->sectionSize(column) + 24);
  }

  if (keep_row >= 0) table_->selectRow(keep_row);
  slot_selection_changed();
}

auto ProfileManagerDialog::selected() const
    -> std::optional<ProfileRegistryEntry> {
  const auto row = table_->currentRow();
  if (row < 0 || row >= data_.profiles.size()) return {};
  return data_.profiles.at(row);
}

void ProfileManagerDialog::slot_selection_changed() {
  const auto entry = selected();
  ProfileRowView view;
  if (entry.has_value()) {
    view = ProfileRowViewFor(*entry, ProfileSession::Instance().Profile().Id(),
                             QFileInfo::exists(entry->root), false);
  }

  open_button_->setEnabled(view.can_open);
  open_action_->setEnabled(view.can_open);
  reveal_action_->setEnabled(view.can_reveal);
  delete_action_->setEnabled(view.can_delete);
  more_button_->setEnabled(entry.has_value());
}

void ProfileManagerDialog::slot_context_menu(const QPoint& pos) {
  const auto index = table_->indexAt(pos);
  if (!index.isValid()) return;
  table_->selectRow(index.row());
  row_menu_->exec(table_->viewport()->mapToGlobal(pos));
}

void ProfileManagerDialog::slot_open() {
  const auto entry = selected();
  if (!entry.has_value()) return;
  if (entry->id == ProfileSession::Instance().Profile().Id()) return;

  // No confirmation: nothing is closed, nothing is changed, and a window the
  // user did not want is closed again in one click.
  const auto result = OpenProfileInNewWindow({.profile_id = entry->id});
  if (!result.Ok()) {
    QMessageBox::warning(this, tr("Cannot Open Profile"), result.detail,
                         QMessageBox::Ok);
    reload();
    return;
  }

  accept();
}

void ProfileManagerDialog::slot_delete() {
  const auto entry = selected();
  if (!entry.has_value() || entry->implicit) return;

  // Deliberately two confirmations. Everything the profile's key encrypted
  // becomes permanently unreadable, and there is no undo anywhere below this.
  if (QMessageBox::warning(
          this, tr("Delete Profile"),
          tr("Delete \"%1\"?").arg(entry->name) + "\n\n" +
              tr("Its keys, settings and saved state are removed from this "
                 "computer permanently."),
          QMessageBox::Cancel | QMessageBox::Yes,
          QMessageBox::Cancel) != QMessageBox::Yes) {
    return;
  }
  if (QMessageBox::warning(
          this, tr("Delete Profile"),
          tr("This cannot be undone. Anything stored only in \"%1\" will be "
             "lost.")
              .arg(entry->name),
          QMessageBox::Cancel | QMessageBox::Yes,
          QMessageBox::Cancel) != QMessageBox::Yes) {
    return;
  }

  if (QFileInfo::exists(ProfileLock::PathFor(entry->root))) {
    QMessageBox::warning(this, tr("Profile Is Open"),
                         tr("\"%1\" is open in another window. Close it first.")
                             .arg(entry->name),
                         QMessageBox::Ok);
    return;
  }

  // The credential entry belongs to the profile and would otherwise be orphaned
  // in the store forever, with no way left to tell what it protected.
  if (auto* store = GetSystemSecretStore(); store != nullptr) {
    const auto marker = ReadProfileMarker(ProfileMarkerPathFor(entry->root));
    const auto account = DeriveAppKeyWrapAccount(
        entry->kind, entry->id, QDir(entry->root).canonicalPath(),
        marker.has_value() ? marker->profile_uuid : QString{});
    if (!account.isEmpty()) store->Remove(account);
  }

  if (!DeleteProfile(CurrentProfileRoots().profiles_root, entry->id)) {
    QMessageBox::critical(
        this, tr("Cannot Delete Profile"),
        tr("The profile folder could not be removed:") + "\n" + entry->root,
        QMessageBox::Ok);
  }

  reload();
}

void ProfileManagerDialog::slot_reveal() {
  const auto entry = selected();
  if (!entry.has_value()) return;
  QDesktopServices::openUrl(QUrl::fromLocalFile(entry->root));
}

}  // namespace GpgFrontend::UI
