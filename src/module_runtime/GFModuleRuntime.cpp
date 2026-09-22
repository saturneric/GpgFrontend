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

#include <QSet>
#include <cstddef>

#include "GFModuleRuntimeBoot.h"
#include "GFModuleRuntimeDispatch.h"
#include "GFModuleRuntimeI18n.h"
#include "include/GFModule.h"

/**
 * @file GFModuleRuntime.cpp
 * @brief The module ABI, implemented once for every module.
 *
 * Every module used to carry its own expansion of GF_MODULE_BOOTSTRAP(): its
 * own ABI range check, its own table, its own dispatch loop, and its own
 * handler registry in an anonymous namespace -- which is what confined every
 * handler to one translation unit.
 */

namespace {

/// The hooks the module handed over. Borrowed; the module owns them and they
/// have static storage, which GFModuleRuntimeGetApi checks by contract.
const GFModuleHooks* g_hooks = nullptr;

/// Whether a field is within the hook table as the module compiled it.
template <typename M>
auto HooksCover(const GFModuleHooks* hooks, M GFModuleHooks::* member) -> bool {
  const auto* base = reinterpret_cast<const char*>(hooks);
  const auto* field = reinterpret_cast<const char*>(&(hooks->*member));
  return hooks->struct_size >= static_cast<size_t>(field - base) + sizeof(M);
}

/// Build the dispatch index, and report what the module says it handles.
auto IndexHooks(const GFModuleHooks* hooks) -> QSet<QString> {
  QSet<QString> hooked;
  auto& table = gf::runtime::HookTable();
  table.clear();

  if (!HooksCover(hooks, &GFModuleHooks::events_size)) return hooked;
  if (hooks->events == nullptr) return hooked;

  for (size_t i = 0; i < hooks->events_size; ++i) {
    const auto& binding = hooks->events[i];
    if (binding.event_id == nullptr || binding.handler == nullptr) continue;
    const auto id = QString::fromUtf8(binding.event_id);
    table.insert(id, binding.handler);
    hooked.insert(id);
  }
  return hooked;
}

/**
 * @brief Reconcile what the manifest declared against what the module handles.
 *
 * A declared event with no handler is a subscription nothing can service; a
 * handler with no declaration can never fire, because the host only delivers
 * what was subscribed. Both used to be possible and silent -- the LISTEN list
 * and the handler registry were separate, and stayed in step only by care.
 *
 * @return true when they agree, or when there is nothing verified to compare
 */
auto ReconcileSubscriptions(const QSet<QString>& hooked) -> bool {
  const auto& facts = gf::runtime::Facts();

  // Two cases fall back to the module's own table, and both are weaker
  // guarantees than a declared allowlist, so both say so.
  //
  // An unverified module has no manifest at all. A verified one that declares
  // nothing is a package built before the field existed -- indistinguishable
  // from one that genuinely subscribes to nothing, which is why the builder
  // omits the field rather than writing an empty array.
  if (!facts.verified || facts.events.isEmpty()) {
    LOG_WARN(
        QString("module %1 (%2): subscribing to %3 event(s) from its own "
                "hook table; a module whose manifest declares its events "
                "subscribes only to those")
            .arg(facts.id, facts.verified ? "no events declared" : "unverified")
            .arg(hooked.size()));
    return true;
  }

  const auto declared =
      QSet<QString>(facts.events.cbegin(), facts.events.cend());

  const auto unhandled = QStringList((declared - hooked).values()).join(", ");
  const auto undeclared = QStringList((hooked - declared).values()).join(", ");

  if (unhandled.isEmpty() && undeclared.isEmpty()) return true;

  if (!unhandled.isEmpty()) {
    LOG_ERROR(QString("module %1 declares events it does not handle: %2")
                  .arg(facts.id, unhandled));
  }
  if (!undeclared.isEmpty()) {
    LOG_ERROR(QString("module %1 handles events it does not declare: %2")
                  .arg(facts.id, undeclared));
  }
  return false;
}

// ------------------------------------------------------------ trampolines

auto RuntimeActivate(const GFHostApi* host, void* reserved) -> int {
  if (!gf::runtime::HostApiIsUsable(host)) {
    LOG_ERROR("the host api table is missing or too small; refusing to load");
    return -1;
  }

  gf::runtime::Facts() = gf::runtime::AdoptBootstrapInfo(
      static_cast<const GFModuleBootstrapInfo*>(reserved), g_hooks->module_id,
      g_hooks->module_version, g_hooks->translation_context);

  // Before anything else can log, translate or subscribe: every one of those
  // goes through the context, and the SDK holds none of its own.
  gf::runtime::AdoptHostApi(host, gf::runtime::Facts().id.toUtf8().constData());

  const auto hooked = IndexHooks(g_hooks);
  if (!ReconcileSubscriptions(hooked)) return -1;

  // Before any subscription, so a handler that fires immediately already has
  // its translations. This is the ordering the old macros had by convention.
  if (!gf::runtime::RegisterTranslations()) {
    LOG_WARN("could not register translations for " + gf::runtime::Facts().id);
  }

  const auto& facts = gf::runtime::Facts();
  const auto& subscribe = facts.verified ? facts.events : hooked.values();
  for (const auto& event_id : subscribe) {
    // Straight to the primitive: subscribing is the runtime's own business,
    // not something a module calls, so it is not part of the public SDK. The
    // host reads which module is subscribing from the context.
    host->event->subscribe(host->context, event_id.toUtf8().constData());
  }

  if (HooksCover(g_hooks, &GFModuleHooks::on_activate) &&
      g_hooks->on_activate != nullptr) {
    const auto result = g_hooks->on_activate();
    if (!result.ok) {
      LOG_ERROR("module activation failed: " + result.reason);
      return -1;
    }
  }
  return 0;
}

auto RuntimeExecute(GFModuleEvent* raw) -> int {
  // Consumed here whatever happens next: the host handed these buffers over,
  // and leaking them on an unknown event id is still a leak.
  const auto event = GFEventFactory::Consume(raw);

  const auto& table = gf::runtime::HookTable();
  const auto hook = table.constFind(event.Id());

  const auto result =
      hook == table.constEnd()
          ? GFEventResult::Bad(
                QString("unsupported event id: %1").arg(event.Id()))
          : (*hook)(event);

  // A deferred handler owns the answer now; sending one here would answer the
  // same trigger twice, and the host frees what it is given.
  if (result.status == GFEventStatus::kDEFERRED) return 0;

  gf::runtime::SendAnswer(event.Id(), event.TriggerId(),
                          gf::runtime::ResultToParams(result));
  return result.ok ? 0 : -1;
}

auto RuntimeDeactivate() -> int {
  if (g_hooks != nullptr &&
      HooksCover(g_hooks, &GFModuleHooks::on_deactivate) &&
      g_hooks->on_deactivate != nullptr) {
    const auto result = g_hooks->on_deactivate();
    if (!result.ok) {
      LOG_ERROR("module deactivation failed: " + result.reason);
      return -1;
    }
  }
  return 0;
}

void RuntimeUnregister() {
  if (g_hooks != nullptr && HooksCover(g_hooks, &GFModuleHooks::on_unload) &&
      g_hooks->on_unload != nullptr) {
    g_hooks->on_unload();
  }
  gf::runtime::HookTable().clear();
}

}  // namespace

extern "C" auto GFModuleRuntimeGetApi(uint32_t host_abi,
                                      const GFModuleHooks* hooks)
    -> const GFModuleApi* {
  // Decline a host outside the range this module was built for, rather than
  // loading and failing on the first mismatched call.
  if (host_abi < GF_SDK_ABI_MIN_SUPPORTED || host_abi > GF_SDK_ABI_VERSION) {
    return nullptr;
  }

  // A hook table too short to describe its own event list cannot be read
  // safely, and a module with no events is a module that does nothing.
  if (hooks == nullptr ||
      hooks->struct_size <
          offsetof(GFModuleHooks, events_size) + sizeof(size_t)) {
    return nullptr;
  }

  // Identity is what the host cross-checks against the signed manifest. A
  // module that does not state it cannot be matched to anything.
  if (hooks->module_id == nullptr || hooks->module_version == nullptr) {
    return nullptr;
  }
  g_hooks = hooks;

  // Static: the host borrows this table and never frees it. The identity
  // comes from the hooks, because this runtime is compiled once and linked
  // into every module -- it cannot see any one module's generated header.
  static const GFModuleApi kApi = {
      sizeof(GFModuleApi),   GF_SDK_ABI_VERSION, hooks->module_id,
      hooks->module_version, &RuntimeActivate,   &RuntimeExecute,
      &RuntimeDeactivate,    &RuntimeUnregister,
  };
  return &kApi;
}

// -------------------------------------------------- facts, for module code

auto GFModuleId() -> const QString& { return gf::runtime::Facts().id; }

auto GFGetModuleID() -> const char* {
  // Encoded once and held, so the pointer stays valid for as long as the
  // module does. Re-encoding per call would hand out a dangling pointer the
  // moment the temporary died.
  static const QByteArray kId = gf::runtime::Facts().id.toUtf8();
  return kId.constData();
}
auto GFModuleVersion() -> const QString& {
  return gf::runtime::Facts().version;
}
auto GFModuleIsVerified() -> bool { return gf::runtime::Facts().verified; }

auto GFModuleHasCapability(const QString& name) -> bool {
  return gf::runtime::Facts().capabilities.contains(name);
}

auto GFModuleSdkContext() -> GFSDKContext* { return gf::runtime::SdkContext(); }
