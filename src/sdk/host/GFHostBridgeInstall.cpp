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

#include "core/module/ModuleSdkBridge.h"
#include "private/GFHostAttribution.h"
#include "private/GFHostContext.h"
#include "sdk/GFSDKModuleApi.h"

/**
 * @file GFHostBridgeInstall.cpp
 * @brief Hand gf_core the SDK entry points it needs, at load time.
 *
 * gf_sdk links gf_core, so it may call into it freely. gf_core must not call
 * back, or the two libraries become mutually dependent and neither a MinGW
 * DLL nor a Mach-O dylib can be linked at all -- see ModuleSdkBridge.h for
 * the whole story. This file is the one place the arrow is drawn.
 *
 * ## Why an explicit call rather than a load-time initializer
 *
 * This used to install itself from a load-time initializer, and the reason
 * given was that there is no point in startup that is reliably both after
 * `libgf_sdk` exists and before the first module activation -- the SDK was
 * dragged in by a module's own `DT_NEEDED`, in the middle of module loading.
 *
 * That reasoning expired when modules stopped linking the host half at all.
 * Nothing drags this in now except the application's own link line, and with
 * `--as-needed` a library from which no symbol is referenced can be dropped
 * outright, taking its initializer with it. The host half is therefore an
 * OBJECT library inside the application and installs itself from one explicit
 * call, which is deterministic and says when it happens.
 *
 * Because caller and implementation end up in the same link unit, the entry
 * point needs no dynamic visibility: `GFModuleGetApi` remains the only symbol
 * anything in this system intentionally exports.
 */

namespace {

auto InstallBridge() -> bool {
  GpgFrontend::Module::ModuleSdkBridge bridge;
  // Type-erased on the way across: gf_core's header deliberately does not
  // include the SDK's, so the host table travels as a const void* and is cast
  // back by its single consumer.
  //
  // A mint rather than a getter. A getter hands the same table to everyone,
  // which makes a per-module grant a suggestion; this one takes the module and
  // its granted set and produces a table that has only what that module was
  // given, bound to a context every call carries.
  bridge.mint_host_api = [](const char* module_id,
                            uint32_t granted) -> const void* {
    return gf_sdk_internal::MintHostApi(module_id, granted);
  };
  bridge.release_host_api = [](const char* module_id) {
    gf_sdk_internal::ReleaseHostApi(module_id);
  };
  bridge.enter_module = &GFSdkEnterModule;
  bridge.current_module = &GFSdkCurrentModule;
  bridge.leave_module = &GFSdkLeaveModule;
  bridge.sweep_module_handles = &GFSdkSweepModuleHandles;
  GpgFrontend::Module::InstallModuleSdkBridge(bridge);
  return true;
}

}  // namespace

void GFHostApiInstallBridge() {
  // Idempotent: installing twice writes the same table. Called once from
  // application startup, before anything can load a module.
  static const bool kInstalled = InstallBridge();
  (void)kInstalled;
}
