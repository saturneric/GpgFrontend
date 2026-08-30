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
using UI::BuildProfileIdentityRows;
using UI::DescribeAppKeyProtection;
using UI::DescribeSessionStorage;
using UI::ShowsDetailInline;

/// The character the whole restructure exists to remove from these readings.
constexpr auto kEmDash = QChar(0x2014);

auto AllReadings() -> QList<AboutStatusValue> {
  return {
      DescribeSessionStorage(true, false),
      DescribeSessionStorage(false, true),
      DescribeSessionStorage(false, false),
      DescribeAppKeyProtection(AppKeyProtection::kNONE, true),
      DescribeAppKeyProtection(AppKeyProtection::kPIN, false),
      DescribeAppKeyProtection(AppKeyProtection::kKEYCHAIN, true),
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

TEST(AboutStatusInfoTest, ARootProfileDoesNotPrintItsNameTwice) {
  // The defect: CurrentProfileDisplayName() answers with the kind for a profile
  // that has no name of its own, and the type row under it then said the same
  // word again.
  for (const auto kind :
       {ProfileKind::kINSTALLED_ROOT, ProfileKind::kPORTABLE_ROOT}) {
    const auto rows = BuildProfileIdentityRows(kind, "Default", false);

    ASSERT_EQ(rows.size(), 1) << ProfileKindToString(kind).toStdString();
    EXPECT_EQ(rows.first().value, "Default");
    // Nothing is lost by the merge: what the second row would have said is now
    // the sentence under the first.
    EXPECT_FALSE(rows.first().detail.isEmpty());
  }
}

TEST(AboutStatusInfoTest, ANamedProfileKeepsItsTypeRow) {
  // Where the two rows say different things, they both earn their place.
  for (const auto kind : {ProfileKind::kPERSIST, ProfileKind::kPACKAGED}) {
    const auto rows = BuildProfileIdentityRows(kind, "Work", false);

    ASSERT_EQ(rows.size(), 2) << ProfileKindToString(kind).toStdString();
    EXPECT_EQ(rows.first().value, "Work");
    EXPECT_NE(rows.at(1).value, rows.first().value);
    EXPECT_FALSE(rows.at(1).value.isEmpty());
  }
}

TEST(AboutStatusInfoTest, OnlyATransientSessionIsCalledTemporary) {
  // Asked of the profile rather than of the kind, so a shape added later cannot
  // inherit the sentence just by being packaged.
  const auto transient =
      BuildProfileIdentityRows(ProfileKind::kPACKAGED, "Work", true);
  const auto kept =
      BuildProfileIdentityRows(ProfileKind::kPACKAGED, "Work", false);

  ASSERT_EQ(transient.size(), 2);
  ASSERT_EQ(kept.size(), 2);
  EXPECT_FALSE(transient.at(1).detail.isEmpty());
  EXPECT_TRUE(kept.at(1).detail.isEmpty());
}

TEST(AboutStatusInfoTest, AForcedProtectionSaysWhyItWasForced) {
  // The settings page greys the keychain out for anything that travels and does
  // not explain itself; this row is where that explanation lives.
  const auto forced = DescribeAppKeyProtection(AppKeyProtection::kNONE, false);
  const auto chosen = DescribeAppKeyProtection(AppKeyProtection::kNONE, true);

  EXPECT_EQ(forced.value, chosen.value);
  EXPECT_FALSE(forced.detail.isEmpty());
  EXPECT_TRUE(chosen.detail.isEmpty());

  // A rule, not a fallback: nothing here went wrong.
  EXPECT_FALSE(forced.degraded);
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

TEST(AboutStatusInfoTest, ADetailWithoutADegradedStateIsAHover) {
  // Elaboration does not have to cost the page a line. Three sentences written
  // under their values is what pushed the Profile card off the bottom of the
  // dialog; behind a tooltip they are still there for anyone who looks.
  const auto detail = DescribeSessionStorage(true, false).detail;

  ASSERT_FALSE(detail.isEmpty());
  EXPECT_FALSE(ShowsDetailInline(detail, false));
}

TEST(AboutStatusInfoTest, ADegradedStateKeepsItsReasonOnScreen) {
  // The point of marking a state degraded is that nobody asked for it and
  // nobody would otherwise notice, so its reason is the one sentence that
  // cannot be left to a hover.
  const auto detail = DescribeSessionStorage(false, false).detail;

  ASSERT_FALSE(detail.isEmpty());
  EXPECT_TRUE(ShowsDetailInline(detail, true));
}

TEST(AboutStatusInfoTest, NothingIsShownInlineWithoutADetail) {
  EXPECT_FALSE(ShowsDetailInline({}, false));
  EXPECT_FALSE(ShowsDetailInline({}, true));
}

TEST(AboutStatusInfoTest, OnlyTheFallbackStorageOutcomeStaysOnScreen) {
  // The regression guard: this fails the moment a reading is marked degraded
  // without anyone deciding whether its sentence should be visible.
  auto inline_count = 0;
  for (const auto& reading : AllReadings()) {
    if (ShowsDetailInline(reading.detail, reading.degraded)) inline_count++;
  }

  EXPECT_EQ(inline_count, 1);

  const auto plain = DescribeSessionStorage(false, false);
  EXPECT_TRUE(ShowsDetailInline(plain.detail, plain.degraded));
}

}  // namespace GpgFrontend::Test
