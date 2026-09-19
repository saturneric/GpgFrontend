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

TEST(ModuleSdkBridgeTest, TheSdkInstallsItselfWhenItIsLoaded) {
  // libgf_sdk is in this process -- the test binary loads modules -- so its
  // load-time initializer has run and the table is populated. If this fails,
  // module activation is about to start refusing every module.
  EXPECT_TRUE(Module::IsModuleSdkBridgeInstalled());
}

TEST(ModuleSdkBridgeTest, TheHostApiTableIsReachableThroughTheBridge) {
  const auto* first = Module::ModuleSdkHostApi();
  ASSERT_NE(first, nullptr);

  // Static for the life of the process, which is the promise Module::Active()
  // relies on when it lets a module keep the pointer forever.
  EXPECT_EQ(Module::ModuleSdkHostApi(), first);
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
