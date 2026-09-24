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

#include "NativeWidgetRegistry.h"

#include "core/module/ModuleNamespace.h"

namespace GpgFrontend::UI {

auto NativeWidgetRegistry::Instance() -> NativeWidgetRegistry& {
  static NativeWidgetRegistry registry;
  return registry;
}

auto NativeWidgetRegistry::Register(NativeWidgetEntry entry) -> bool {
  if (!Module::IsOwnedName(entry.owner, entry.id) || !entry.create) {
    return false;
  }
  QMutexLocker locker(&mutex_);
  if (entries_.contains(entry.id)) return false;
  const auto id = entry.id;
  entries_.insert(id, std::move(entry));
  return true;
}

auto NativeWidgetRegistry::Unregister(const QString& owner, const QString& id)
    -> bool {
  QMutexLocker locker(&mutex_);
  const auto it = entries_.constFind(id);
  if (it == entries_.constEnd() || it->owner != owner) return false;
  entries_.erase(it);
  return true;
}

auto NativeWidgetRegistry::Find(const QString& id)
    -> std::optional<NativeWidgetEntry> {
  QMutexLocker locker(&mutex_);
  const auto it = entries_.constFind(id);
  if (it == entries_.constEnd()) return std::nullopt;
  return *it;
}

void NativeWidgetRegistry::RemoveAllFor(const QString& owner) {
  QMutexLocker locker(&mutex_);
  for (auto it = entries_.begin(); it != entries_.end();) {
    if (it->owner == owner) {
      it = entries_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace GpgFrontend::UI
