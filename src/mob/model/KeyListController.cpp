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

}  // namespace

KeyListController::KeyListController(QObject* parent)
    : QObject(parent), keys_(new KeyRoleModel(this)) {}

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
  if (refreshing_) return;
  refreshing_ = true;
  emit SignalRefreshingChanged();

  RefreshKeyDatabases(this, [this]() {
    const auto channel = UsableDefaultChannelHandle();
    SetTable(AbstractKeyRepository::GetInstance(channel).GetGpgKeyTableModel());
    qInfo() << "key list refreshed on channel" << channel << ":"
            << keys_->rowCount() << "entries";
    refreshing_ = false;
    emit SignalRefreshingChanged();
    emit SignalRefreshed();
  });
}

void KeyListController::SetTable(QSharedPointer<GpgKeyTableModel> table) {
  // The proxy lets go of the old table before it is released.
  keys_->setSourceModel(table.get());
  table_ = std::move(table);
}

}  // namespace GpgFrontend::Mobile
