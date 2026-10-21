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

#include "ui/model/KeySearchMatcher.h"

namespace GpgFrontend::Test {

namespace {
const QString kFpr = "E3F5A2C1B4D6E8F0A1B2C3D4E5F60718293A4B5C";
}  // namespace

TEST(UiKeySearchMatcherTest, NormalizeStripsSeparatorsAndCase) {
  EXPECT_EQ(UI::KeySearchNormalize("E3F5 A2C1:b4d6-E8F0\t"),
            QString("e3f5a2c1b4d6e8f0"));
}

TEST(UiKeySearchMatcherTest, EmptyKeywordMatchesEverything) {
  EXPECT_TRUE(UI::KeySearchMatches({}, kFpr, ""));
}

TEST(UiKeySearchMatcherTest, MatchesFieldsCaseInsensitively) {
  EXPECT_TRUE(
      UI::KeySearchMatches({"Alice", "alice@example.com"}, kFpr, "ALICE@"));
  EXPECT_FALSE(UI::KeySearchMatches({"Alice"}, kFpr, "bob"));
}

TEST(UiKeySearchMatcherTest, MatchesFingerprintInAnyGrouping) {
  EXPECT_TRUE(UI::KeySearchMatches({}, kFpr, kFpr));
  EXPECT_TRUE(UI::KeySearchMatches({}, kFpr, "e3f5a2c1b4d6"));
  EXPECT_TRUE(UI::KeySearchMatches({}, kFpr, "E3F5 A2C1 B4D6 E8F0"));
  EXPECT_TRUE(UI::KeySearchMatches({}, kFpr, "e3:f5:a2:c1"));
  EXPECT_FALSE(UI::KeySearchMatches({}, kFpr, "ffff ffff"));
}

TEST(UiKeySearchMatcherTest, SeparatorOnlyKeywordDoesNotMatchFingerprint) {
  EXPECT_FALSE(UI::KeySearchMatches({"Alice"}, kFpr, " : "));
}

}  // namespace GpgFrontend::Test
