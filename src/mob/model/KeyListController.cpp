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

#include "model/KeyListController.h"

#include <QJSEngine>

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyDatabaseRefresh.h"
#include "core/model/GpgKeyTableModel.h"

namespace GpgFrontend::Mobile {

namespace {

QPointer<KeyListController> g_instance;

auto RefresherSlot() -> KeyListController::Refresher& {
  static KeyListController::Refresher refresher = RefreshKeyDatabases;
  return refresher;
}

auto DefaultDatabaseId() -> QString {
  return LookupChannelHandle(UsableDefaultChannelHandle()).database_id;
}

auto DatabaseIdSlot() -> KeyListController::DatabaseId& {
  static KeyListController::DatabaseId database_id = DefaultDatabaseId;
  return database_id;
}

}  // namespace

KeyListController::KeyListController(QObject* parent)
    : QObject(parent), keys_(new KeyRoleModel(this)) {
  timeout_.setSingleShot(true);
  timeout_.setInterval(kRefreshTimeoutMs);
  connect(&timeout_, &QTimer::timeout, this,
          [this]() { finish(running_generation_, true); });
}

void KeyListController::SetRefresherForTesting(Refresher refresher) {
  RefresherSlot() =
      refresher ? std::move(refresher) : Refresher(RefreshKeyDatabases);
}

void KeyListController::SetRefreshTimeoutForTesting(int ms) {
  timeout_.setInterval(ms);
}

void KeyListController::SetDatabaseIdForTesting(DatabaseId database_id) {
  DatabaseIdSlot() =
      database_id ? std::move(database_id) : DatabaseId(DefaultDatabaseId);
}

KeyListController::~KeyListController() = default;

void KeyListController::SetInstance(KeyListController* instance) {
  g_instance = instance;
}

auto KeyListController::create(QQmlEngine*, QJSEngine*) -> KeyListController* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

auto KeyListController::Keys() const -> KeyRoleModel* { return keys_; }

auto KeyListController::Refreshing() const -> bool { return refreshing_; }

auto KeyListController::Loaded() const -> bool { return loaded_; }

auto KeyListController::Error() const -> QString { return error_; }

auto KeyListController::LoadedDatabase() const -> QString { return loaded_db_; }

void KeyListController::refresh() {
  ++generation_;
  // Another database than the one shown: what is shown is not its keys.
  if (loaded_ && loaded_db_ != DatabaseIdSlot()()) loaded_ = false;
  setRefreshing(true);
  // Never two at once: this one runs when the one under way ends -- unless
  // that one timed out, and may never report back (its end has been lost
  // before): then this one starts now. The core runs the two one after the
  // other on its one task thread, and the late report, of an older
  // generation, changes nothing.
  if (running_ && !timed_out_) {
    rerun_ = true;
    return;
  }
  start();
}

void KeyListController::ShowLoaded() {
  if (running_) return;
  ++generation_;
  show(DatabaseIdSlot()(), {});
}

void KeyListController::start() {
  running_ = true;
  timed_out_ = false;
  rerun_ = false;
  running_generation_ = generation_;
  running_db_ = DatabaseIdSlot()();
  timeout_.start();
  const auto generation = running_generation_;
  RefresherSlot()(this, [this, generation]() { finish(generation, false); });
}

void KeyListController::finish(int generation, bool timed_out) {
  // Not the run under way (a late report already handled).
  if (generation != running_generation_ || !running_) return;

  if (timed_out) {
    qWarning() << "the key database refresh did not report back within"
               << timeout_.interval() << "ms; showing what is loaded";
    timed_out_ = true;
    // A later refresh asked for meanwhile starts now: the report it waited
    // for may never come (as refresh() does after a timeout). The core runs
    // the two in turn on its one task thread.
    if (rerun_ || generation != generation_) {
      start();
      return;
    }
    // Otherwise what is loaded is shown; the late report, if it comes,
    // changes nothing more.
    show(DatabaseIdSlot()(), tr("The keys took too long to load."));
    return;
  }

  timeout_.stop();
  running_ = false;
  // Asked again meanwhile: this run's result is not the latest. Run again,
  // for the latest; it alone changes what is shown.
  if (rerun_ || generation != generation_) {
    start();
    return;
  }
  // Another database came into use while it ran, unasked: what it read may
  // be the old one's.
  const auto database = DatabaseIdSlot()();
  if (database != running_db_) {
    start();
    return;
  }
  show(database, {});
}

void KeyListController::show(const QString& database, const QString& error) {
  const auto channel = UsableDefaultChannelHandle();
  SetTable(AbstractKeyRepository::GetInstance(channel).GetGpgKeyTableModel());
  qInfo() << "key list refreshed on channel" << channel << ":"
          << keys_->rowCount() << "entries";
  loaded_db_ = database;
  loaded_ = true;
  error_ = error;
  refreshing_ = false;
  emit SignalRefreshingChanged();
  emit SignalRefreshed();
}

void KeyListController::setRefreshing(bool refreshing) {
  refreshing_ = refreshing;
  emit SignalRefreshingChanged();
}

void KeyListController::Reload() {
  if (g_instance == nullptr || g_instance->refreshing_) return;
  const auto channel = UsableDefaultChannelHandle();
  g_instance->SetTable(
      AbstractKeyRepository::GetInstance(channel).GetGpgKeyTableModel());
  emit g_instance->SignalRefreshed();
}

void KeyListController::SetTable(QSharedPointer<GpgKeyTableModel> table) {
  // The proxy lets go of the old table before it is released.
  keys_->setSourceModel(table.get());
  table_ = std::move(table);
}

}  // namespace GpgFrontend::Mobile
