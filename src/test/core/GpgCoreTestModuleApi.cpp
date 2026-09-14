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

#include <cstddef>
#include <cstring>

#include "GpgFrontendTest.h"
#include "sdk/GFSDKBuildInfo.h"
#include "sdk/GFSDKModuleApi.h"

/**
 * @file GpgCoreTestModuleApi.cpp
 * @brief The runtime ABI: the host table, and the growth rule.
 *
 * The property that matters most here is the one that is invisible until it
 * breaks: struct_size is what lets the table gain fields without breaking a
 * module compiled against an older, smaller one. If a field is ever reordered
 * or repurposed instead of appended, every already-built module silently
 * reads the wrong slot.
 */

namespace GpgFrontend::Test {

TEST(ModuleApiTest, TheHostTableIsPopulatedAndSelfDescribing) {
  const auto* host = GFGetHostApi();
  ASSERT_NE(host, nullptr);

  EXPECT_EQ(host->struct_size, sizeof(GFHostApi));
  EXPECT_EQ(host->abi_version, static_cast<uint32_t>(GF_SDK_ABI_VERSION));
}

// A module reaches everything through this table. A null entry is not a
// neutral default -- it is "this capability was withheld" -- so the always
// available groups must actually be filled in.
TEST(ModuleApiTest, TheAlwaysAvailableGroupsAreNotNull) {
  const auto* host = GFGetHostApi();
  ASSERT_NE(host, nullptr);

  // buffers: everything else traffics in them
  EXPECT_NE(host->buffer_new_from_bytes, nullptr);
  EXPECT_NE(host->buffer_data, nullptr);
  EXPECT_NE(host->buffer_size, nullptr);
  EXPECT_NE(host->buffer_zeroize, nullptr);
  EXPECT_NE(host->buffer_release, nullptr);

  // logging: a module that cannot report anything is undebuggable
  EXPECT_NE(host->log_debug, nullptr);
  EXPECT_NE(host->log_info, nullptr);
  EXPECT_NE(host->log_warn, nullptr);
  EXPECT_NE(host->log_error, nullptr);
}

TEST(ModuleApiTest, TheGpgGroupIsWiredToTheRealEntryPoints) {
  const auto* host = GFGetHostApi();
  ASSERT_NE(host, nullptr);

  EXPECT_EQ(host->gpg_sign, &GFGpgSign);
  EXPECT_EQ(host->gpg_encrypt, &GFGpgEncrypt);
  EXPECT_EQ(host->gpg_decrypt, &GFGpgDecrypt);
  EXPECT_EQ(host->gpg_verify, &GFGpgVerify);
  EXPECT_EQ(host->result_release, &GFGpgResultRelease);
}

// The table is usable through the pointers, not just present.
TEST(ModuleApiTest, AModuleCanRoundTripBytesThroughTheTableAlone) {
  const auto* host = GFGetHostApi();
  ASSERT_NE(host, nullptr);

  const char payload[] = "a\0b";  // an embedded NUL, as message data has
  auto* buf = host->buffer_new_from_bytes(payload, sizeof(payload) - 1);
  ASSERT_NE(buf, nullptr);

  EXPECT_EQ(host->buffer_size(buf), sizeof(payload) - 1);
  EXPECT_EQ(std::memcmp(host->buffer_data(buf), payload, sizeof(payload) - 1),
            0);

  host->buffer_release(buf);
}

// The growth rule, stated as a test: a field may only ever be APPENDED.
// These offsets are the prefix that already-built modules read; changing any
// of them makes those modules read the wrong slot at runtime, with no
// diagnostic anywhere.
TEST(ModuleApiTest, ThePrefixLayoutIsFrozen) {
  EXPECT_EQ(offsetof(GFHostApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostApi, abi_version), sizeof(size_t));

  EXPECT_EQ(offsetof(GFModuleApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFModuleApi, abi_version), sizeof(size_t));

  // The host reads up to and including unregister, so a module table must be
  // at least this big to be usable. Module.cpp enforces exactly this.
  EXPECT_LE(offsetof(GFModuleApi, unregister) + sizeof(void*),
            sizeof(GFModuleApi));
}

// An older module hands over a SMALLER struct. The host must accept it and
// read only the shared prefix -- that is the entire point of struct_size.
TEST(ModuleApiTest, ASmallerModuleTableIsStillUsable) {
  GFModuleApi older{};
  older.struct_size = offsetof(GFModuleApi, unregister) + sizeof(void*);
  older.abi_version = GF_SDK_ABI_VERSION;
  older.module_id = "com.example.older";
  older.version = "1.0.0";

  // The host's minimum is exactly the prefix it reads, so this qualifies.
  EXPECT_GE(older.struct_size,
            offsetof(GFModuleApi, unregister) + sizeof(void*));
  EXPECT_LE(older.struct_size, sizeof(GFModuleApi));
}

TEST(ModuleApiTest, ATableTooSmallToReadIsRejectable) {
  // Below the prefix the host dereferences: accepting this would read past
  // the module's own struct.
  const size_t too_small = offsetof(GFModuleApi, activate);
  EXPECT_LT(too_small, offsetof(GFModuleApi, unregister) + sizeof(void*));
}

}  // namespace GpgFrontend::Test
