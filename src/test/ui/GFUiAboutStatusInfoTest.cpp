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
#include "ui/dialog/help/AboutStatusInfo.h"

namespace GpgFrontend::Test {

namespace {

using UI::AboutStatusValue;
using UI::DescribeKeySource;
using UI::DescribeSessionStorage;

/// The character the whole restructure exists to remove from these readings.
constexpr auto kEmDash = QChar(0x2014);

auto AllReadings() -> QList<AboutStatusValue> {
  return {
      DescribeSessionStorage(true, false),
      DescribeSessionStorage(false, true),
      DescribeSessionStorage(false, false),
      DescribeKeySource(true, ProfileKind::kPERSIST),
      DescribeKeySource(false, ProfileKind::kPERSIST),
      DescribeKeySource(false, ProfileKind::kPACKAGED),
  };
}

}  // namespace

TEST(AboutStatusInfoTest, EachStorageOutcomeReadsDifferently) {
  const auto memory = DescribeSessionStorage(true, false);
  const auto encrypted = DescribeSessionStorage(false, true);
  const auto plain = DescribeSessionStorage(false, false);

  EXPECT_NE(memory.value, encrypted.value);
  EXPECT_NE(encrypted.value, plain.value);
  EXPECT_NE(memory.value, plain.value);

  // Every storage outcome has something to add, so none of them is a bare
  // value: that is what the second line is for.
  EXPECT_FALSE(memory.detail.isEmpty());
  EXPECT_FALSE(encrypted.detail.isEmpty());
  EXPECT_FALSE(plain.detail.isEmpty());
}

TEST(AboutStatusInfoTest, VolatileWinsOverEncryptedAtRest) {
  // The two axes are independent, and an accessor can report both. Memory is
  // the stronger statement, so it is the one shown.
  const auto both = DescribeSessionStorage(true, true);

  EXPECT_EQ(both.value, DescribeSessionStorage(true, false).value);
  EXPECT_FALSE(both.degraded);
}

TEST(AboutStatusInfoTest, OnlyTheOrdinaryFolderCountsAsAFallback) {
  // A silent downgrade is the one thing this mechanism must not be, so the
  // outcome nobody asked for is the only one marked.
  EXPECT_FALSE(DescribeSessionStorage(true, false).degraded);
  EXPECT_FALSE(DescribeSessionStorage(false, true).degraded);
  EXPECT_TRUE(DescribeSessionStorage(false, false).degraded);
}

TEST(AboutStatusInfoTest, ASelfContainedProfileNeedsNoExplanation) {
  const auto contained = DescribeKeySource(true, ProfileKind::kPERSIST);

  EXPECT_FALSE(contained.value.isEmpty());
  EXPECT_TRUE(contained.detail.isEmpty());
  EXPECT_FALSE(contained.degraded);

  // The profile kind cannot change the answer once the keys are inside it.
  EXPECT_EQ(contained.value,
            DescribeKeySource(true, ProfileKind::kPACKAGED).value);
  EXPECT_TRUE(DescribeKeySource(true, ProfileKind::kPACKAGED).detail.isEmpty());
}

TEST(AboutStatusInfoTest, OnlyAPackageSaysItCarriesNoKeysOfItsOwn) {
  const auto packaged = DescribeKeySource(false, ProfileKind::kPACKAGED);
  const auto ordinary = DescribeKeySource(false, ProfileKind::kPERSIST);

  // Both are looking at the same keyring, and say so identically.
  EXPECT_EQ(packaged.value, ordinary.value);

  // Only the package has the thing worth adding: the window is showing this
  // computer's keys, not the ones the sender meant to hand over.
  EXPECT_FALSE(packaged.detail.isEmpty());
  EXPECT_TRUE(ordinary.detail.isEmpty());
}

TEST(AboutStatusInfoTest, NoReadingCarriesAnEmDash) {
  // The regression this whole structure replaces: a value, an em dash, and an
  // explanatory clause written as one string, which turned the value column
  // into a paragraph.
  for (const auto& reading : AllReadings()) {
    EXPECT_FALSE(reading.value.contains(kEmDash))
        << reading.value.toStdString();
    EXPECT_FALSE(reading.detail.contains(kEmDash))
        << reading.detail.toStdString();
  }
}

TEST(AboutStatusInfoTest, EveryValueStillReadsAsAValue) {
  for (const auto& reading : AllReadings()) {
    EXPECT_FALSE(reading.value.isEmpty());

    // A value is a phrase, not a sentence: no terminating period, and short
    // enough to scan in a form column.
    EXPECT_FALSE(reading.value.trimmed().endsWith(QLatin1Char('.')))
        << reading.value.toStdString();
    EXPECT_EQ(reading.value.trimmed(), reading.value);

    // The sentence, where there is one, is where the length went.
    if (!reading.detail.isEmpty()) {
      EXPECT_LT(reading.value.length(), reading.detail.length())
          << reading.value.toStdString();
      EXPECT_TRUE(reading.detail.trimmed().endsWith(QLatin1Char('.')))
          << reading.detail.toStdString();
    }
  }
}

}  // namespace GpgFrontend::Test
