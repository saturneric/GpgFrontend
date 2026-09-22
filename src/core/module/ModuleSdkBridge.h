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

#include <QString>
#include <cstddef>
#include <cstdint>

namespace GpgFrontend::Module {

/**
 * @file ModuleSdkBridge.h
 * @brief The SDK services gf_core needs, supplied by the host half of the SDK
 * rather than linked against it.
 *
 * ## Why this exists
 *
 * The host half of the SDK (`gf_host_api`, `src/sdk/host`) is built on top of
 * `gf_core` and `gf_ui`. But the core's module loader needs a few things only
 * that half can provide: the host API table it hands a module at activation,
 * the attribution brackets that record which module asked for a handle, and
 * the release, idle wait and sweep that retire a module's grant and reclaim
 * what it leaked. Calling them directly would make `gf_core` depend on code
 * that depends on `gf_core`, and neither a MinGW DLL nor a Mach-O dylib can
 * be linked with a cycle.
 *
 * So the dependency is inverted. `gf_core` declares what it needs, the host
 * half fills in this table, and the link graph stays a DAG:
 *
 * ```
 *   gf_host_api  ---- links ---->  gf_core
 *        |                            ^
 *        '------ installs bridge -----'   (GFHostApiInstallBridge(), called
 *                                          from main(); no link dependency)
 * ```
 *
 * ## It is not a plugin seam
 *
 * Exactly one implementation exists and exactly one ever will. This is not an
 * extension point, and nothing should grow policy behind it. It is the
 * narrowest possible way to say "these functions live on the other side of a
 * link edge that only points one way".
 */

/// The entry points, as a table. A null member is a service this build does
/// not have, which is not the same as one that does nothing -- see
/// ModuleSdkMintHostApi().
struct GF_CORE_EXPORT ModuleSdkBridge {
  /// Mint the host api table for ONE module, granting exactly @p granted.
  /// There is deliberately no "get the host api" here any more: a getter
  /// returns the same everything-table to every caller, which makes a
  /// per-module grant advisory.
  const void* (*mint_host_api)(const char* module_id,
                               uint32_t granted) = nullptr;
  /// Invalidate that module's table and context. A call arriving afterwards
  /// is refused rather than served, on any thread.
  void (*release_host_api)(const char* module_id) = nullptr;
  /// Wait for calls already past the gate to finish. False on timeout.
  bool (*wait_host_api_idle)(const char* module_id, int timeout_ms) = nullptr;
  const char* (*enter_module)(const char* module_id) = nullptr;
  /// Whose code this thread is currently running, for diagnostics and for
  /// the handle ledger. NEVER an authorization input: that is the context.
  const char* (*current_module)() = nullptr;
  void (*leave_module)(const char* previous) = nullptr;
  size_t (*sweep_module_handles)(const char* module_id) = nullptr;
};

/**
 * @brief Install the SDK's implementation. Called once, from
 * GFHostApiInstallBridge() at startup.
 *
 * Idempotent, and deliberately not reversible: there is no uninstall, because
 * a module that has already been handed the host api table holds it for the
 * life of the process.
 */
void GF_CORE_EXPORT InstallModuleSdkBridge(const ModuleSdkBridge& bridge);

/// Whether the bridge has been installed yet.
auto GF_CORE_EXPORT IsModuleSdkBridgeInstalled() -> bool;

/**
 * @brief Mint the host api table to hand @p module_id at activation.
 *
 * @param module_id the module the table is for; it is baked into the table's
 *        context, so the table cannot be usefully passed to another module
 * @param granted GF_HOST_CAP_* bits from the module's signed manifest. A
 *        group whose bit is clear is NULL in the resulting table, and its
 *        primitives refuse the context even if reached another way.
 *
 * Returns nullptr when the bridge is not installed, which a caller MUST treat
 * as a refusal to activate rather than as an empty table. Handing a module a
 * null host api would leave it unable to call anything and unable to say why;
 * refusing to activate it says so once, in the log, at the point of failure.
 *
 * The type is erased so this header does not drag the SDK's headers into
 * gf_core's. The one caller casts it back to `const GFHostApi*`, which is
 * safe because there is exactly one producer of this pointer.
 */
auto GF_CORE_EXPORT ModuleSdkMintHostApi(const char* module_id,
                                         uint32_t granted) -> const void*;

/**
 * @brief Drop @p module_id's table and invalidate its context.
 *
 * Called at unload, before the image is unmapped. Afterwards the module's
 * context is unknown to the host, so a call that somehow arrives from a
 * thread the module failed to stop is refused rather than followed into
 * memory that is no longer there.
 */
void GF_CORE_EXPORT ModuleSdkReleaseHostApi(const char* module_id);

/**
 * @brief Wait until no SDK call made with @p module_id's context is running.
 *
 * Releasing the grant refuses new calls; a call that passed the gate just
 * before keeps running. Wait for those too before freeing anything they may
 * be using. Calls made on the waiting thread itself are not waited for.
 *
 * True when the bridge is not installed: then no call can be running.
 *
 * @return false on timeout
 */
auto GF_CORE_EXPORT ModuleSdkWaitHostApiIdle(const char* module_id,
                                             int timeout_ms) -> bool;

/**
 * @brief The module this thread is currently attributed to, or empty.
 *
 * Diagnostics and handle bookkeeping only. Authorization lives in the context
 * a call carries, because a module's own worker threads never pass through
 * here at all.
 */
auto GF_CORE_EXPORT ModuleSdkCurrentModule() -> QString;

/**
 * @brief Reclaim every SDK handle still held by @p module_id.
 *
 * Zero when the bridge was never installed, which is the honest answer: then
 * no module was ever activated, so none obtained a handle.
 */
auto GF_CORE_EXPORT ModuleSdkSweepHandles(const char* module_id) -> size_t;

/**
 * @brief Bracket a call into module code so its handles are attributed to it.
 *
 * Calls a module makes through its own context are attributed from that
 * context. This covers the rest: handles created while the host is running
 * a module's hook, for which no module context is on hand.
 *
 * A no-op when the bridge is not installed. That is correct rather than
 * merely tolerable -- with no SDK loaded there are no handles to attribute.
 */
class GF_CORE_EXPORT ModuleAttributionScope {
 public:
  explicit ModuleAttributionScope(const char* module_id);
  ~ModuleAttributionScope();

  ModuleAttributionScope(const ModuleAttributionScope&) = delete;
  auto operator=(const ModuleAttributionScope&)
      -> ModuleAttributionScope& = delete;
  ModuleAttributionScope(ModuleAttributionScope&&) = delete;
  auto operator=(ModuleAttributionScope&&) -> ModuleAttributionScope& = delete;

 private:
  const char* previous_ = nullptr;
  bool bracketed_ = false;
};

}  // namespace GpgFrontend::Module
