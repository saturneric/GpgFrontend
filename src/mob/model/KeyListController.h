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

#pragma once

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QPointer>
#include <QSharedPointer>

#include "model/KeyRoleModel.h"

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend {
class GpgKeyTableModel;
}

namespace GpgFrontend::Mobile {

/**
 * @brief The keys of the default key database, for every page that lists them.
 *
 * One KeyRoleModel, re-pointed at a fresh GpgKeyTableModel after each
 * refresh(), which re-reads the key databases the same way the desktop does
 * (RefreshKeyDatabases()). Pages filter it with their own KeyFilterModel.
 */
class KeyListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(GpgFrontend::Mobile::KeyRoleModel* keys READ Keys CONSTANT)
  Q_PROPERTY(bool refreshing READ Refreshing NOTIFY SignalRefreshingChanged)

 public:
  // Deliberately no default constructor: given one, QML constructs a
  // singleton of its own and never calls create(), so the interface would
  // talk to a different object than the application.
  explicit KeyListController(QObject* parent);
  ~KeyListController() override;

  static void SetInstance(KeyListController* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> KeyListController*;

  [[nodiscard]] auto Keys() const -> KeyRoleModel*;
  [[nodiscard]] auto Refreshing() const -> bool;

  /// Re-read the key databases, then show what they hold. Needs a ready core.
  Q_INVOKABLE void refresh();

  /// Show @p table's keys; refresh() does this with the default database's.
  void SetTable(QSharedPointer<GpgKeyTableModel> table);

 signals:
  void SignalRefreshingChanged();
  void SignalRefreshed();

 private:
  KeyRoleModel* keys_;
  QSharedPointer<GpgKeyTableModel> table_;
  bool refreshing_ = false;
};

}  // namespace GpgFrontend::Mobile
