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
using UI::MetaListRow;

auto Held(qint64 pid, const QString& host) -> ProfileLockResult {
  ProfileLockResult result;
  result.status = ProfileLockStatus::kHELD_ELSEWHERE;
  result.pid = pid;
  result.host = host;
  result.path = "/home/someone/.local/share/GpgFrontend/profiles/work";
  return result;
}

// Rows are looked up by caption rather than by index, same as
// GFUiProfilePackageMetaTest.cpp's CaptionOf(): the assertions should survive
// a row being reordered or another one being inserted between them.
auto RowValue(const QContainer<MetaListRow>& rows, const QString& caption)
    -> std::optional<QString> {
  for (const auto& row : rows) {
    if (row.caption == caption) return row.value;
  }
  return std::nullopt;
}

auto Row(const QContainer<MetaListRow>& rows, const QString& caption)
    -> std::optional<MetaListRow> {
  for (const auto& row : rows) {
    if (row.caption == caption) return row;
  }
  return std::nullopt;
}

}  // namespace

TEST(ForceUnlockPromptTest, AKnownHolderIsNamed) {
  const auto texts = BuildProfileLockConflictTexts(Held(4242, "workstation"));

  EXPECT_EQ(texts.title, "Profile Is Already Open");
  EXPECT_TRUE(texts.subtitle.contains("corrupt"));

  const auto held_by = RowValue(texts.rows, "Held by");
  ASSERT_TRUE(held_by.has_value());
  EXPECT_TRUE(held_by->contains("4242"));
  EXPECT_TRUE(held_by->contains("workstation"));
}

TEST(ForceUnlockPromptTest, AHolderWithoutAHostIsStillPlacedSomewhere) {
  // QLockFile reports the pid without a hostname often enough that "on " with
  // nothing after it would be a real thing to read.
  const auto texts = BuildProfileLockConflictTexts(Held(4242, {}));

  const auto held_by = RowValue(texts.rows, "Held by");
  ASSERT_TRUE(held_by.has_value());
  EXPECT_TRUE(held_by->contains("4242"));
  EXPECT_TRUE(held_by->contains("this computer"));
}

TEST(ForceUnlockPromptTest, AnAnonymousHolderIsStillReported) {
  const auto texts = BuildProfileLockConflictTexts(Held(0, {}));

  const auto held_by = RowValue(texts.rows, "Held by");
  ASSERT_TRUE(held_by.has_value());
  EXPECT_TRUE(held_by->contains("Another process"));
  EXPECT_FALSE(held_by->contains("process 0"));
}

TEST(ForceUnlockPromptTest, TheProfileIsNamed) {
  // Which profile is refused, not just that one was: the choice is made against
  // a path, and more than one profile can be open at a time.
  const auto texts = BuildProfileLockConflictTexts(Held(4242, "workstation"));

  const auto profile = Row(texts.rows, "Profile");
  ASSERT_TRUE(profile.has_value());
  EXPECT_EQ(profile->value,
            "/home/someone/.local/share/GpgFrontend/profiles/work");
  // Elided in the middle and shown in full on hover, like every other path in
  // the application -- a profile's path is exactly the unbreakable token that
  // styling exists for.
  EXPECT_TRUE(profile->path);
}

TEST(ForceUnlockPromptTest, TheCorruptionWarningSurvivedTheSecondDialog) {
  // This prompt used to be two: a refusal, and then a separate confirmation
  // behind Force Unlock carrying the warning below. The second box was removed
  // because every answer to the first one led to another box to dismiss. This
  // asserts the warning was folded in rather than dropped with it.
  const auto texts = BuildProfileLockConflictTexts(Held(4242, "workstation"));

  EXPECT_TRUE(texts.note.contains(
      "Only do this if you are certain no other GpgFrontend window has this "
      "profile open."));
  EXPECT_TRUE(texts.note.contains(
      "both copies will corrupt the profile's stored data"));
}

}  // namespace GpgFrontend::Test
