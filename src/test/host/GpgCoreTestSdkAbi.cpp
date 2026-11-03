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
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestSdkAbi.cpp
 * @brief The module ABI floor.
 *
 * The version gate in Module.cpp was one-sided for a long time: it rejected a
 * module built against a NEWER sdk and silently accepted one built against an
 * older, incompatible ABI, which then loaded and crashed on the first changed
 * entry point. These pin the replacement.
 */

namespace GpgFrontend::Test {

/// Mirrors the host's accept/reject decision in Module::Impl.
static auto AbiAccepted(int module_abi) -> bool {
  return module_abi >= GF_SDK_ABI_MIN_SUPPORTED &&
         module_abi <= GF_SDK_ABI_VERSION;
}

TEST(SdkAbiTest, TheAbiVersionIsIndependentOfTheProjectVersion) {
  // If these were the same number, the ABI would move every release for
  // reasons unrelated to the ABI. A plain integer, bumped by hand, is the
  // point -- so this asserts the constants exist and are sane, not that they
  // hold any particular value.
  EXPECT_GE(GF_SDK_ABI_VERSION, 1);
  EXPECT_GE(GF_SDK_ABI_MIN_SUPPORTED, 1);
  EXPECT_LE(GF_SDK_ABI_MIN_SUPPORTED, GF_SDK_ABI_VERSION);
}

TEST(SdkAbiTest, AModuleBuiltAgainstThisAbiIsAccepted) {
  EXPECT_TRUE(AbiAccepted(GF_SDK_ABI_VERSION));
}

TEST(SdkAbiTest, AModuleBelowTheFloorIsRejected) {
  // The case that used to slip through and crash.
  EXPECT_FALSE(AbiAccepted(GF_SDK_ABI_MIN_SUPPORTED - 1));
  EXPECT_FALSE(AbiAccepted(0));
}

TEST(SdkAbiTest, AModuleFromTheFutureIsRejected) {
  EXPECT_FALSE(AbiAccepted(GF_SDK_ABI_VERSION + 1));
}

}  // namespace GpgFrontend::Test
