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
#include <QMutex>
#include <functional>
#include <optional>

namespace GpgFrontend::UI {

/// The three typed interfaces a native widget may implement.
enum class NativeWidgetKind { kDOCUMENT = 1, kSETTINGS = 2, kDIALOG = 3 };

/**
 * @brief A widget a module registered from C++, for its UI script to mount.
 *
 * The module owns the widget and its whole tree; the Host owns the container
 * it is mounted in. What crosses is a module widget pointer, from the module
 * to the Host, and never the other way: the Host hands the module an opaque
 * instance number, not an object of its own.
 */
struct GF_UI_EXPORT NativeWidgetEntry {
  QString owner;  ///< module id
  QString id;     ///< "<module id>.<name>"
  NativeWidgetKind kind = NativeWidgetKind::kDIALOG;
  bool multi_instance = false;  ///< a factory: one widget per mount instance

  // Presentation, untranslated, in the module's "GTrC" context.
  QString title;
  QString keywords;
  QString suffix;
  QString filter;
  QString icon;
  int width = 0;
  int height = 0;

  /// Build the widget for one instance, on the GUI thread. The Host takes
  /// the result as a QWidget and places it in a container of its own.
  std::function<QWidget*(quint64 instance, const QCborMap& args)> create;
  /// The instance is going away; the module forgets it. Optional.
  std::function<void(quint64 instance)> destroyed;

  /// Typed operations, by kind. Filled by the SDK adapter; see
  /// NativeWidgetOps.h. Opaque here so the registry itself stays kind-agnostic.
  std::shared_ptr<void> ops;
};

class GF_UI_EXPORT NativeWidgetRegistry {
 public:
  static auto Instance() -> NativeWidgetRegistry&;

  /// A module registers only inside its own namespace, once per id.
  auto Register(NativeWidgetEntry entry) -> bool;
  auto Unregister(const QString& owner, const QString& id) -> bool;
  [[nodiscard]] auto Find(const QString& id) -> std::optional<NativeWidgetEntry>;
  [[nodiscard]] auto IdsOf(const QString& owner) -> QStringList;
  void RemoveAllFor(const QString& owner);

 private:
  QMutex mutex_;
  QHash<QString, NativeWidgetEntry> entries_;
};

}  // namespace GpgFrontend::UI
