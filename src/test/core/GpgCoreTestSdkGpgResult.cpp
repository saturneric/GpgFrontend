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

#include "GpgFrontendTest.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKBuffer.hpp"
#include "sdk/GFSDKGpgResult.h"
#include "sdk/GFSDKGpgResult.hpp"

/**
 * @file GpgCoreTestSdkGpgResult.cpp
 * @brief The opaque gpg result handle's contract.
 *
 * The properties worth pinning are the ones the old struct-based API got
 * wrong: that a FAILED operation still hands back something owned (that is
 * where the explanation lives, and forgetting it is what leaked), that there
 * is nothing to free field by field, and that accessors are borrowed.
 */

namespace GpgFrontend::Test {

namespace {

/// Shards run against a throwaway profile with no agent, so an engine is not
/// guaranteed. Tests that need one skip rather than fail.
auto ChannelIsUsable(int channel) -> bool {
  return OpenPGPContext::GetInstance(channel).Good();
}

}  // namespace

TEST(SdkGpgResultTest, ANullOutParameterIsRejectedRatherThanDereferenced) {
  auto in = GFBuf::Copy(QByteArray("data"));

  EXPECT_LT(GFGpgDecrypt(0, in.View(), nullptr), 0);
  EXPECT_LT(GFGpgVerify(0, in.View(), nullptr, nullptr), 0);
  EXPECT_LT(GFGpgSign(0, nullptr, 0, in.View(), 0, 1, nullptr), 0);
  EXPECT_LT(GFGpgEncrypt(0, nullptr, 0, in.View(), 1, nullptr), 0);
}

// The contract that the old API broke by accident: a non-negative return
// ALWAYS means there is an owned result to release, failures included.
TEST(SdkGpgResultTest, ARejectedRequestStillHandsBackAnOwnedResult) {
  auto in = GFBuf::Copy(QByteArray("data"));

  GFGpgResultRef raw = nullptr;
  const auto ret = GFGpgEncrypt(0, nullptr, 0, in.View(), 1, &raw);

  ASSERT_GE(ret, 0) << "no usable recipient is a request error, not a crash";
  ASSERT_NE(raw, nullptr) << "a non-negative return must yield a result";
  EXPECT_EQ(GFGpgResultStatusOf(raw), GF_GPG_BAD_REQUEST);

  // And it says why, rather than failing mutely.
  EXPECT_STRNE(GFGpgResultErrorString(raw), "");

  GFGpgResultRelease(raw);
}

TEST(SdkGpgResultTest, TheLedgerBalancesAcrossAFailedRequest) {
  const auto before = GFGpgResultOutstandingCount(nullptr);
  {
    auto in = GFBuf::Copy(QByteArray("data"));
    GFGpgResult r;
    GFGpgEncrypt(0, nullptr, 0, in.View(), 1, r.Out());
    EXPECT_EQ(GFGpgResultOutstandingCount(nullptr), before + 1);
  }
  // The RAII wrapper reclaimed it on scope exit, error path and all.
  EXPECT_EQ(GFGpgResultOutstandingCount(nullptr), before);
}

TEST(SdkGpgResultTest, EveryAccessorToleratesANullHandle) {
  EXPECT_EQ(GFGpgResultStatusOf(nullptr), GF_GPG_BAD_REQUEST);
  EXPECT_EQ(GFGpgResultError(nullptr), 0U);
  EXPECT_EQ(GFGpgResultData(nullptr), nullptr);
  EXPECT_STREQ(GFGpgResultCapsuleId(nullptr), "");
  EXPECT_STREQ(GFGpgResultErrorString(nullptr), "");
  EXPECT_STREQ(GFGpgResultHashAlgo(nullptr), "");
  EXPECT_EQ(GFGpgResultTakeData(nullptr), nullptr);
  GFGpgResultRelease(nullptr);  // must not crash
}

TEST(SdkGpgResultTest, ADoubleReleaseIsDetectedRatherThanCorruptingTheHeap) {
  auto in = GFBuf::Copy(QByteArray("data"));
  GFGpgResultRef raw = nullptr;
  ASSERT_GE(GFGpgEncrypt(0, nullptr, 0, in.View(), 1, &raw), 0);
  ASSERT_NE(raw, nullptr);

  const auto before = GFGpgResultOutstandingCount(nullptr);
  GFGpgResultRelease(raw);
  ASSERT_EQ(GFGpgResultOutstandingCount(nullptr), before - 1);

  // Same policy as the buffer handle and as SecureMemoryAllocator: warn in
  // release, fail fast in debug. Decided from the registry, so the freed
  // block is never read to reach the verdict.
#ifdef DEBUG
  EXPECT_DEATH({ GFGpgResultRelease(raw); }, "");
#else
  GFGpgResultRelease(raw);
  EXPECT_EQ(GFGpgResultOutstandingCount(nullptr), before - 1);
#endif
}

TEST(SdkGpgResultTest, MovingTheWrapperTransfersOwnershipExactlyOnce) {
  const auto before = GFGpgResultOutstandingCount(nullptr);
  {
    auto in = GFBuf::Copy(QByteArray("data"));
    GFGpgResult a;
    GFGpgEncrypt(0, nullptr, 0, in.View(), 1, a.Out());
    ASSERT_EQ(GFGpgResultOutstandingCount(nullptr), before + 1);

    GFGpgResult b = std::move(a);
    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(GFGpgResultOutstandingCount(nullptr), before + 1);
  }
  EXPECT_EQ(GFGpgResultOutstandingCount(nullptr), before);
}

/* --- round trips that need a real engine -------------------------------- */

TEST(SdkGpgResultTest, ASignRoundTripCarriesItsPayloadAndCapsule) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";

  // Without a secret key this is a request or operation failure, not a crash;
  // either way the result must be owned, self-describing, and releasable.
  auto in = GFBuf::Copy(QByteArray("sign me"));
  GFGpgResult r;
  const auto ret = GFGpgSign(0, nullptr, 0, in.View(), 0, 1, r.Out());

  ASSERT_GE(ret, 0);
  ASSERT_TRUE(static_cast<bool>(r));
  EXPECT_NE(r.Status(), GF_GPG_OK) << "no signer was supplied";
  EXPECT_FALSE(r.ErrorString().isEmpty());
}

// Verification produces no payload -- the input IS the message -- so Data()
// must be empty rather than undefined.
TEST(SdkGpgResultTest, AVerifyResultCarriesNoPayload) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";
  auto in = GFBuf::Copy(QByteArray("not a signed message"));
  GFGpgResult r;
  const auto ret = GFGpgVerify(0, in.View(), nullptr, r.Out());

  ASSERT_GE(ret, 0);
  ASSERT_TRUE(static_cast<bool>(r));
  EXPECT_TRUE(r.DataCopy().isEmpty());
}

}  // namespace GpgFrontend::Test
