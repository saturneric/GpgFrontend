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
#include <cstring>

#include "GpgFrontendTest.h"
#include "core/SdkTestContext.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKBuffer.hpp"

/**
 * @file GpgCoreTestSdkBuffer.cpp
 * @brief The SDK buffer handle's ownership contract, as executable rules.
 *
 * These deliberately go through the PUBLIC SDK headers only. A test here that
 * needs a core internal is a signal that the surface is wrong.
 */

namespace GpgFrontend::Test {

namespace {

/// One granted context for this file, minted the way the module loader
/// mints one. Process-lifetime on purpose: a grant is retired, never
/// freed, so that a stale caller is refused rather than following a
/// dangling pointer.
auto Ctx() -> GFSDKContext* {
  static SdkTestContext context("com.example.sdk.buffer");
  return context.get();
}

}  // namespace

TEST(SdkBufferTest, RoundTripsBytesVerbatim) {
  const QByteArray payload = "hello openpgp";

  auto* buf = GFBufferNewFromBytes(Ctx(), payload.constData(),
                                   static_cast<size_t>(payload.size()));
  ASSERT_NE(buf, nullptr);

  EXPECT_EQ(GFBufferSize(Ctx(), buf), static_cast<size_t>(payload.size()));
  EXPECT_EQ(std::memcmp(GFBufferData(Ctx(), buf), payload.constData(),
                        payload.size()),
            0);

  GFBufferRelease(Ctx(), buf);
}

// The defect the whole *N family was added to work around. A NUL in the
// middle of a MIME entity or a signature is an ordinary octet, and stopping
// there means verifying a prefix of what the user was shown.
TEST(SdkBufferTest, EmbeddedNulsSurviveARoundTrip) {
  QByteArray payload("a\0b\0\0c", 6);
  ASSERT_EQ(payload.size(), 6);

  auto* buf = GFBufferNewFromBytes(Ctx(), payload.constData(), 6);
  ASSERT_NE(buf, nullptr);

  EXPECT_EQ(GFBufferSize(Ctx(), buf), 6U);
  EXPECT_EQ(std::memcmp(GFBufferData(Ctx(), buf), payload.constData(), 6), 0);

  GFBufferRelease(Ctx(), buf);
}

TEST(SdkBufferTest, AnEmptyBufferIsNotAnError) {
  auto* buf = GFBufferNewFromBytes(Ctx(), "", 0);
  ASSERT_NE(buf, nullptr);
  EXPECT_EQ(GFBufferSize(Ctx(), buf), 0U);
  GFBufferRelease(Ctx(), buf);
}

TEST(SdkBufferTest, ANullSourceWithANonZeroSizeIsRejected) {
  EXPECT_EQ(GFBufferNewFromBytes(Ctx(), nullptr, 16), nullptr);
}

TEST(SdkBufferTest, EveryEntryPointToleratesANullHandle) {
  EXPECT_EQ(GFBufferData(Ctx(), nullptr), nullptr);
  EXPECT_EQ(GFBufferSize(Ctx(), nullptr), 0U);
  GFBufferZeroize(Ctx(), nullptr);  // must not crash
  GFBufferRelease(Ctx(), nullptr);  // must not crash
}

TEST(SdkBufferTest, ZeroizeErasesTheContentsWhileTheHandleIsStillLive) {
  const QByteArray secret = "correct horse battery staple";

  auto* buf = GFBufferNewFromBytes(Ctx(), secret.constData(),
                                   static_cast<size_t>(secret.size()));
  ASSERT_NE(buf, nullptr);

  GFBufferZeroize(Ctx(), buf);

  // Size is unchanged -- Zeroize erases, it does not shrink.
  ASSERT_EQ(GFBufferSize(Ctx(), buf), static_cast<size_t>(secret.size()));

  const auto* bytes = static_cast<const char*>(GFBufferData(Ctx(), buf));
  ASSERT_NE(bytes, nullptr);
  for (int i = 0; i < secret.size(); ++i) {
    EXPECT_EQ(bytes[i], '\0') << "byte " << i << " survived Zeroize";
  }

  GFBufferRelease(Ctx(), buf);
}

// The ledger is only worth having if it is pinned: this is the property the
// whole per-module accounting layer exists to provide.
TEST(SdkBufferTest, TheLedgerBalancesAcrossCreateAndRelease) {
  const auto before = GFBufferOutstandingCount(Ctx());

  auto* a = GFBufferNewFromBytes(Ctx(), "one", 3);
  auto* b = GFBufferNewFromBytes(Ctx(), "two", 3);
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before + 2);

  GFBufferRelease(Ctx(), a);
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before + 1);

  GFBufferRelease(Ctx(), b);
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before);
}

// Releasing twice must be REPORTED, not corrupt the heap. The detection is a
// registry lookup on the pointer value, so nothing dereferences the freed
// block to reach this verdict -- reading a magic word out of released memory
// would itself be the undefined behaviour we are trying to catch.
TEST(SdkBufferTest, ADoubleReleaseIsDetectedRatherThanCorruptingTheHeap) {
  auto* buf = GFBufferNewFromBytes(Ctx(), "payload", 7);
  ASSERT_NE(buf, nullptr);

  const auto before = GFBufferOutstandingCount(Ctx());
  GFBufferRelease(Ctx(), buf);
  ASSERT_EQ(GFBufferOutstandingCount(Ctx()), before - 1);

  // Same policy as SecureMemoryAllocator::report_invalid_free, and the same
  // test shape as SecureMemoryAllocatorTest.DoubleFreeShouldWarn: a release
  // build warns and carries on, a debug build fails fast so the offending
  // call site is caught. Either way the verdict is reached from the registry
  // WITHOUT dereferencing the freed handle.
#ifdef DEBUG
  EXPECT_DEATH({ GFBufferRelease(Ctx(), buf); }, "");
#else
  GFBufferRelease(Ctx(), buf);
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before - 1);
#endif
}

TEST(SdkBufferTest, AForeignPointerIsNotTreatedAsAHandle) {
  int not_a_handle = 0;
  auto* bogus = reinterpret_cast<GFBufferRef>(&not_a_handle);

  // The point: these answer without ever dereferencing `bogus`. A design that
  // read a magic word out of the candidate would be reading a stack int here,
  // and freed heap in the double-release case.
#ifdef DEBUG
  EXPECT_DEATH({ (void)GFBufferData(Ctx(), bogus); }, "");
#else
  EXPECT_EQ(GFBufferData(Ctx(), bogus), nullptr);
  EXPECT_EQ(GFBufferSize(Ctx(), bogus), 0U);

  const auto before = GFBufferOutstandingCount(Ctx());
  GFBufferRelease(Ctx(), bogus);
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before);
#endif
}

/* --- the C++ RAII wrapper ------------------------------------------------ */

TEST(SdkBufferRaiiTest, ScopeExitReleasesTheHandle) {
  const auto before = GFBufferOutstandingCount(Ctx());
  {
    auto buf = GFBuf::Copy(Ctx(), QByteArray("scoped"));
    ASSERT_TRUE(static_cast<bool>(buf));
    EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before + 1);
  }
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before);
}

TEST(SdkBufferRaiiTest, MovingTransfersOwnershipExactlyOnce) {
  const auto before = GFBufferOutstandingCount(Ctx());
  {
    auto a = GFBuf::Copy(Ctx(), QByteArray("moved"));
    ASSERT_EQ(GFBufferOutstandingCount(Ctx()), before + 1);

    auto b = std::move(a);
    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_TRUE(static_cast<bool>(b));

    // Still exactly one live handle: a move transfers, it does not duplicate.
    EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before + 1);
  }
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before);
}

TEST(SdkBufferRaiiTest, OutReleasesWhateverWasAlreadyHeld) {
  const auto before = GFBufferOutstandingCount(Ctx());
  {
    auto buf = GFBuf::Copy(Ctx(), QByteArray("first"));
    ASSERT_EQ(GFBufferOutstandingCount(Ctx()), before + 1);

    *buf.Out() = GFBufferNewFromBytes(Ctx(), "second", 6);
    // The first handle was reclaimed by Out(), so still exactly one.
    EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before + 1);
    EXPECT_EQ(buf.Size(), 6U);
  }
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before);
}

TEST(SdkBufferRaiiTest, TakeHandsOwnershipToTheCaller) {
  const auto before = GFBufferOutstandingCount(Ctx());

  GFBufferRef raw = nullptr;
  {
    auto buf = GFBuf::Copy(Ctx(), QByteArray("taken"));
    raw = buf.Take();
    EXPECT_FALSE(static_cast<bool>(buf));
  }
  // The wrapper went out of scope but must NOT have released what it gave up.
  ASSERT_NE(raw, nullptr);
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before + 1);

  GFBufferRelease(Ctx(), raw);
  EXPECT_EQ(GFBufferOutstandingCount(Ctx()), before);
}

TEST(SdkBufferRaiiTest, UnwipeableCopyReproducesTheBytesIncludingNuls) {
  QByteArray payload("x\0y", 3);
  auto buf = GFBuf::Copy(Ctx(), payload);
  ASSERT_TRUE(static_cast<bool>(buf));

  const auto copy = buf.UnwipeableCopy();
  EXPECT_EQ(copy.size(), 3);
  EXPECT_EQ(copy, payload);
}

TEST(SdkBufferRaiiTest, ADefaultConstructedWrapperIsSafeToUse) {
  GFBuf buf(Ctx());
  EXPECT_FALSE(static_cast<bool>(buf));
  EXPECT_EQ(buf.Size(), 0U);
  EXPECT_EQ(buf.Data(), nullptr);
  EXPECT_TRUE(buf.Empty());
  EXPECT_TRUE(buf.UnwipeableCopy().isEmpty());
  buf.Zeroize();  // must not crash
}

}  // namespace GpgFrontend::Test
