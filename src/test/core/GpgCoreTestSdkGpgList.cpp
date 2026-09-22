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
#include "core/SdkTestContext.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKBuffer.hpp"
#include "sdk/GFSDKGpgList.h"

/**
 * @file GpgCoreTestSdkGpgList.cpp
 * @brief The opaque list handles' contract.
 *
 * What these pin: one release per list, borrowed element accessors, a
 * bounds-checked index, and that the distinct types cannot be mixed up. The
 * per-struct teardown functions these replace (GFGpgFreeKeyBriefs,
 * GFGpgFreeEncRecipients, GFGpgFreeStringArray) each needed the caller to
 * remember both the right walker and the right count.
 */

namespace GpgFrontend::Test {

namespace {

/// One granted context for this file, minted the way the module loader
/// mints one. Process-lifetime on purpose: a grant is retired, never
/// freed, so that a stale caller is refused rather than following a
/// dangling pointer.
auto Ctx() -> GFSDKContext* {
  static SdkTestContext context("com.example.sdk.gpglist");
  return context.get();
}

}  // namespace

namespace {

auto ChannelIsUsable(int channel) -> bool {
  return OpenPGPContext::GetInstance(channel).Good();
}

}  // namespace

TEST(SdkGpgListTest, ANullOutParameterIsRejectedRatherThanDereferenced) {
  auto in = GFBuf::Copy(Ctx(), QByteArray("data"));
  EXPECT_LT(GFGpgFindKeys(Ctx(), 0, "someone@example.org", nullptr), 0);
  EXPECT_LT(GFGpgSniffRecipients(Ctx(), 0, in.View(), nullptr), 0);
  EXPECT_LT(GFGpgListAddresses(Ctx(), 0, 1, nullptr), 0);
}

TEST(SdkGpgListTest, ABorrowedInputIsNotTakenOwnershipOf) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";

  // The address stays the caller's: readable, unchanged, and freeable by us
  // afterwards. Under the one ownership rule no SDK argument is ever consumed.
  QByteArray address = "nobody@example.invalid";
  GFGpgKeyBriefListRef list = nullptr;

  ASSERT_EQ(GFGpgFindKeys(Ctx(), 0, address.constData(), &list), 0);
  EXPECT_EQ(address, QByteArray("nobody@example.invalid"));

  GFGpgKeyBriefRelease(Ctx(), list);
}

TEST(SdkGpgListTest, AnEmptyResultIsSuccessNotAnError) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";
  GFGpgKeyBriefListRef list = nullptr;
  // "matches nothing" is an answer, not a failure.
  ASSERT_EQ(GFGpgFindKeys(Ctx(), 0, "definitely-nobody@example.invalid", &list),
            0);
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(GFGpgKeyBriefCount(Ctx(), list), 0U);

  GFGpgKeyBriefRelease(Ctx(), list);
}

TEST(SdkGpgListTest, AnOutOfRangeIndexIsBoundedRatherThanUndefined) {
  // Engine-free: bytes that are not an OpenPGP message yield an empty list
  // without a key lookup, which is all this needs to index past the end.
  auto in = GFBuf::Copy(Ctx(), QByteArray("plainly not an openpgp message"));

  GFGpgRecipientListRef list = nullptr;
  ASSERT_EQ(GFGpgSniffRecipients(Ctx(), 0, in.View(), &list), 0);
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(GFGpgRecipientCount(Ctx(), list), 0U);

  // Past the end yields NULL, never a read of nothing. That is the row
  // accessor's contract, and it is stricter than the per-field accessors it
  // replaced: those returned "" and 0, which a caller could not tell from a
  // real empty value. A caller must check, and this is where that is said.
  EXPECT_EQ(GFGpgRecipientAt(Ctx(), list, 0), nullptr);
  EXPECT_EQ(GFGpgRecipientAt(Ctx(), list, 99), nullptr);

  GFGpgRecipientRelease(Ctx(), list);
}

TEST(SdkGpgListTest, EveryAccessorToleratesANullHandle) {
  EXPECT_EQ(GFGpgKeyBriefCount(Ctx(), nullptr), 0U);
  EXPECT_EQ(GFGpgKeyBriefAt(Ctx(), nullptr, 0), nullptr);
  GFGpgKeyBriefRelease(Ctx(), nullptr);

  EXPECT_EQ(GFGpgRecipientCount(Ctx(), nullptr), 0U);
  EXPECT_EQ(GFGpgRecipientAt(Ctx(), nullptr, 0), nullptr);
  GFGpgRecipientRelease(Ctx(), nullptr);

  EXPECT_EQ(GFStringListCount(Ctx(), nullptr), 0U);
  EXPECT_STREQ(GFStringListAt(Ctx(), nullptr, 0), "");
  GFStringListRelease(Ctx(), nullptr);
}

TEST(SdkGpgListTest, SniffingRejectsAnEmptyInputRatherThanGuessing) {
  GFBuf empty(Ctx());
  GFGpgRecipientListRef list = nullptr;
  EXPECT_LT(GFGpgSniffRecipients(Ctx(), 0, empty.View(), &list), 0);
  EXPECT_EQ(list, nullptr);
}

// The list tests above need a key repository, and a sharded run uses a
// throwaway profile with no engine, where merely fetching keys asserts. These
// two cover the same lifetime properties without touching the repository:
// bytes that are not an OpenPGP message carry no PKESK packet, so sniffing
// them returns an empty list without ever looking a key up.
TEST(SdkGpgListTest, TheLedgerBalancesWithoutNeedingAnEngine) {
  auto in = GFBuf::Copy(Ctx(), QByteArray("plainly not an openpgp message"));

  const auto before = GFListOutstandingCount(Ctx());

  GFGpgRecipientListRef list = nullptr;
  ASSERT_EQ(GFGpgSniffRecipients(Ctx(), 0, in.View(), &list), 0);
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(GFGpgRecipientCount(Ctx(), list), 0U);
  EXPECT_EQ(GFListOutstandingCount(Ctx()), before + 1);

  GFGpgRecipientRelease(Ctx(), list);
  EXPECT_EQ(GFListOutstandingCount(Ctx()), before);
}

TEST(SdkGpgListTest, ADoubleReleaseIsDetectedWithoutNeedingAnEngine) {
  auto in = GFBuf::Copy(Ctx(), QByteArray("plainly not an openpgp message"));

  GFGpgRecipientListRef list = nullptr;
  ASSERT_EQ(GFGpgSniffRecipients(Ctx(), 0, in.View(), &list), 0);
  ASSERT_NE(list, nullptr);

  const auto before = GFListOutstandingCount(Ctx());
  GFGpgRecipientRelease(Ctx(), list);
  ASSERT_EQ(GFListOutstandingCount(Ctx()), before - 1);

#ifdef DEBUG
  EXPECT_DEATH({ GFGpgRecipientRelease(Ctx(), list); }, "");
#else
  GFGpgRecipientRelease(Ctx(), list);
  EXPECT_EQ(GFListOutstandingCount(Ctx()), before - 1);
#endif
}

}  // namespace GpgFrontend::Test
