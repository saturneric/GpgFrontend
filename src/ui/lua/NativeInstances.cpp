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

#include "NativeInstances.h"

namespace GpgFrontend::UI {

auto NativeInstances::Instance() -> NativeInstances& {
  static NativeInstances instances;
  return instances;
}

auto NativeInstances::Create(const QString& widget_id, const QCborMap& args,
                             NativeContainer* container)
    -> std::optional<NativeInstance> {
  const auto entry = NativeWidgetRegistry::Instance().Find(widget_id);
  if (!entry.has_value() || !entry->create) return std::nullopt;

  NativeInstance instance;
  instance.id = ++next_;
  instance.owner = entry->owner;
  instance.widget_id = widget_id;
  instance.kind = entry->kind;
  // Registered first: the widget may notify from its constructor.
  live_.insert(instance.id, Live{instance, container});

  auto* widget = entry->create(instance.id, args);
  if (widget == nullptr) {
    live_.remove(instance.id);
    LOG_W() << "native widget" << widget_id << "was not built";
    return std::nullopt;
  }
  instance.widget = widget;
  live_[instance.id].instance.widget = widget;
  return instance;
}

void NativeInstances::Destroy(quint64 id) {
  const auto it = live_.constFind(id);
  if (it == live_.constEnd()) return;
  const auto widget_id = it->instance.widget_id;
  live_.remove(id);
  const auto entry = NativeWidgetRegistry::Instance().Find(widget_id);
  if (entry.has_value() && entry->destroyed) entry->destroyed(id);
}

auto NativeInstances::Find(quint64 id) const -> std::optional<NativeInstance> {
  const auto it = live_.constFind(id);
  if (it == live_.constEnd()) return std::nullopt;
  return it->instance;
}

auto NativeInstances::Entry(quint64 id) const
    -> std::optional<NativeWidgetEntry> {
  const auto it = live_.constFind(id);
  if (it == live_.constEnd()) return std::nullopt;
  return NativeWidgetRegistry::Instance().Find(it->instance.widget_id);
}

auto NativeInstances::ContainerFor(quint64 id, const QString& module,
                                   NativeWidgetKind kind) const
    -> NativeContainer* {
  const auto it = live_.constFind(id);
  if (it == live_.constEnd()) return nullptr;
  if (it->instance.owner != module || it->instance.kind != kind) {
    LOG_W() << "module" << module << "addressed native instance" << id
            << "which is not its own, or not of that kind; refused";
    return nullptr;
  }
  return it->container;
}

auto NativeInstances::CountFor(const QString& module) const -> int {
  int n = 0;
  for (const auto& l : live_) n += l.instance.owner == module ? 1 : 0;
  return n;
}

void NativeInstances::WithdrawAll(const QString& module) {
  QList<NativeContainer*> containers;
  for (auto it = live_.begin(); it != live_.end();) {
    if (it->instance.owner == module) {
      if (it->container != nullptr) containers.append(it->container);
      it = live_.erase(it);
    } else {
      ++it;
    }
  }
  // After the table is consistent: a container may be destroyed by this.
  for (auto* c : containers) c->OnWithdrawn();
}

}  // namespace GpgFrontend::UI
