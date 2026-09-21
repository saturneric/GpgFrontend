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

#include "ModuleSdkBridge.h"

namespace GpgFrontend::Module {

namespace {

/// The one table. A plain function-local static rather than a namespace-scope
/// object: gf_sdk installs from a load-time initializer, and a function-local
/// static is constructed on first use rather than in an order nothing pins.
auto Bridge() -> ModuleSdkBridge& {
  static ModuleSdkBridge bridge;
  return bridge;
}

}  // namespace

void InstallModuleSdkBridge(const ModuleSdkBridge& bridge) {
  Bridge() = bridge;
}

auto IsModuleSdkBridgeInstalled() -> bool {
  return Bridge().mint_host_api != nullptr;
}

auto ModuleSdkMintHostApi(const char* module_id, uint32_t granted) -> const
    void* {
  const auto& bridge = Bridge();
  if (bridge.mint_host_api == nullptr) return nullptr;
  return bridge.mint_host_api(module_id, granted);
}

void ModuleSdkReleaseHostApi(const char* module_id) {
  const auto& bridge = Bridge();
  if (bridge.release_host_api == nullptr) return;
  bridge.release_host_api(module_id);
}

auto ModuleSdkSweepHandles(const char* module_id) -> size_t {
  const auto& bridge = Bridge();
  if (bridge.sweep_module_handles == nullptr) return 0;
  return bridge.sweep_module_handles(module_id);
}

ModuleAttributionScope::ModuleAttributionScope(const char* module_id) {
  const auto& bridge = Bridge();
  if (bridge.enter_module == nullptr || bridge.leave_module == nullptr) return;
  previous_ = bridge.enter_module(module_id);
  // Recorded rather than re-checked in the destructor: the table is installed
  // once and never withdrawn, but a scope that entered must leave through the
  // same pair it entered with, whatever happens in between.
  bracketed_ = true;
}

ModuleAttributionScope::~ModuleAttributionScope() {
  if (!bracketed_) return;
  Bridge().leave_module(previous_);
}

}  // namespace GpgFrontend::Module
