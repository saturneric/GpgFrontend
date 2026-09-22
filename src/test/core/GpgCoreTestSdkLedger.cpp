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
#include "core/SdkTestContext.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKBuffer.h"

/**
 * @file GpgCoreTestSdkLedger.cpp
 * @brief Per-module handle attribution and the unload sweep, as rules.
 *
 * ## What changed, and why these tests look different now
 *
 * Attribution used to be read from a thread-local that the host set while it
 * was calling into module code. That meant a handle created on a module's own
 * worker thread belonged to nobody, and the sweep could not reclaim it -- a
 * limit the old tests pinned as an honest gap.
 *
 * The gap is gone. Every SDK call carries the module's context, and the host
 * attributes the handle from that, so WHICH CONTEXT a buffer was made with is
 * the whole answer. These tests therefore mint two contexts and use them,
 * where they used to push and pop an attribution scope.
 *
 * They deliberately use buffers rather than crypto results: attribution and
 * the sweep are properties of the ledger, not of gpg, and a sharded run has
 * no engine to sign anything with.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto* kModuleA = "com.bktus.gpgfrontend.module.ledger_test_a";
constexpr auto* kModuleB = "com.bktus.gpgfrontend.module.ledger_test_b";

/// Two modules, two grants, exactly as the loader would mint them.
auto CtxA() -> GFSDKContext* {
  static SdkTestContext context(kModuleA);
  return context.get();
}

auto CtxB() -> GFSDKContext* {
  static SdkTestContext context(kModuleB);
  return context.get();
}

auto MakeBuffer(GFSDKContext* ctx) -> GFBufferRef {
  const QByteArray payload = "ledger";
  return GFBufferNewFromBytes(ctx, payload.constData(),
                              static_cast<size_t>(payload.size()));
}

}  // namespace

TEST(SdkLedgerTest, AHandleIsRecordedAgainstTheContextThatAskedForIt) {
  ASSERT_EQ(GFBufferOutstandingCount(CtxA()), 0U);
  ASSERT_EQ(GFBufferOutstandingCount(CtxB()), 0U);

  auto* buf = MakeBuffer(CtxA());
  ASSERT_NE(buf, nullptr);

  EXPECT_EQ(GFBufferOutstandingCount(CtxA()), 1U);
  EXPECT_EQ(GFBufferOutstandingCount(CtxB()), 0U)
      << "a handle belongs to the module whose context made it, and to no "
         "other";

  GFBufferRelease(CtxA(), buf);
  EXPECT_EQ(GFBufferOutstandingCount(CtxA()), 0U);
}

TEST(SdkLedgerTest, AHandleMadeOnAWorkerThreadIsStillAttributed) {
  // The case the old design could not serve at all: no host frame anywhere on
  // this thread's stack, so a thread-local answer would have been "nobody"
  // and the sweep would have missed the handle entirely.
  ASSERT_EQ(GFBufferOutstandingCount(CtxA()), 0U);

  GFBufferRef buf = nullptr;
  std::thread worker([&buf]() { buf = MakeBuffer(CtxA()); });
  worker.join();

  ASSERT_NE(buf, nullptr);
  EXPECT_EQ(GFBufferOutstandingCount(CtxA()), 1U)
      << "attribution must come from the context, not from the call stack";

  GFBufferRelease(CtxA(), buf);
}

TEST(SdkLedgerTest, TheSweepReclaimsWhatAModuleLeaked) {
  ASSERT_EQ(GFBufferOutstandingCount(CtxA()), 0U);

  // Deliberately dropped on the floor: this is the leak the sweep exists to
  // catch, and the point is that it is reclaimed rather than merely counted.
  (void)MakeBuffer(CtxA());
  (void)MakeBuffer(CtxA());
  ASSERT_EQ(GFBufferOutstandingCount(CtxA()), 2U);

  EXPECT_EQ(Module::ModuleSdkSweepHandles(kModuleA), 2U);
  EXPECT_EQ(GFBufferOutstandingCount(CtxA()), 0U);

  // Idempotent: a second teardown pass has nothing left to do.
  EXPECT_EQ(Module::ModuleSdkSweepHandles(kModuleA), 0U);
}

TEST(SdkLedgerTest, TheSweepTouchesOnlyTheModuleBeingTornDown) {
  auto* kept = MakeBuffer(CtxB());
  ASSERT_NE(kept, nullptr);

  (void)MakeBuffer(CtxA());
  ASSERT_EQ(GFBufferOutstandingCount(CtxA()), 1U);
  ASSERT_EQ(GFBufferOutstandingCount(CtxB()), 1U);

  EXPECT_EQ(Module::ModuleSdkSweepHandles(kModuleA), 1U);

  EXPECT_EQ(GFBufferOutstandingCount(CtxA()), 0U);
  EXPECT_EQ(GFBufferOutstandingCount(CtxB()), 1U)
      << "one module unloading must not reclaim another's handles";

  // Still usable, which is the half a count alone would not prove.
  EXPECT_EQ(GFBufferSize(CtxB(), kept), 6U);
  GFBufferRelease(CtxB(), kept);
}

TEST(SdkLedgerTest, SweepingNothingIsHarmless) {
  EXPECT_EQ(Module::ModuleSdkSweepHandles("com.example.never.loaded"), 0U);
  EXPECT_EQ(Module::ModuleSdkSweepHandles(nullptr), 0U);
}

TEST(SdkLedgerTest, AttributionRemainsForDiagnostics) {
  // The thread-local is still there, still bracketing host-to-module calls,
  // and still the thing that names a module in a log line. What it no longer
  // does is decide anything: that is the context's job, on every thread.
  EXPECT_TRUE(Module::ModuleSdkCurrentModule().isEmpty());
  {
    const Module::ModuleAttributionScope outer(kModuleA);
    EXPECT_EQ(Module::ModuleSdkCurrentModule(), QString::fromUtf8(kModuleA));
    {
      const Module::ModuleAttributionScope inner(kModuleB);
      EXPECT_EQ(Module::ModuleSdkCurrentModule(), QString::fromUtf8(kModuleB));
    }
    EXPECT_EQ(Module::ModuleSdkCurrentModule(), QString::fromUtf8(kModuleA))
        << "nesting must restore the previous attribution, not clear it";
  }
  EXPECT_TRUE(Module::ModuleSdkCurrentModule().isEmpty());
}

TEST(SdkLedgerTest, AttributionIsPerThread) {
  const Module::ModuleAttributionScope attributed(kModuleA);
  ASSERT_EQ(Module::ModuleSdkCurrentModule(), QString::fromUtf8(kModuleA));

  QString seen_elsewhere = QString::fromUtf8(kModuleA);
  std::thread other([&seen_elsewhere]() {
    seen_elsewhere = Module::ModuleSdkCurrentModule();
  });
  other.join();

  EXPECT_TRUE(seen_elsewhere.isEmpty())
      << "a thread the host never entered is attributed to nobody";
  EXPECT_EQ(Module::ModuleSdkCurrentModule(), QString::fromUtf8(kModuleA));
}

}  // namespace GpgFrontend::Test
