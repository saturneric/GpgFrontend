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
#include "sdk/GFSDKModuleApi.h"
#include "sdk/GFSDKModuleAttribution.h"

/**
 * @file GFSDKBridgeInstall.cpp
 * @brief Hand gf_core the four SDK entry points it needs, at load time.
 *
 * gf_sdk links gf_core, so it may call into it freely. gf_core must not call
 * back, or the two libraries become mutually dependent and neither a MinGW
 * DLL nor a Mach-O dylib can be linked at all -- see ModuleSdkBridge.h for
 * the whole story. This file is the one place the arrow is drawn.
 *
 * ## Why a load-time initializer rather than a call from main()
 *
 * There is no point in the application's startup that is reliably before the
 * first module activation AND after gf_sdk exists -- on Linux the SDK is
 * pulled in by a module's own DT_NEEDED, which happens in the middle of module
 * loading. A load-time initializer sidesteps the question: whenever and
 * however libgf_sdk arrives, it registers itself before anything can call
 * through it, because nothing can call through it until it has arrived.
 *
 * The initializer touches only a function-local static in gf_core and
 * allocates nothing, so it is safe at this point in a process's life.
 */

namespace {

auto InstallBridge() -> bool {
  GpgFrontend::Module::ModuleSdkBridge bridge;
  // Type-erased on the way across: gf_core's header deliberately does not
  // include the SDK's, so the host table travels as a const void* and is cast
  // back by its single consumer.
  bridge.get_host_api = []() -> const void* { return GFGetHostApi(); };
  bridge.enter_module = &GFSdkEnterModule;
  bridge.leave_module = &GFSdkLeaveModule;
  bridge.sweep_module_handles = &GFSdkSweepModuleHandles;
  GpgFrontend::Module::InstallModuleSdkBridge(bridge);
  return true;
}

/// Runs when libgf_sdk is loaded, whichever way it was loaded.
const bool kBridgeInstalled = InstallBridge();

}  // namespace
