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
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/ProfileController.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kColumnName = 0;
constexpr int kColumnId = 1;
constexpr int kColumnKind = 2;
constexpr int kColumnLastOpened = 3;
constexpr int kColumnStatus = 4;

}  // namespace

ProfileManagerDialog::ProfileManagerDialog(QWidget* parent)
    : GeneralDialog("profile_manager_dialog", parent) {
  init_ui();
  reload();
  setWindowTitle(tr("Profiles On This Computer"));
  setModal(true);

  // GeneralDialog deletes itself on close, which would free this one out from
  // under the caller that allocated it on the stack.
  setAttribute(Qt::WA_DeleteOnClose, false);

  movePosition2CenterOfParent();
}

void ProfileManagerDialog::init_ui() {
  auto* layout = new QVBoxLayout(this);

  // Says what the list *is* as well as what opening does. Everything here is a
  // profile this computer keeps; a profile file opened directly never appears,
  // which is otherwise only discoverable by not finding it.
  hint_ = new QLabel(
      tr("The profiles kept on this computer. Each one has its own settings, "
         "keys and saved state. Opening one starts a new window; this window "
         "stays exactly as it is."),
      this);
  hint_->setWordWrap(true);
  hint_->setStyleSheet("color: gray;");

  // Word-wrapped labels report one line of height unless asked to declare
  // heightForWidth, and the dialog is sized from what its layout reports.
  auto hint_policy = hint_->sizePolicy();
  hint_policy.setHeightForWidth(true);
  hint_policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
  hint_->setSizePolicy(hint_policy);
  layout->addWidget(hint_);

  table_ = new QTableWidget(this);
  table_->setColumnCount(5);
  table_->setHorizontalHeaderLabels(
      {tr("Name"), tr("Folder"), tr("Type"), tr("Last Opened"), tr("Status")});
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->hide();
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->setMinimumSize(680, 320);
  layout->addWidget(table_);

  auto* buttons = new QHBoxLayout();
  open_button_ = new QPushButton(tr("Open"), this);
  open_button_->setDefault(true);
  create_button_ = new QPushButton(tr("New..."), this);
  import_button_ = new QPushButton(tr("Import..."), this);
  delete_button_ = new QPushButton(tr("Delete"), this);
  reveal_button_ = new QPushButton(tr("Open Folder"), this);

  open_button_->setToolTip(tr("Open the selected profile in a new window"));
  create_button_->setToolTip(tr("Start an empty profile on this computer"));
  // Deliberately the same words as Profile -> Import Profile File: it is the
  // same act, and a second phrasing for it is how a user concludes there are
  // two.
  import_button_->setToolTip(
      tr("Copy a profile file into a new profile kept on this computer. The "
         "file is not used again afterwards."));
  delete_button_->setToolTip(
      tr("Remove the selected profile and everything in it, permanently"));
  reveal_button_->setToolTip(
      tr("Show the selected profile's folder in the file manager"));

  buttons->addWidget(open_button_);
  buttons->addWidget(create_button_);
  buttons->addWidget(import_button_);
  buttons->addWidget(delete_button_);
  buttons->addWidget(reveal_button_);
  buttons->addStretch();

  auto* close = new QPushButton(tr("Close"), this);
  buttons->addWidget(close);
  layout->addLayout(buttons);

  connect(open_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_open);
  connect(import_button_, &QPushButton::clicked, this, [this]() {
    ImportProfileInteractive(
        this, [this]() { reload(); }, [this]() { accept(); });
  });
  connect(create_button_, &QPushButton::clicked, this, [this]() {
    CreateProfileInteractive(
        this, [this]() { reload(); }, [this]() { accept(); });
  });
  connect(delete_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_delete);
  connect(reveal_button_, &QPushButton::clicked, this,
          &ProfileManagerDialog::slot_reveal);
  connect(close, &QPushButton::clicked, this, &ProfileManagerDialog::accept);
  connect(table_, &QTableWidget::itemSelectionChanged, this,
          &ProfileManagerDialog::slot_selection_changed);
  connect(table_, &QTableWidget::itemDoubleClicked, this,
          &ProfileManagerDialog::slot_open);
}

void ProfileManagerDialog::reload() {
  data_ = LoadProfiles();
  const auto current = ProfileSession::Instance().Profile().Id();

  table_->setRowCount(static_cast<int>(data_.profiles.size()));
  for (int row = 0; row < data_.profiles.size(); ++row) {
    const auto& e = data_.profiles.at(row);
    const auto is_current = e.id == current;

    auto* name = new QTableWidgetItem(is_current ? tr("%1  (open)").arg(e.name)
                                                 : e.name);
    if (is_current) {
      auto font = name->font();
      font.setBold(true);
      name->setFont(font);
    }
    name->setToolTip(e.root);

    table_->setItem(row, kColumnName, name);
    table_->setItem(row, kColumnId, new QTableWidgetItem(e.id));
    table_->setItem(row, kColumnKind,
                    new QTableWidgetItem(ProfileKindDisplayName(e.kind)));
    table_->setItem(row, kColumnLastOpened,
                    new QTableWidgetItem(e.last_opened));

    // "in use elsewhere" is worth showing here rather than only on a failed
    // switch: it explains the refusal before the user runs into it
    QString status;
    if (is_current) {
      status = tr("This window");
    } else if (!QFileInfo::exists(e.root)) {
      status = tr("Not created yet");
    } else if (ProfileLock::Probe(e.root).status ==
               ProfileLockStatus::kHELD_ELSEWHERE) {
      status = tr("Open in another window");
    }
    table_->setItem(row, kColumnStatus, new QTableWidgetItem(status));
  }

  table_->resizeColumnsToContents();
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
  const auto current = ProfileSession::Instance().Profile().Id();

  const auto has = entry.has_value();
  open_button_->setEnabled(has && entry->id != current);
  reveal_button_->setEnabled(has && QFileInfo::exists(entry->root));

  // Classic and portable are resolved by the application itself, so there is
  // nothing coherent for "delete" to mean; the current profile cannot be
  // deleted out from under the process that is using it.
  delete_button_->setEnabled(has && !entry->implicit && entry->id != current);
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
