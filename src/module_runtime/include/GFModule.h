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

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <cstddef>
#include <cstdint>

#include "GFModuleCommand.h"
#include "GFModuleEvent.h"
#include "GFModuleExport.h"
#include "GFModuleLog.h"
#include "GFModuleMemory.h"
#include "GFModuleNativeWidget.h"
#include "GFSDKBuildInfo.h"
// The whole public SDK, so a module author includes one header. Each of
// these declares functions that take a GFSDKContext*; the context comes from
// the event (`e.Context()`) or, outside a handler, from GFModuleSdkContext().
#include "GFSDKApp.h"
#include "GFSDKBuffer.h"
#include "GFSDKContext.h"
#include "GFSDKEditor.h"
#include "GFSDKGpg.h"
#include "GFSDKGpgList.h"
#include "GFSDKGpgResult.h"
#include "GFSDKLog.h"
#include "GFSDKModuleApi.h"
#include "GFSDKPgp.h"
#include "GFSDKProcess.h"
#include "GFSDKStorage.h"
#include "GFSDKUI.h"

// The C++ conveniences over the same ABI: QString in and out, fallbacks, and
// RAII for the handles. Still stateless, and still context-explicit.
#include "GFSDK.hpp"

/**
 * @file GFModule.h
 * @brief Everything a module author includes.
 *
 * A module declares its static facts in module.json, implements business
 * hooks, and gets bootstrap, translations, subscription, dispatch and
 * lifecycle from gf_module_runtime -- which it links statically, so there is
 * no second shared-library ABI to keep compatible.
 *
 * A whole module is:
 *
 *     #include <GFModule.h>
 *     #include "GFModuleIdentity.h"
 *
 *     struct ShowAbout {                       // a command: what it does
 *       static constexpr gf::cmd::Meta kMeta{GF_MODULE_ID ".show_about",
 *           GC_TR("About My Module"), "", "", 0, gf::cmd::kNeedsGuiThread};
 *       using Args = gf::cmd::Unit;
 *       using Result = gf::cmd::Unit;
 *     };
 *     auto DoShowAbout(const gf::cmd::CommandContext&, const gf::cmd::Unit&)
 *         -> gf::cmd::Outcome<gf::cmd::Unit> { ... }
 *
 *     auto OnActivate() -> GFResult { ... }  // register native widgets here
 *
 *     const std::array<gf::cmd::Binding, 1> kCommands = {
 *         gf::cmd::Bind<ShowAbout, &DoShowAbout>()};
 *     const GFModuleHooks kHooks = {
 *         sizeof(GFModuleHooks),
 *         GF_MODULE_ID, GF_MODULE_VERSION, GF_MODULE_TRANSLATION_CONTEXT,
 *         &OnActivate, nullptr, nullptr,
 *         nullptr, 0,                          // events it handles
 *         kCommands.data(), kCommands.size(),  // commands it provides
 *     };
 *
 *     extern "C" GF_MODULE_EXPORT auto GFModuleGetApi(uint32_t abi)
 *         -> const GFModuleApi* {
 *       return GFModuleRuntimeGetApi(abi, &kHooks);
 *     }
 *
 * and where the command is offered is its UI script, embedded with
 * `gf_add_module(... LUA_SCRIPTS ui/main.lua)`:
 *
 *     ui.action { id = "about", anchor = ui.anchor("main.menu.help"),
 *                 command = commands.get("<module id>.show_about") }
 *
 * GFModuleGetApi is the one piece of ABI a module writes, and it is
 * written out rather than hidden in a macro for a reason: it is the reference
 * that makes the linker keep the runtime's entry point. A static archive
 * member is pulled in only to resolve a symbol something already references,
 * and nothing in a module references GFModuleGetApi -- the host resolves it
 * after the module is linked.
 */

/// The Qt translation context every module's GC_TR() and tr() land in.
///
/// One class, declared once. Note this is the *context* -- the name Qt matches
/// against -- and is unrelated to a module's translation_context, which names
/// the .qm file and may carry many contexts.
class GTrC {
  Q_DECLARE_TR_FUNCTIONS(GTrC)
};

/// Mark a string for extraction without translating it yet. For text that is
/// registered before the module translators are installed, which would
/// otherwise be frozen at its source form for the whole session.
#define GC_TR(text) QT_TRANSLATE_NOOP("GTrC", text)

/// Why a lifecycle hook finished the way it did.
enum class GFStatus { kOK, kFAILED, kUNAVAILABLE };

/// A lifecycle hook's verdict. The house shape: {ok, status, reason}.
struct GFResult {
  bool ok = true;
  GFStatus status = GFStatus::kOK;
  QString reason;

  static auto Ok() -> GFResult { return {}; }
  static auto Fail(QString why) -> GFResult {
    return {false, GFStatus::kFAILED, std::move(why)};
  }
  static auto Unavailable(QString why) -> GFResult {
    return {false, GFStatus::kUNAVAILABLE, std::move(why)};
  }
};

/**
 * @brief Everything a module hands the runtime.
 *
 * Every hook is optional; a null one is a no-op, so a module implements only
 * what it has something to say about. Grows by APPENDING only, guarded by
 * @ref struct_size, exactly like the host and module ABI tables.
 */
struct GFModuleHooks {
  size_t struct_size;  ///< sizeof as the MODULE compiled it

  /// This module's identity, from the generated GFModuleIdentity.h.
  ///
  /// It has to be compiled in rather than delivered: the host reads
  /// GFModuleApi::module_id to decide whether the binary agrees with the
  /// manifest it was verified against, and that happens before activate(), so
  /// before anything could have been handed over. The generated header is the
  /// single build-time copy of what module.json already says.
  const char* module_id;
  const char* module_version;

  /// Names this module's .qm files when the verified context does not.
  const char* translation_context;

  /// After the runtime has registered translations, subscriptions and
  /// commands. This is where a module registers its native widgets and
  /// metatypes.
  GFResult (*on_activate)();

  /// Stop the module's OWN work -- threads it started, timers, network
  /// requests. Everything it registered with the Host (commands, widgets,
  /// scripts, subscriptions, translations) the Host withdraws itself, before
  /// this runs; there is nothing to unregister here. May be null.
  GFResult (*on_deactivate)();

  /// Final teardown. No module code runs after this returns.
  void (*on_unload)();

  const GFEventBinding* events;  ///< borrowed, static storage
  size_t events_size;

  /* --- appended ---------------------------------------------------------- */

  /// The commands this module provides, each `gf::cmd::Bind<C, &Fn>()`.
  /// Registered after activation and before on_activate, withdrawn at
  /// deactivation; the manifest's `commands` must list exactly these.
  const gf::cmd::Binding* commands;  ///< borrowed, static storage
  size_t commands_size;
};

/**
 * @brief Build the module ABI table. Implemented by gf_module_runtime.
 *
 * @param host_abi the host's ABI generation, for negotiation
 * @param hooks this module's hooks; borrowed, must have static storage
 * @return the table, or NULL to decline this host
 */
extern "C" auto GFModuleRuntimeGetApi(uint32_t host_abi,
                                      const GFModuleHooks* hooks)
    -> const GFModuleApi*;

/// This module's identity, as the host verified it. Valid from on_activate().
auto GFModuleId() -> const QString&;

/// The same, as a stable UTF-8 C string, for passing straight to the SDK.
///
/// Kept because the SDK takes `const char*` and roughly seventy call sites
/// across the modules pass this without wanting a conversion at each one. The
/// storage belongs to the runtime and lives as long as the module.
auto GFGetModuleID() -> const char*;
auto GFModuleVersion() -> const QString&;

/// Whether the signed manifest declared a capability. A courtesy check: what a
/// module may actually do is decided by what the host put in GFHostApi.
auto GFModuleHasCapability(const QString& name) -> bool;

/// Whether the facts above came from a verified manifest. Always true in an
/// active module: the Host activates only modules it verified, and the
/// runtime refuses to activate without that.
auto GFModuleIsVerified() -> bool;

/**
 * @brief This module's SDK context. Null before activation.
 *
 * Every public SDK function that needs the host takes one of these. Inside an
 * event handler prefer `event.Context()`, which carries the same context and
 * needs no lookup at all.
 *
 * Work started on a worker thread should CAPTURE the context (or the facade)
 * rather than calling this from the worker. The lookup is safe from any
 * thread, but capturing is what makes a piece of asynchronous code say which
 * module it belongs to, which is the property this whole design is for.
 */
auto GFModuleSdkContext() -> GFSDKContext*;
