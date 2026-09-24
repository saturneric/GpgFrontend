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

#include <QDir>
#include <QFile>
#include <QSet>
#include <cstddef>

#include "GFModuleRuntimeBoot.h"
#include "GFModuleRuntimeCommand.h"
#include "GFModuleRuntimeDispatch.h"
#include "GFModuleRuntimeI18n.h"
#include "GFModuleRuntimeNative.h"
#include "include/GFModule.h"

/**
 * @file GFModuleRuntime.cpp
 * @brief The module ABI, implemented once for every module.
 *
 * The ABI range check, the module table, the dispatch loop and the handler
 * registry live here, once; a module hands over a hook table and nothing
 * else.
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
 * @return true when they agree
 */
auto ReconcileSubscriptions(const QSet<QString>& hooked) -> bool {
  const auto& facts = gf::runtime::Facts();

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

/**
 * @brief Hand the Host every UI script `gf_add_module(LUA_SCRIPTS ...)`
 *        embedded, in name order.
 *
 * Under a resource prefix named for the module, so two modules' scripts
 * never meet in Qt's one process-wide resource tree.
 */
void LoadEmbeddedScripts() {
  const auto root =
      QStringLiteral(":/gf_module/%1/lua").arg(gf::runtime::Facts().id);
  QDir dir(root);
  if (!dir.exists()) return;
  auto names =
      dir.entryList({QStringLiteral("*.lua")}, QDir::Files, QDir::Name);
  for (const auto& name : names) {
    QFile f(dir.filePath(name));
    if (!f.open(QIODevice::ReadOnly)) continue;
    const auto source = f.readAll();
    auto* ctx = gf::runtime::SdkContext();
    auto* buf = GFBufferNewFromBytes(ctx, source.constData(),
                                     static_cast<size_t>(source.size()));
    if (GFUILoadScript(ctx, name.toUtf8().constData(), buf) != 0) {
      LOG_ERROR("could not hand UI script " + name + " to the host");
    }
    GFBufferRelease(ctx, buf);
  }
}

// ------------------------------------------------------------ trampolines

auto RuntimeActivate(const GFHostApi* host, void* reserved) -> int {
  if (!gf::runtime::HostApiIsUsable(host)) {
    LOG_ERROR("the host api table is missing or too small; refusing to load");
    return -1;
  }

  gf::runtime::PublishFacts(gf::runtime::AdoptBootstrapInfo(
      static_cast<const GFModuleBootstrapInfo*>(reserved), g_hooks->module_id,
      g_hooks->module_version, g_hooks->translation_context));

  // Before anything else can log, translate or subscribe: every one of those
  // goes through the context, and the SDK holds none of its own.
  gf::runtime::AdoptHostApi(host, gf::runtime::Facts().id.toUtf8().constData());

  // The host activates only verified modules; the runtime relies on the
  // signed lists below and has no other source for them.
  if (!gf::runtime::Facts().verified) {
    LOG_ERROR("the host did not vouch for this module; refusing to activate");
    return -1;
  }

  gf::runtime::ResetNativeRegistrations();

  const auto hooked = IndexHooks(g_hooks);
  if (!ReconcileSubscriptions(hooked)) return -1;

  // Before any subscription, so a handler that fires immediately already has
  // its translations.
  if (!gf::runtime::RegisterTranslations()) {
    LOG_WARN("could not register translations for " + gf::runtime::Facts().id);
  }

  const auto& facts = gf::runtime::Facts();
  for (const auto& event_id : facts.events) {
    // Straight to the primitive: subscribing is the runtime's own business,
    // not something a module calls, so it is not part of the public SDK. The
    // host reads which module is subscribing from the context -- and a
    // subscription it refuses is a module that cannot do what it declared.
    if (host->event->subscribe(host->context, event_id.toUtf8().constData()) !=
        0) {
      LOG_ERROR(QString("the host refused module %1's subscription to %2")
                    .arg(facts.id, event_id));
      return -1;
    }
  }

  // Before on_activate, which may already invoke them or load a UI script
  // that refers to them.
  if (HooksCover(g_hooks, &GFModuleHooks::commands_size) &&
      !gf::runtime::RegisterCommands(g_hooks->commands,
                                     g_hooks->commands_size)) {
    return -1;
  }
  if (HooksCover(g_hooks, &GFModuleHooks::commands_size)) {
    // And the other direction: a declared command nothing provides.
    for (const auto& id : facts.commands) {
      bool bound = false;
      for (size_t i = 0; i < g_hooks->commands_size; ++i) {
        bound = bound || id == QString::fromUtf8(g_hooks->commands[i].id);
      }
      if (!bound) {
        LOG_ERROR(QString("module %1 declares command %2 but provides none")
                      .arg(facts.id, id));
        return -1;
      }
    }
  }

  if (HooksCover(g_hooks, &GFModuleHooks::on_activate) &&
      g_hooks->on_activate != nullptr) {
    const auto result = g_hooks->on_activate();
    if (!result.ok) {
      LOG_ERROR("module activation failed: " + result.reason);
      return -1;
    }
  }

  // Last: a script mounts the native widgets on_activate registered.
  LoadEmbeddedScripts();
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
  // The Host withdraws the module's commands itself once this returns;
  // anything still owed to the module is forgotten here, on its side.
  gf::runtime::DropContinuations();
  gf::runtime::ForgetNativeWidgets();
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
  // Encoded once per published set of facts, which are never freed, so the
  // pointer stays valid for as long as the module does. A static cache here
  // used to freeze whatever the first call saw -- empty, before activation.
  return gf::runtime::Facts().id_utf8.constData();
}
auto GFModuleVersion() -> const QString& {
  return gf::runtime::Facts().version;
}
auto GFModuleIsVerified() -> bool { return gf::runtime::Facts().verified; }

auto GFModuleHasCapability(const QString& name) -> bool {
  return gf::runtime::Facts().capabilities.contains(name);
}

auto GFModuleSdkContext() -> GFSDKContext* { return gf::runtime::SdkContext(); }
