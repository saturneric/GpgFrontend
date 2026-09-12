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
#include "sdk/GFSDKGpg.h"

namespace GpgFrontend::Test {

namespace {

/// A lookup that actually reaches the key repository needs a working engine on
/// the channel. Shards run against a throwaway profile with no agent, where
/// there is none, so the tests that get that far say so instead of asserting
/// deep inside the dispatcher.
auto ChannelIsUsable(int channel) -> bool {
  return OpenPGPContext::GetInstance(channel).Good();
}

}  // namespace

// The SDK has two kinds of string parameter and they look almost identical at
// a call site. A `char*` is handed over: the callee consumes it via GFUnStrDup,
// which frees it. A `const char*` is borrowed and stays the caller's to free.
//
// GFGpgFindKeysByEmail borrows, and once did not: it ran its argument through
// GFUnStrDup, which released memory the caller still owned and had not
// allocated through the secure allocator. The result was heap corruption inside
// free() the moment the Security tab looked up a key. These tests pin the
// borrow down so the distinction cannot quietly flip again.

TEST(SdkKeyBriefTest, ARejectedAddressIsStillNotTakenOwnershipOf) {
  // Deliberately a buffer this test owns, from the ordinary heap rather than
  // the SDK's secure allocator -- exactly what a module hands in.
  //
  // The address is whitespace, so the call is rejected before it ever reaches
  // the key repository: this pins the borrow without needing a working engine,
  // which the sharded runs do not have. It still catches the original bug,
  // because that one released the argument on the very first line, ahead of
  // every validity check.
  QByteArray address("   ");
  const QByteArray original = address;

  GFGpgKeyBrief* briefs = nullptr;
  int count = 0;
  EXPECT_EQ(GFGpgFindKeysByEmail(0, address.constData(), &briefs, &count), -1);
  EXPECT_EQ(address, original);

  // Still the caller's to hand over again.
  EXPECT_EQ(GFGpgFindKeysByEmail(0, address.constData(), &briefs, &count), -1);
  EXPECT_EQ(address, original);
}

TEST(SdkKeyBriefTest, LookupDoesNotTakeOwnershipOfTheAddress) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";

  // Deliberately a buffer this test owns, allocated by the ordinary heap rather
  // than the SDK's allocator -- exactly what a module passes in.
  QByteArray address = QByteArray("nobody-") + QByteArray(32, 'a') +
                       QByteArray("@example.invalid");
  const QByteArray original = address;

  GFGpgKeyBrief* briefs = nullptr;
  int count = -1;
  ASSERT_EQ(GFGpgFindKeysByEmail(0, address.constData(), &briefs, &count), 0);

  // The caller's buffer must come back untouched and still be its own to use.
  EXPECT_EQ(address, original);

  // And it must survive a second lookup: under the ownership bug the first call
  // had already released it.
  GFGpgKeyBrief* again = nullptr;
  int again_count = -1;
  ASSERT_EQ(GFGpgFindKeysByEmail(0, address.constData(), &again, &again_count),
            0);
  EXPECT_EQ(again_count, count);
  EXPECT_EQ(address, original);

  GFGpgFreeKeyBriefs(briefs, count);
  GFGpgFreeKeyBriefs(again, again_count);
}

TEST(SdkKeyBriefTest, AnAddressThatMatchesNothingIsNotAnError) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";

  GFGpgKeyBrief* briefs = nullptr;
  int count = -1;
  EXPECT_EQ(GFGpgFindKeysByEmail(0, "nobody@example.invalid", &briefs, &count),
            0);
  EXPECT_EQ(count, 0);
  EXPECT_EQ(briefs, nullptr);

  // Freeing the empty result is still legal, and so is freeing nothing at all.
  GFGpgFreeKeyBriefs(briefs, count);
  GFGpgFreeKeyBriefs(nullptr, 0);
}

TEST(SdkKeyBriefTest, MissingArgumentsAreRejectedRatherThanDereferenced) {
  GFGpgKeyBrief* briefs = nullptr;
  int count = 0;

  EXPECT_EQ(GFGpgFindKeysByEmail(0, "someone@example.invalid", nullptr, &count),
            -1);
  EXPECT_EQ(
      GFGpgFindKeysByEmail(0, "someone@example.invalid", &briefs, nullptr), -1);
  EXPECT_EQ(GFGpgFindKeysByEmail(0, nullptr, &briefs, &count), -1);
  EXPECT_EQ(GFGpgFindKeysByEmail(0, "   ", &briefs, &count), -1);

  // A rejected call must leave the out-parameters in the safe state it
  // promises, so the caller's cleanup path is always valid.
  EXPECT_EQ(briefs, nullptr);
  EXPECT_EQ(count, 0);
}

}  // namespace GpgFrontend::Test
