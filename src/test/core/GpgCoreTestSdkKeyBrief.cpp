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
#include "sdk/GFSDKGpg.h"
#include "sdk/GFSDKGpgList.h"

namespace GpgFrontend::Test {

namespace {

/// One granted context for this file, minted the way the module loader
/// mints one. Process-lifetime on purpose: a grant is retired, never
/// freed, so that a stale caller is refused rather than following a
/// dangling pointer.
auto Ctx() -> GFSDKContext* {
  static SdkTestContext context("com.example.sdk.keybrief");
  return context.get();
}

}  // namespace

namespace {

/// A lookup that actually reaches the key repository needs a working engine on
/// the channel. Shards run against a throwaway profile with no agent, where
/// there is none, so the tests that get that far say so instead of asserting
/// deep inside the dispatcher.
auto ChannelIsUsable(int channel) -> bool {
  return OpenPGPContext::GetInstance(channel).Good();
}

}  // namespace

// No SDK argument is ever consumed: every string a module passes in is
// borrowed and stays the caller's to free (see GFModuleMemory.h).
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

  GFGpgKeyBriefListRef briefs = nullptr;
  EXPECT_EQ(GFGpgFindKeys(Ctx(), 0, address.constData(), &briefs), -1);
  EXPECT_EQ(address, original);

  // Still the caller's to hand over again.
  EXPECT_EQ(GFGpgFindKeys(Ctx(), 0, address.constData(), &briefs), -1);
  EXPECT_EQ(address, original);
}

TEST(SdkKeyBriefTest, LookupDoesNotTakeOwnershipOfTheAddress) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";

  // Deliberately a buffer this test owns, allocated by the ordinary heap rather
  // than the SDK's allocator -- exactly what a module passes in.
  QByteArray address = QByteArray("nobody-") + QByteArray(32, 'a') +
                       QByteArray("@example.invalid");
  const QByteArray original = address;

  GFGpgKeyBriefListRef briefs = nullptr;
  ASSERT_EQ(GFGpgFindKeys(Ctx(), 0, address.constData(), &briefs), 0);
  const auto count = GFGpgKeyBriefCount(Ctx(), briefs);

  // The caller's buffer must come back untouched and still be its own to use.
  EXPECT_EQ(address, original);

  // And it must survive a second lookup: under the ownership bug the first call
  // had already released it.
  GFGpgKeyBriefListRef again = nullptr;
  ASSERT_EQ(GFGpgFindKeys(Ctx(), 0, address.constData(), &again), 0);
  EXPECT_EQ(GFGpgKeyBriefCount(Ctx(), again), count);
  EXPECT_EQ(address, original);

  // One release per list, no count to pass back.
  GFGpgKeyBriefRelease(Ctx(), briefs);
  GFGpgKeyBriefRelease(Ctx(), again);
}

TEST(SdkKeyBriefTest, AnAddressThatMatchesNothingIsNotAnError) {
  if (!ChannelIsUsable(0)) GTEST_SKIP() << "no usable engine on channel 0";

  GFGpgKeyBriefListRef briefs = nullptr;
  EXPECT_EQ(GFGpgFindKeys(Ctx(), 0, "nobody@example.invalid", &briefs), 0);
  ASSERT_NE(briefs, nullptr) << "an empty result is still an owned list";
  EXPECT_EQ(GFGpgKeyBriefCount(Ctx(), briefs), 0U);

  // Releasing the empty list is legal, and so is releasing nothing at all.
  GFGpgKeyBriefRelease(Ctx(), briefs);
  GFGpgKeyBriefRelease(Ctx(), nullptr);
}

TEST(SdkKeyBriefTest, MissingArgumentsAreRejectedRatherThanDereferenced) {
  GFGpgKeyBriefListRef briefs = nullptr;

  EXPECT_EQ(GFGpgFindKeys(Ctx(), 0, "someone@example.invalid", nullptr), -1);
  EXPECT_EQ(GFGpgFindKeys(Ctx(), 0, nullptr, &briefs), -1);
  EXPECT_EQ(GFGpgFindKeys(Ctx(), 0, "   ", &briefs), -1);

  // A rejected call must leave the out-parameter in the safe state it
  // promises, so the caller's cleanup path is always valid.
  EXPECT_EQ(briefs, nullptr);
}

}  // namespace GpgFrontend::Test
