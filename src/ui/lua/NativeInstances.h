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

#include <QCborMap>
#include <QPointer>
#include <optional>

#include "ui/lua/NativeWidgetRegistry.h"

namespace GpgFrontend::UI {

/**
 * @brief The Host container a native widget lives in, as the module's
 *        notifications reach it.
 *
 * Each container implements only what its kind can hear; the instance
 * registry has already checked that the notification came from the module
 * that owns the instance, and fits the instance's kind.
 */
class GF_UI_EXPORT NativeContainer {
 public:
  virtual ~NativeContainer() = default;
  virtual void OnModified() {}
  virtual void OnShowSource(bool /*source*/) {}
  virtual void OnRequestCrypto(uint32_t /*op*/) {}
  virtual void OnOpsChanged() {}
  virtual void OnRestartNeeded(int /*level*/) {}
  virtual void OnClose() {}

  /**
   * @brief The module that owns this container's widget was withdrawn.
   *
   * The instance is already forgotten when this runs, and nothing enters the
   * module any more: the container drops the module's widget and carries on
   * without it -- a dialog closes, a document shows its own source.
   */
  virtual void OnWithdrawn() {}
};

/// One live native widget.
struct GF_UI_EXPORT NativeInstance {
  quint64 id = 0;
  QString owner;
  QString widget_id;
  NativeWidgetKind kind = NativeWidgetKind::kDIALOG;
  QPointer<QWidget> widget;
};

/**
 * @brief Every native widget alive now, and which container holds it.
 *
 * GUI thread only. An instance number is all a module ever holds of the
 * Host's side; it names nothing once the container is gone.
 */
class GF_UI_EXPORT NativeInstances {
 public:
  static auto Instance() -> NativeInstances&;

  /**
   * @brief Build @p widget_id's widget for @p container.
   *
   * @return the instance, or nullopt when the module is gone or its
   *         factory returned nothing
   */
  auto Create(const QString& widget_id, const QCborMap& args,
              NativeContainer* container) -> std::optional<NativeInstance>;

  /// The container is going away: tell the module, and forget the instance.
  void Destroy(quint64 id);

  [[nodiscard]] auto Find(quint64 id) const -> std::optional<NativeInstance>;
  [[nodiscard]] auto Entry(quint64 id) const
      -> std::optional<NativeWidgetEntry>;

  /**
   * @brief The container for a notification from @p module about @p id.
   *
   * @return nullptr -- refused -- when the instance is not @p module's, or
   *         is not of @p kind
   */
  [[nodiscard]] auto ContainerFor(quint64 id, const QString& module,
                                  NativeWidgetKind kind) const
      -> NativeContainer*;

  [[nodiscard]] auto CountFor(const QString& module) const -> int;

  /**
   * @brief Forget every instance of @p module and tell each container.
   *
   * Part of withdrawing a module. Its widgets must not outlive it inside the
   * Host's window, still running module code against state the module has
   * dropped. GUI thread.
   */
  void WithdrawAll(const QString& module);

 private:
  struct Live {
    NativeInstance instance;
    NativeContainer* container = nullptr;
  };
  QHash<quint64, Live> live_;
  quint64 next_ = 0;
};

}  // namespace GpgFrontend::UI
