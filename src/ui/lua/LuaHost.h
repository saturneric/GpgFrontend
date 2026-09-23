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

#include <QObject>
#include <QPointer>
#include <memory>

#include "ui/lua/LuaModuleRuntime.h"

namespace GpgFrontend::UI::Lua {

/// One action, as a placement sees it: whose, and which.
struct PlacedAction {
  QPointer<LuaModuleRuntime> runtime;
  ActionInfo info;
};

/// One mount, as a container sees it.
struct PlacedMount {
  QString module;
  MountInfo info;
};

/**
 * @brief Every module's UI runtime, for the Host's placements.
 *
 * Lives on the GUI thread; every call here is made there.
 */
class GF_UI_EXPORT LuaHost : public QObject {
  Q_OBJECT

 public:
  static auto Instance() -> LuaHost&;

  /**
   * @brief Load one of @p module's scripts, creating its runtime on first use.
   *
   * @return false, with the reason, when the chunk failed; nothing it
   *         registered remains
   */
  auto Load(const QString& module, uint32_t caps, const QByteArray& source,
            const QString& chunk, QString* error) -> bool;

  [[nodiscard]] auto Runtime(const QString& module) const -> LuaModuleRuntime*;
  [[nodiscard]] auto Modules() const -> QStringList;

  /// Every action on @p anchor, across modules, in anchor order.
  [[nodiscard]] auto ActionsOn(const QString& anchor) const
      -> QList<PlacedAction>;

  /// Every mount of @p kind, across modules.
  [[nodiscard]] auto MountsOf(AnchorKind kind) const -> QList<PlacedMount>;

  /// The module whose editor mount owns @p document_type, or empty.
  [[nodiscard]] auto EditorOwner(const QString& document_type) const
      -> QString;

  /// Deliver a UI event to every module that subscribed.
  void Deliver(const QString& event, const UiContext& ctx,
               qint64 document_id = 0);

  /// Tear down @p module's runtime, in order, and forget it. Idempotent.
  void Teardown(const QString& module);

  /// For tests: forget every runtime without the rest of module teardown.
  void Reset();

 signals:
  /// Something a placement shows changed; rebuild.
  void SignalChanged();

 private:
  LuaHost() = default;

  QHash<QString, std::shared_ptr<LuaModuleRuntime>> runtimes_;
};

}  // namespace GpgFrontend::UI::Lua
