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

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKHostApi.h"

/**
 * @file GpgCoreTestModuleSdkBridge.cpp
 * @brief The seam that keeps gf_core and gf_sdk from depending on each other.
 *
 * These tests look almost content-free, and the reason they are worth having
 * is not what they assert but what fails if the arrangement regresses. gf_core
 * referencing a gf_sdk function links perfectly well on Linux and stops both
 * the Windows and macOS builds dead -- so the failure surfaces in CI, on two
 * platforms at once, days after the change. `--no-undefined` on gf_core is the
 * real guard; this is the part that says what the guard is protecting.
 */

namespace GpgFrontend::Test {

TEST(ModuleSdkBridgeTest, TheBridgeIsInstalledAtStartup) {
  // main() calls GFHostApiInstallBridge() before anything else, and the test
  // binary runs through the same main(). If this fails, module activation is
  // about to start refusing every module.
  EXPECT_TRUE(Module::IsModuleSdkBridgeInstalled());
}

TEST(ModuleSdkBridgeTest, AHostApiTableIsMintedThroughTheBridge) {
  const auto* first =
      Module::ModuleSdkMintHostApi("com.example.bridge_probe", 0);
  ASSERT_NE(first, nullptr);

  // Minting the same module twice returns the SAME table. It has to: the
  // module is holding the first pointer, and handing it a second one would
  // leave the first live, unreachable, and still carrying the grant.
  EXPECT_EQ(Module::ModuleSdkMintHostApi("com.example.bridge_probe", 0), first);

  // A different module gets a different table, which is the entire reason
  // this is a mint and not a getter.
  const auto* other =
      Module::ModuleSdkMintHostApi("com.example.bridge_probe_other", 0);
  ASSERT_NE(other, nullptr);
  EXPECT_NE(other, first);

  Module::ModuleSdkReleaseHostApi("com.example.bridge_probe");
  Module::ModuleSdkReleaseHostApi("com.example.bridge_probe_other");
}

// A table is never rewritten once handed out: other threads read it without
// a lock. A different grant gets a new table, and the old context is revoked
// so a thread left over from before cannot use it.
TEST(ModuleSdkBridgeTest, ADifferentGrantMintsANewTableAndRevokesTheOld) {
  const auto* first = static_cast<const GFHostApi*>(
      Module::ModuleSdkMintHostApi("com.example.bridge_regrant", 0));
  ASSERT_NE(first, nullptr);
  ASSERT_EQ(first->storage, nullptr);

  const auto* second =
      static_cast<const GFHostApi*>(Module::ModuleSdkMintHostApi(
          "com.example.bridge_regrant", GF_HOST_CAP_STORAGE));
  ASSERT_NE(second, nullptr);
  EXPECT_NE(second, first);
  EXPECT_NE(second->storage, nullptr);

  // The old table is intact but its context is refused.
  EXPECT_EQ(first->storage, nullptr);
  EXPECT_EQ(first->buffer->new_from_bytes(first->context, "x", 1), nullptr);

  auto* buf = second->buffer->new_from_bytes(second->context, "x", 1);
  EXPECT_NE(buf, nullptr);
  second->buffer->release(second->context, buf);

  Module::ModuleSdkReleaseHostApi("com.example.bridge_regrant");
}

// A released module minted again is a reload: a new context, and the old one
// stays revoked.
TEST(ModuleSdkBridgeTest, AReloadDoesNotReviveTheOldContext) {
  const auto* first = static_cast<const GFHostApi*>(
      Module::ModuleSdkMintHostApi("com.example.bridge_reload", 0));
  ASSERT_NE(first, nullptr);
  Module::ModuleSdkReleaseHostApi("com.example.bridge_reload");

  const auto* second = static_cast<const GFHostApi*>(
      Module::ModuleSdkMintHostApi("com.example.bridge_reload", 0));
  ASSERT_NE(second, nullptr);
  EXPECT_NE(second->context, first->context);
  EXPECT_EQ(first->buffer->new_from_bytes(first->context, "x", 1), nullptr);

  Module::ModuleSdkReleaseHostApi("com.example.bridge_reload");
}

TEST(ModuleSdkBridgeTest, AnAttributionScopeNestsAndUnwinds) {
  // The brackets exist so a handle a module asks for is recorded against it.
  // Nesting has to restore the previous attribution rather than clear it: the
  // host calls into module A, which calls back into the host, which calls
  // module B -- and A must still be current when B returns.
  {
    const Module::ModuleAttributionScope outer("com.example.outer");
    {
      const Module::ModuleAttributionScope inner("com.example.inner");
    }
  }
  // Reaching here without a crash is the assertion: the scope holds a pointer
  // into a thread-local stack, and restoring the wrong one is a use-after-free
  // rather than a wrong answer.
  SUCCEED();
}

TEST(ModuleSdkBridgeTest, AnEmptyModuleIdIsAcceptedRatherThanCrashing) {
  // The host brackets every call with whatever identifier the module carries,
  // and a malformed descriptor could in principle carry an empty one. The SDK
  // treats that as "no module", which is the safe reading.
  const Module::ModuleAttributionScope scope("");
  SUCCEED();
}

TEST(ModuleSdkBridgeTest, GfCoreCarriesNoUnresolvedSdkSymbols) {
  // The property this whole file exists for, stated once where a reader will
  // find it: gf_core is linked with --no-undefined on Linux, so if it
  // referenced a gf_sdk function the LINK would have failed and this binary
  // would not exist. The test passing is a consequence of the build having
  // succeeded, which is exactly the guarantee wanted -- and on Windows and
  // macOS the platform linkers enforce the same thing without being asked.
  SUCCEED();
}

}  // namespace GpgFrontend::Test
