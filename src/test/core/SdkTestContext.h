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

#include "core/module/ModuleCapability.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKBuildInfo.h"
#include "sdk/GFSDKContext.h"

namespace GpgFrontend::Test {

/**
 * @file SdkTestContext.h
 * @brief A real, fully granted SDK context, for the C-ABI tests.
 *
 * These tests exercise the public SDK, which is module-side and stateless and
 * therefore needs a context. They get a REAL one: minted through gf_core's
 * bridge exactly as `Module::Impl::Active()` mints one, so a call made here
 * travels the whole chain -- public SDK, GFHostApi primitive, host
 * implementation -- rather than only the host half, which is all these tests
 * could reach before the split.
 *
 * Deliberately NOT linked against the host component directly: `gf_test` goes
 * through the bridge like everything else, because the context registry and
 * the handle ledgers are process-wide and a second copy would be a second
 * registry.
 */
class SdkTestContext {
 public:
  /// @param module_id the identity the grant is recorded against
  /// @param granted GF_HOST_CAP_* bits; everything, by default, because a
  ///        C-ABI ownership test is not about capabilities
  explicit SdkTestContext(const char* module_id,
                          uint32_t granted = GF_HOST_CAP_GPG | GF_HOST_CAP_PGP |
                                             GF_HOST_CAP_UI |
                                             GF_HOST_CAP_EDITOR |
                                             GF_HOST_CAP_STORAGE |
                                             GF_HOST_CAP_PROCESS)
      : module_id_(module_id) {
    const auto* host = static_cast<const GFHostApi*>(
        Module::ModuleSdkMintHostApi(module_id, granted));

    context_.struct_size = sizeof(GFSDKContext);
    context_.abi_version = GF_SDK_ABI_VERSION;
    context_.host = host;
    context_.module_id = module_id_;
  }

  ~SdkTestContext() { Module::ModuleSdkReleaseHostApi(module_id_); }

  SdkTestContext(const SdkTestContext&) = delete;
  auto operator=(const SdkTestContext&) -> SdkTestContext& = delete;

  auto operator()() -> GFSDKContext* { return &context_; }
  [[nodiscard]] auto get() -> GFSDKContext* { return &context_; }
  [[nodiscard]] auto live() const -> bool { return context_.host != nullptr; }

 private:
  const char* module_id_;
  GFSDKContext context_{};
};

}  // namespace GpgFrontend::Test
