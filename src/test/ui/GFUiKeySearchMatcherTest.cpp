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
#include "ui/model/KeySearchMatcher.h"

namespace GpgFrontend::Test {

namespace {

// How gpg reports a fingerprint: 40 hex characters, unbroken.
const QString kFingerprint = "9F36C0F8B2A45D71E6C4A1B39D08E7524CAB31FD";

auto Fields() -> QStringList {
  return {"pub/sec",
          "Alice Example",
          "alice@example.com",
          "E",
          "Full",
          "4CAB31FD",
          "2024-01-01",
          "Never",
          "RSA",
          "2",
          ""};
}

}  // namespace

TEST(KeySearchMatcherTest, EmptyKeywordAcceptsEveryKey) {
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, ""));
  EXPECT_TRUE(UI::KeySearchMatches({}, "", ""));
}

TEST(KeySearchMatcherTest, MatchesFieldTextCaseInsensitively) {
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, "alice"));
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, "ALICE"));
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, "example.com"));
}

TEST(KeySearchMatcherTest, NoMatchReturnsFalse) {
  EXPECT_FALSE(UI::KeySearchMatches(Fields(), kFingerprint, "bob"));
}

// The search box has always promised fingerprint search, but no column carries
// the fingerprint, so pasting one used to return nothing at all.

TEST(KeySearchMatcherTest, MatchesAFingerprintPastedAsGpgPrintsIt) {
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, kFingerprint));
}

TEST(KeySearchMatcherTest, MatchesAFingerprintPastedWithSeparators) {
  // The three groupings a user actually ends up with on the clipboard.
  EXPECT_TRUE(
      UI::KeySearchMatches(Fields(), kFingerprint,
                           "9F36 C0F8 B2A4 5D71 E6C4  A1B3 9D08 E752 4CAB "
                           "31FD"));
  EXPECT_TRUE(UI::KeySearchMatches(
      Fields(), kFingerprint,
      "9F:36:C0:F8:B2:A4:5D:71:E6:C4:A1:B3:9D:08:E7:52:4C:AB:31:FD"));
  EXPECT_TRUE(
      UI::KeySearchMatches(Fields(), kFingerprint, "9f36-c0f8-b2a4-5d71"));
}

TEST(KeySearchMatcherTest, MatchesAFingerprintPrefixInEitherCase) {
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, "9F36C0F8"));
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, "9f36c0f8"));
}

TEST(KeySearchMatcherTest, MatchesAFingerprintTailNotShownInAnyColumn) {
  // The Key ID column carries only the last 8 hex digits, so a middle slice
  // can only be found through the fingerprint itself.
  EXPECT_TRUE(UI::KeySearchMatches(Fields(), kFingerprint, "E6C4A1B3"));
}

TEST(KeySearchMatcherTest, DoesNotMatchAForeignFingerprint) {
  EXPECT_FALSE(
      UI::KeySearchMatches(Fields(), kFingerprint, "0000111122223333"));
}

TEST(KeySearchMatcherTest, EmptyFingerprintDoesNotCrashOrMatch) {
  // Key groups have no fingerprint at all.
  EXPECT_FALSE(UI::KeySearchMatches(Fields(), "", "9F36C0F8"));
}

TEST(KeySearchMatcherTest, SeparatorOnlyKeywordDoesNotMatchEverything) {
  // Normalising " - " down to "" must not turn into an empty-needle contains()
  // that matches every key.
  EXPECT_FALSE(UI::KeySearchMatches({"Alice"}, kFingerprint, " - "));
}

TEST(KeySearchMatcherTest, MatchesAUserIdThatNoColumnDisplays) {
  // A key's secondary UIDs appear in no column; they are appended to the field
  // list by the caller precisely so they stay searchable.
  auto fields = Fields();
  fields << "Alice Example (work) <alice@corp.example>";

  EXPECT_TRUE(UI::KeySearchMatches(fields, kFingerprint, "corp.example"));
}

TEST(KeySearchNormalizeTest, StripsSeparatorsAndLowerCases) {
  EXPECT_EQ(UI::KeySearchNormalize("9F36 C0F8"), "9f36c0f8");
  EXPECT_EQ(UI::KeySearchNormalize("9F:36-C0F8"), "9f36c0f8");
}

TEST(KeySearchNormalizeTest, IsIdempotent) {
  const auto once = UI::KeySearchNormalize(kFingerprint);
  EXPECT_EQ(UI::KeySearchNormalize(once), once);
}

}  // namespace GpgFrontend::Test
