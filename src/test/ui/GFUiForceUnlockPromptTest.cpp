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
#include "ui/function/GuiProfileLoaderDelegate.h"

namespace GpgFrontend::Test {

namespace {

using UI::BuildProfileLockConflictTexts;

auto Held(qint64 pid, const QString& host) -> ProfileLockResult {
  ProfileLockResult result;
  result.status = ProfileLockStatus::kHELD_ELSEWHERE;
  result.pid = pid;
  result.host = host;
  result.path = "/home/someone/.local/share/GpgFrontend/profiles/work";
  return result;
}

}  // namespace

TEST(ForceUnlockPromptTest, AKnownHolderIsNamed) {
  const auto texts = BuildProfileLockConflictTexts(Held(4242, "workstation"));

  EXPECT_EQ(texts.title, "Profile Is Already Open");
  EXPECT_TRUE(texts.text.contains("process 4242"));
  EXPECT_TRUE(texts.text.contains("workstation"));
}

TEST(ForceUnlockPromptTest, AHolderWithoutAHostIsStillPlacedSomewhere) {
  // QLockFile reports the pid without a hostname often enough that "on " with
  // nothing after it would be a real thing to read.
  const auto texts = BuildProfileLockConflictTexts(Held(4242, {}));

  EXPECT_TRUE(texts.text.contains("process 4242"));
  EXPECT_TRUE(texts.text.contains("this computer"));
}

TEST(ForceUnlockPromptTest, AnAnonymousHolderIsStillReported) {
  const auto texts = BuildProfileLockConflictTexts(Held(0, {}));

  EXPECT_TRUE(texts.text.contains("Another process has it open."));
  EXPECT_FALSE(texts.text.contains("process 0"));
}

TEST(ForceUnlockPromptTest, TheProfileIsNamed) {
  // Which profile is refused, not just that one was: the choice is made against
  // a path, and more than one profile can be open at a time.
  const auto texts = BuildProfileLockConflictTexts(Held(4242, "workstation"));

  EXPECT_TRUE(texts.text.contains(
      "/home/someone/.local/share/GpgFrontend/profiles/work"));
}

TEST(ForceUnlockPromptTest, TheCorruptionWarningSurvivedTheSecondDialog) {
  // This prompt used to be two: a refusal, and then a separate confirmation
  // behind Force Unlock carrying the warning below. The second box was removed
  // because every answer to the first one led to another box to dismiss. This
  // asserts the warning was folded in rather than dropped with it.
  const auto texts = BuildProfileLockConflictTexts(Held(4242, "workstation"));

  EXPECT_TRUE(texts.informative.contains(
      "Only do this if you are certain no other GpgFrontend window has this "
      "profile open."));
  EXPECT_TRUE(texts.informative.contains(
      "both copies will corrupt the profile's stored data"));
}

}  // namespace GpgFrontend::Test
