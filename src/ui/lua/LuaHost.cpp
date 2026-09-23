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

#include "LuaHost.h"

namespace GpgFrontend::UI::Lua {

auto LuaHost::Instance() -> LuaHost& {
  static LuaHost* host = new LuaHost();  // outlives every placement
  return *host;
}

auto LuaHost::Load(const QString& module, uint32_t caps,
                   const QByteArray& source, const QString& chunk,
                   QString* error) -> bool {
  auto rt = runtimes_.value(module);
  if (rt == nullptr) {
    rt = std::make_shared<LuaModuleRuntime>(module, caps);
    if (rt->State() == nullptr) {
      if (error != nullptr) *error = QStringLiteral("no Lua state");
      return false;
    }
    connect(rt.get(), &LuaModuleRuntime::SignalChanged, this,
            &LuaHost::SignalChanged);
    runtimes_.insert(module, rt);
  }
  const bool ok = rt->Load(source, chunk, error);
  if (ok) {
    LOG_D() << "module" << module << "UI script" << chunk << "loaded:"
            << rt->Actions().size() << "action(s)," << rt->Mounts().size()
            << "mount(s)," << rt->Subscriptions().size() << "subscription(s)";
  }
  return ok;
}

auto LuaHost::Runtime(const QString& module) const -> LuaModuleRuntime* {
  return runtimes_.value(module).get();
}

auto LuaHost::Modules() const -> QStringList {
  auto m = runtimes_.keys();
  m.sort();
  return m;
}

auto LuaHost::ActionsOn(const QString& anchor) const -> QList<PlacedAction> {
  QList<PlacedAction> all;
  for (auto it = runtimes_.constBegin(); it != runtimes_.constEnd(); ++it) {
    for (const auto& a : (*it)->Actions(anchor)) {
      all.append(PlacedAction{it->get(), a});
    }
  }
  // The anchor's ordering rule: order, then module, then action id.
  std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
    if (a.info.order != b.info.order) return a.info.order < b.info.order;
    return a.info.id < b.info.id;
  });
  return all;
}

auto LuaHost::MountsOf(AnchorKind kind) const -> QList<PlacedMount> {
  QList<PlacedMount> all;
  for (auto it = runtimes_.constBegin(); it != runtimes_.constEnd(); ++it) {
    for (const auto& m : (*it)->Mounts()) {
      if (m.kind == kind) all.append(PlacedMount{it.key(), m});
    }
  }
  std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
    if (a.info.order != b.info.order) return a.info.order < b.info.order;
    return a.info.id < b.info.id;
  });
  return all;
}

auto LuaHost::EditorOwner(const QString& document_type) const -> QString {
  for (const auto& m : MountsOf(AnchorKind::kEDITOR)) {
    if (m.info.document_type == document_type) return m.module;
  }
  return {};
}

void LuaHost::Deliver(const QString& event, const UiContext& ctx,
                      qint64 document_id) {
  // A copy: a handler may, through a command, end up tearing a runtime down.
  const auto runtimes = runtimes_.values();
  for (const auto& rt : runtimes) rt->Deliver(event, ctx, document_id);
}

void LuaHost::Teardown(const QString& module) {
  auto rt = runtimes_.take(module);
  if (rt == nullptr) return;
  rt->Teardown();
  emit SignalChanged();
}

void LuaHost::Reset() {
  for (const auto& m : runtimes_.keys()) Teardown(m);
}

}  // namespace GpgFrontend::UI::Lua
