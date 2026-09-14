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

auto ChannelIsUsable(int channel) -> bool {
  return OpenPGPContext::GetInstance(channel).Good();
}

}  // namespace

TEST(SdkGpgListTest, ANullOutParameterIsRejectedRatherThanDereferenced) {
  auto in = GFBuf::Copy(QByteArray("data"));
  EXPECT_LT(GFGpgFindKeys(0, "someone@example.org", nullptr), 0);
  EXPECT_LT(GFGpgSniffRecipients(0, in.View(), nullptr), 0);
  EXPECT_LT(GFGpgListAddresses(0, 1, nullptr), 0);
}

TEST(SdkGpgListTest, ABorrowedInputIsNotTakenOwnershipOf) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";

  // The address stays the caller's: readable, unchanged, and freeable by us
  // afterwards. Under the one ownership rule no SDK argument is ever consumed.
  QByteArray address = "nobody@example.invalid";
  GFGpgKeyBriefListRef list = nullptr;

  ASSERT_EQ(GFGpgFindKeys(0, address.constData(), &list), 0);
  EXPECT_EQ(address, QByteArray("nobody@example.invalid"));

  GFGpgKeyBriefListRelease(list);
}

TEST(SdkGpgListTest, AnEmptyResultIsSuccessNotAnError) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";
  GFGpgKeyBriefListRef list = nullptr;
  // "matches nothing" is an answer, not a failure.
  ASSERT_EQ(GFGpgFindKeys(0, "definitely-nobody@example.invalid", &list), 0);
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(GFGpgKeyBriefListCount(list), 0U);

  GFGpgKeyBriefListRelease(list);
}

TEST(SdkGpgListTest, AnOutOfRangeIndexIsBoundedRatherThanUndefined) {
  // Engine-free: bytes that are not an OpenPGP message yield an empty list
  // without a key lookup, which is all this needs to index past the end.
  auto in = GFBuf::Copy(QByteArray("plainly not an openpgp message"));

  GFGpgRecipientListRef list = nullptr;
  ASSERT_EQ(GFGpgSniffRecipients(0, in.View(), &list), 0);
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(GFGpgRecipientListCount(list), 0U);

  // Past the end yields "" and 0, never a read of nothing.
  EXPECT_STREQ(GFGpgRecipientKeyId(list, 0), "");
  EXPECT_STREQ(GFGpgRecipientFingerprint(list, 99), "");
  EXPECT_EQ(GFGpgRecipientKeyFound(list, 99), 0);
  EXPECT_EQ(GFGpgRecipientHidden(list, 99), 0);

  GFGpgRecipientListRelease(list);
}

TEST(SdkGpgListTest, EveryAccessorToleratesANullHandle) {
  EXPECT_EQ(GFGpgKeyBriefListCount(nullptr), 0U);
  EXPECT_STREQ(GFGpgKeyBriefFingerprint(nullptr, 0), "");
  EXPECT_EQ(GFGpgKeyBriefUsability(nullptr, 0), 0);
  GFGpgKeyBriefListRelease(nullptr);

  EXPECT_EQ(GFGpgRecipientListCount(nullptr), 0U);
  EXPECT_STREQ(GFGpgRecipientKeyId(nullptr, 0), "");
  EXPECT_EQ(GFGpgRecipientHidden(nullptr, 0), 0);
  GFGpgRecipientListRelease(nullptr);

  EXPECT_EQ(GFStringListCount(nullptr), 0U);
  EXPECT_STREQ(GFStringListAt(nullptr, 0), "");
  GFStringListRelease(nullptr);
}

TEST(SdkGpgListTest, SniffingRejectsAnEmptyInputRatherThanGuessing) {
  GFBuf empty;
  GFGpgRecipientListRef list = nullptr;
  EXPECT_LT(GFGpgSniffRecipients(0, empty.View(), &list), 0);
  EXPECT_EQ(list, nullptr);
}

// The list tests above need a key repository, and a sharded run uses a
// throwaway profile with no engine, where merely fetching keys asserts. These
// two cover the same lifetime properties without touching the repository:
// bytes that are not an OpenPGP message carry no PKESK packet, so sniffing
// them returns an empty list without ever looking a key up.
TEST(SdkGpgListTest, TheLedgerBalancesWithoutNeedingAnEngine) {
  auto in = GFBuf::Copy(QByteArray("plainly not an openpgp message"));

  const auto before = GFGpgListOutstandingCount(nullptr);

  GFGpgRecipientListRef list = nullptr;
  ASSERT_EQ(GFGpgSniffRecipients(0, in.View(), &list), 0);
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(GFGpgRecipientListCount(list), 0U);
  EXPECT_EQ(GFGpgListOutstandingCount(nullptr), before + 1);

  GFGpgRecipientListRelease(list);
  EXPECT_EQ(GFGpgListOutstandingCount(nullptr), before);
}

TEST(SdkGpgListTest, ADoubleReleaseIsDetectedWithoutNeedingAnEngine) {
  auto in = GFBuf::Copy(QByteArray("plainly not an openpgp message"));

  GFGpgRecipientListRef list = nullptr;
  ASSERT_EQ(GFGpgSniffRecipients(0, in.View(), &list), 0);
  ASSERT_NE(list, nullptr);

  const auto before = GFGpgListOutstandingCount(nullptr);
  GFGpgRecipientListRelease(list);
  ASSERT_EQ(GFGpgListOutstandingCount(nullptr), before - 1);

#ifdef DEBUG
  EXPECT_DEATH({ GFGpgRecipientListRelease(list); }, "");
#else
  GFGpgRecipientListRelease(list);
  EXPECT_EQ(GFGpgListOutstandingCount(nullptr), before - 1);
#endif
}

}  // namespace GpgFrontend::Test
