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

#include "GFModuleEvent.h"
#include "GFModuleExport.h"
#include "GFModuleLog.h"
#include "GFModuleMemory.h"
#include "GFModuleUI.h"
#include "GFSDKBuildInfo.h"
#include "GFSDKModuleApi.h"

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
 *     auto OnActivate() -> GFResult { ... }
 *
 *     auto OnMainWindowMenuMounted(const GFEvent& e) -> GFEventResult {
 *       QMenu* help_menu = nullptr;
 *       if (auto r = e.RequireGui("help_menu", help_menu); !r.ok) return r;
 *       ...
 *       return GFEventResult::Ok();
 *     }
 *
 *     constexpr GFEventBinding kEvents[] = {
 *         {"MAINWINDOW_MENU_MOUNTED", &OnMainWindowMenuMounted},
 *     };
 *     constexpr GFModuleHooks kHooks = {
 *         sizeof(GFModuleHooks),
 *         GF_MODULE_ID, GF_MODULE_VERSION, GF_MODULE_TRANSLATION_CONTEXT,
 *         &OnActivate, nullptr, nullptr,
 *         kEvents, std::size(kEvents),
 *     };
 *
 *     extern "C" GF_MODULE_EXPORT auto GFModuleGetApi(uint32_t abi)
 *         -> const GFModuleApi* {
 *       return GFModuleRuntimeGetApi(abi, &kHooks);
 *     }
 *
 * That last function is the one piece of ABI a module writes, and it is
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

  /// Names this module's .qm files. Used only when the host hands over no
  /// verified context, i.e. in a loose development build.
  const char* translation_context;

  /// After the runtime has registered translations and subscriptions. This is
  /// where a module registers settings pages, tab views and metatypes.
  GFResult (*on_activate)();

  /// Undo what on_activate registered with the host. Anything holding a
  /// function pointer into this shared object MUST be unregistered here.
  GFResult (*on_deactivate)();

  /// Final teardown. No module code runs after this returns.
  void (*on_unload)();

  const GFEventBinding* events;  ///< borrowed, static storage
  size_t events_size;
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
auto GFModuleVersion() -> const QString&;

/// Whether the signed manifest declared a capability. A courtesy check: what a
/// module may actually do is decided by what the host put in GFHostApi.
auto GFModuleHasCapability(const QString& name) -> bool;

/// Whether the facts above came from a verified manifest rather than from the
/// module's own word for itself (a loose development build).
auto GFModuleIsVerified() -> bool;

/// The host table this module was activated with. Null before activation.
auto GFHost() -> const GFHostApi*;
