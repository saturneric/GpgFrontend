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

#include <QByteArray>
#include <thread>

#include "GpgFrontendTest.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKModuleAttribution.h"

/**
 * @file GpgCoreTestSdkLedger.cpp
 * @brief Per-module handle attribution and the unload sweep, as rules.
 *
 * These go through the public SDK headers only, which is what a module sees.
 * They deliberately use buffers rather than crypto results: attribution and
 * the sweep are properties of the ledger, not of gpg, and a sharded run has no
 * engine to sign anything with.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto* kModuleA = "com.bktus.gpgfrontend.module.ledger_test_a";
constexpr auto* kModuleB = "com.bktus.gpgfrontend.module.ledger_test_b";

auto MakeBuffer() -> GFBufferRef {
  const QByteArray payload = "ledger";
  return GFBufferNewFromBytes(payload.constData(),
                              static_cast<size_t>(payload.size()));
}

}  // namespace

TEST(SdkLedgerTest, NoModuleIsCurrentOutsideAModuleCall) {
  // The application's own calls are not a module's, and must not be swept as
  // one. A null answer here is what keeps them out of every module's ledger.
  EXPECT_EQ(GFSdkCurrentModule(), nullptr);
}

TEST(SdkLedgerTest, AttributionIsScopedAndNests) {
  {
    GFSdkModuleAttributionScope outer(kModuleA);
    EXPECT_STREQ(GFSdkCurrentModule(), kModuleA);
    {
      GFSdkModuleAttributionScope inner(kModuleB);
      EXPECT_STREQ(GFSdkCurrentModule(), kModuleB);
    }
    // Restored, not cleared: the host may call into one module from inside
    // another, and the outer call's handles are still the outer module's.
    EXPECT_STREQ(GFSdkCurrentModule(), kModuleA);
  }
  EXPECT_EQ(GFSdkCurrentModule(), nullptr);
}

TEST(SdkLedgerTest, AttributionIsPerThread) {
  // Two modules can be running at once on different runners. If the current
  // module were process-wide, one would collect the other's handles and the
  // sweep would free memory that is still in use.
  GFSdkModuleAttributionScope here(kModuleA);
  ASSERT_STREQ(GFSdkCurrentModule(), kModuleA);

  const char* seen_elsewhere = kModuleA;
  std::thread other(
      [&seen_elsewhere]() { seen_elsewhere = GFSdkCurrentModule(); });
  other.join();

  EXPECT_EQ(seen_elsewhere, nullptr);
  EXPECT_STREQ(GFSdkCurrentModule(), kModuleA);
}

TEST(SdkLedgerTest, AHandleIsRecordedAgainstTheModuleThatAskedForIt) {
  ASSERT_EQ(GFBufferOutstandingCount(kModuleA), 0U);

  GFBufferRef buf = nullptr;
  {
    GFSdkModuleAttributionScope attributed(kModuleA);
    buf = MakeBuffer();
    ASSERT_NE(buf, nullptr);
  }

  // Outside the scope, and still that module's: attribution is fixed when the
  // handle is issued, not read again when it is counted.
  EXPECT_EQ(GFBufferOutstandingCount(kModuleA), 1U);
  EXPECT_EQ(GFBufferOutstandingCount(kModuleB), 0U);

  GFBufferRelease(buf);
  EXPECT_EQ(GFBufferOutstandingCount(kModuleA), 0U);
}

TEST(SdkLedgerTest, AHandleCreatedOutsideAModuleCallBelongsToNoModule) {
  // The honest limit, pinned so nobody later reads the sweep as covering more
  // than it does: a handle made on a thread that never came through the host
  // has no owner, and the unload sweep will not reclaim it.
  auto* buf = MakeBuffer();
  ASSERT_NE(buf, nullptr);

  EXPECT_EQ(GFBufferOutstandingCount(kModuleA), 0U);
  EXPECT_EQ(GFSdkSweepModuleHandles(kModuleA), 0U);

  // Still live, and still the caller's to release.
  EXPECT_EQ(GFBufferSize(buf), 6U);
  GFBufferRelease(buf);
}

TEST(SdkLedgerTest, TheSweepReclaimsWhatAModuleLeaked) {
  ASSERT_EQ(GFBufferOutstandingCount(kModuleA), 0U);

  {
    GFSdkModuleAttributionScope attributed(kModuleA);
    // Deliberately dropped on the floor: this is the leak the sweep exists to
    // catch, and the point is that it is reclaimed rather than merely counted.
    (void)MakeBuffer();
    (void)MakeBuffer();
  }
  ASSERT_EQ(GFBufferOutstandingCount(kModuleA), 2U);

  EXPECT_EQ(GFSdkSweepModuleHandles(kModuleA), 2U);
  EXPECT_EQ(GFBufferOutstandingCount(kModuleA), 0U);

  // Idempotent: a second teardown pass has nothing left to do.
  EXPECT_EQ(GFSdkSweepModuleHandles(kModuleA), 0U);
}

TEST(SdkLedgerTest, TheSweepTouchesOnlyTheModuleBeingTornDown) {
  GFBufferRef kept = nullptr;
  {
    GFSdkModuleAttributionScope attributed(kModuleB);
    kept = MakeBuffer();
    ASSERT_NE(kept, nullptr);
  }
  {
    GFSdkModuleAttributionScope attributed(kModuleA);
    (void)MakeBuffer();
  }

  ASSERT_EQ(GFSdkSweepModuleHandles(kModuleA), 1U);

  // B's handle survived A's teardown and is still usable. Getting this wrong
  // would free memory a running module is still reading.
  EXPECT_EQ(GFBufferOutstandingCount(kModuleB), 1U);
  EXPECT_EQ(GFBufferSize(kept), 6U);
  GFBufferRelease(kept);
}

TEST(SdkLedgerTest, SweepingNothingIsHarmless) {
  EXPECT_EQ(GFSdkSweepModuleHandles(nullptr), 0U);
  EXPECT_EQ(GFSdkSweepModuleHandles(""), 0U);
  EXPECT_EQ(GFSdkSweepModuleHandles("com.example.never.loaded"), 0U);
}

TEST(SdkLedgerTest, ProcessWideCountStillCountsEverything) {
  // GFBufferOutstandingCount(nullptr) asks about the process rather than a
  // module, and attribution must not have quietly changed that.
  const auto before = GFBufferOutstandingCount(nullptr);

  GFSdkModuleAttributionScope attributed(kModuleA);
  auto* buf = MakeBuffer();
  ASSERT_NE(buf, nullptr);
  EXPECT_EQ(GFBufferOutstandingCount(nullptr), before + 1);

  GFBufferRelease(buf);
  EXPECT_EQ(GFBufferOutstandingCount(nullptr), before);
}

}  // namespace GpgFrontend::Test
