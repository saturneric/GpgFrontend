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

}  // namespace

KeyListController::KeyListController(QObject* parent)
    : QObject(parent), keys_(new KeyRoleModel(this)) {
  timeout_.setSingleShot(true);
  timeout_.setInterval(kRefreshTimeoutMs);
  connect(&timeout_, &QTimer::timeout, this,
          [this]() { finish(generation_, true); });
}

void KeyListController::SetRefresherForTesting(Refresher refresher) {
  RefresherSlot() =
      refresher ? std::move(refresher) : Refresher(RefreshKeyDatabases);
}

void KeyListController::SetRefreshTimeoutForTesting(int ms) {
  timeout_.setInterval(ms);
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

void KeyListController::refresh() {
  // Nor while one that timed out still runs: two would flush the same
  // repositories at once.
  if (refreshing_ || late_pending_) return;
  refreshing_ = true;
  emit SignalRefreshingChanged();

  const auto generation = ++generation_;
  timeout_.start();
  RefresherSlot()(this, [this, generation]() { finish(generation, false); });
}

void KeyListController::finish(int generation, bool timed_out) {
  // An earlier refresh's report: a later one is under way.
  if (generation != generation_) return;
  // The refresh that timed out reports back after all: the databases are
  // read now, so the list is shown again.
  const auto late = !refreshing_;
  if (late && timed_out) return;
  timeout_.stop();
  late_pending_ = timed_out;
  if (timed_out) {
    qWarning() << "the key database refresh did not report back within"
               << timeout_.interval() << "ms; showing what is loaded";
  } else if (late) {
    qInfo() << "the key database refresh that timed out ended after all";
  }

  const auto channel = UsableDefaultChannelHandle();
  SetTable(AbstractKeyRepository::GetInstance(channel).GetGpgKeyTableModel());
  qInfo() << "key list refreshed on channel" << channel << ":"
          << keys_->rowCount() << "entries";
  refreshing_ = false;
  if (!late) emit SignalRefreshingChanged();
  emit SignalRefreshed();
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
