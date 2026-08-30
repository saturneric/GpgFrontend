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
#include "ui/dialog/profile/ProfileExportSummary.h"

namespace GpgFrontend::Test {

namespace {

using UI::BuildExportProtectionRows;
using UI::BuildProfileExportContents;
using UI::DescribeProfileExport;
using UI::DescribeProfileExportWarning;
using UI::EvaluateProfileExport;
using UI::ProfileExportArea;
using UI::ProfileExportChoice;
using UI::ProfileExportWarning;
using UI::ToMetaListRows;
using UI::TotalProfileExportBytes;

/// Exactly the five keys MeasureProfileAreas() returns, with distinct sizes so
/// a row taking another row's number is visible.
auto MeasuredAreas() -> QMap<QString, qint64> {
  return {
      {"config", 1000},        {"data_objs", 2000},  {"secure", 4000},
      {"key_databases", 8000}, {"workspace", 16000},
  };
}

auto Ready() -> ProfileExportChoice {
  ProfileExportChoice choice;
  choice.has_destination = true;
  choice.passphrase_acceptable = true;
  return choice;
}

auto Contains(const QVector<ProfileExportWarning>& warnings,
              ProfileExportWarning warning) -> bool {
  return warnings.contains(warning);
}

}  // namespace

TEST(ProfileExportSummaryTest, EveryMeasuredAreaGetsARow) {
  // The guard for the defect that prompted this: MeasureProfileAreas() measured
  // five areas and the dialog listed three. The one it never named was the
  // profile's own key — precisely what makes an unprotected file dangerous.
  const auto rows = BuildProfileExportContents(MeasuredAreas(), false);
  ASSERT_EQ(rows.size(), 5);

  EXPECT_EQ(rows[0].area, ProfileExportArea::kSettings);
  EXPECT_EQ(rows[1].area, ProfileExportArea::kSavedState);
  EXPECT_EQ(rows[2].area, ProfileExportArea::kProfileKey);
  EXPECT_EQ(rows[3].area, ProfileExportArea::kKeyDatabases);
  EXPECT_EQ(rows[4].area, ProfileExportArea::kWorkspace);

  for (const auto& row : rows) {
    EXPECT_FALSE(row.label.isEmpty());
    EXPECT_FALSE(row.icon.isEmpty());
  }
}

TEST(ProfileExportSummaryTest, EachRowCarriesItsOwnMeasurement) {
  const auto rows = BuildProfileExportContents(MeasuredAreas(), true);
  EXPECT_EQ(rows[0].bytes, 1000);
  EXPECT_EQ(rows[1].bytes, 2000);
  EXPECT_EQ(rows[2].bytes, 4000);
  EXPECT_EQ(rows[3].bytes, 8000);
  EXPECT_EQ(rows[4].bytes, 16000);
}

TEST(ProfileExportSummaryTest, AnEmptyAreaStillGetsARow) {
  // "Empty" and "left out" are different facts. Someone checking this list
  // against their own profile folder needs to see both.
  const auto rows = BuildProfileExportContents({{"config", 0},
                                                {"data_objs", 0},
                                                {"secure", 0},
                                                {"key_databases", 0},
                                                {"workspace", 0}},
                                               false);
  EXPECT_EQ(rows.size(), 5);
  EXPECT_EQ(TotalProfileExportBytes(rows), 0);
}

TEST(ProfileExportSummaryTest, MissingKeysMeasureZeroRatherThanDropTheRow) {
  // Renaming an area key in the core should show as a zero, not as a line that
  // silently stops being listed.
  const auto rows = BuildProfileExportContents({}, true);
  EXPECT_EQ(rows.size(), 5);
  EXPECT_EQ(TotalProfileExportBytes(rows), 0);
}

TEST(ProfileExportSummaryTest, TheWorkspaceIsTheOnlyOptionalArea) {
  for (const auto include : {false, true}) {
    const auto rows = BuildProfileExportContents(MeasuredAreas(), include);
    for (const auto& row : rows) {
      const auto is_workspace = row.area == ProfileExportArea::kWorkspace;
      EXPECT_EQ(row.optional, is_workspace);
      EXPECT_EQ(row.included, is_workspace ? include : true);
    }
  }
}

TEST(ProfileExportSummaryTest, TheTotalCountsOnlyWhatTravels) {
  const auto without = TotalProfileExportBytes(
      BuildProfileExportContents(MeasuredAreas(), false));
  const auto with = TotalProfileExportBytes(
      BuildProfileExportContents(MeasuredAreas(), true));

  EXPECT_EQ(without, 1000 + 2000 + 4000 + 8000);
  EXPECT_EQ(with, without + 16000);
}

TEST(ProfileExportSummaryTest, ADestinationIsAlwaysRequired) {
  auto choice = Ready();
  choice.has_destination = false;

  for (const auto acceptable : {false, true}) {
    choice.passphrase_acceptable = acceptable;
    EXPECT_FALSE(EvaluateProfileExport(choice).can_export);
  }
}

TEST(ProfileExportSummaryTest, ProtectedExportWaitsForAnAcceptablePassphrase) {
  auto choice = Ready();
  choice.passphrase_acceptable = false;
  EXPECT_FALSE(EvaluateProfileExport(choice).can_export);

  choice.passphrase_acceptable = true;
  EXPECT_TRUE(EvaluateProfileExport(choice).can_export);
}

TEST(ProfileExportSummaryTest, NoChoiceProducesAnUnprotectedExport) {
  // A profile file carries the profile's own key, so an unsealed one hands over
  // everything the profile ever encrypted. The dialog no longer offers that,
  // and this is what keeps the option from coming back through the rules: there
  // is no combination of choices that exports without an acceptable passphrase.
  ProfileExportChoice choice;
  for (const auto destination : {false, true}) {
    for (const auto workspace : {false, true}) {
      choice.has_destination = destination;
      choice.include_workspace = workspace;
      choice.passphrase_acceptable = false;
      EXPECT_FALSE(EvaluateProfileExport(choice).can_export);
    }
  }
}

TEST(ProfileExportSummaryTest, SpaceIsOnlyJudgedWhenItIsKnown) {
  auto choice = Ready();
  choice.free_bytes = -1;
  choice.total_bytes = std::numeric_limits<qint64>::max();

  EXPECT_FALSE(Contains(EvaluateProfileExport(choice).warnings,
                        ProfileExportWarning::kMayNotFit));
}

TEST(ProfileExportSummaryTest, MayNotFitFiresOnlyAboveTheFreeSpace) {
  auto choice = Ready();
  choice.free_bytes = 1000;

  for (const auto [total, expected] : std::array<std::pair<qint64, bool>, 3>{
           {{999, false}, {1000, false}, {1001, true}}}) {
    choice.total_bytes = total;
    EXPECT_EQ(Contains(EvaluateProfileExport(choice).warnings,
                       ProfileExportWarning::kMayNotFit),
              expected)
        << "total " << total;
  }
}

TEST(ProfileExportSummaryTest, RunningOutOfRoomDoesNotBlockTheExport) {
  // The estimate is uncompressed and the payload is gzip'd, so "too tight"
  // often still fits. The packing has its own refusal; this is only a warning
  // ahead of it.
  auto choice = Ready();
  choice.free_bytes = 10;
  choice.total_bytes = 1000;

  const auto readiness = EvaluateProfileExport(choice);
  EXPECT_TRUE(readiness.can_export);
  EXPECT_TRUE(Contains(readiness.warnings, ProfileExportWarning::kMayNotFit));
}

TEST(ProfileExportSummaryTest, EveryWarningHasAMessage) {
  for (const auto warning : {ProfileExportWarning::kMayNotFit}) {
    EXPECT_FALSE(DescribeProfileExportWarning(warning).isEmpty());
  }
}

TEST(ProfileExportSummaryTest, TheSummaryIsEmptyUntilThereIsADestination) {
  auto choice = Ready();
  choice.has_destination = false;
  EXPECT_TRUE(DescribeProfileExport(choice, "work.gfp").isEmpty());
  EXPECT_TRUE(DescribeProfileExport(Ready(), {}).isEmpty());
}

TEST(ProfileExportSummaryTest, TheSummaryNamesTheDestination) {
  EXPECT_TRUE(DescribeProfileExport(Ready(), "work.gfp").contains("work.gfp"));
}

TEST(ProfileExportSummaryTest, TheSummaryIsOneLine) {
  // It sits immediately above the button, where a paragraph is not read at all.
  // Everything it used to spell out -- what is inside, what it comes to -- is a
  // row in the list above it now, and rows can be checked; what is left here is
  // the act itself.
  const auto summary = DescribeProfileExport(Ready(), "work.gfp");

  EXPECT_FALSE(summary.contains('\n'));
  EXPECT_LT(summary.size(), 110) << summary.toStdString();
}

TEST(ProfileExportSummaryTest,
     TheSummaryMentionsTheWorkspaceOnlyWhenItTravels) {
  // The assertion that makes this sentence worth building rather than
  // decoration: it has to actually change when the checkbox does.
  auto with_workspace = Ready();
  with_workspace.include_workspace = true;

  const auto without = DescribeProfileExport(Ready(), "work.gfp");
  const auto with = DescribeProfileExport(with_workspace, "work.gfp");

  EXPECT_FALSE(without.contains("workspace", Qt::CaseInsensitive));
  EXPECT_TRUE(with.contains("workspace", Qt::CaseInsensitive));
}

TEST(ProfileExportSummaryTest, TheContentsRowsEndWithTheTotalAndTheExclusions) {
  // The dialog holds no wording of its own, so what the list adds up to and
  // what never travels are decided here, where they can be asserted.
  const auto rows =
      ToMetaListRows(BuildProfileExportContents(MeasuredAreas(), true));

  ASSERT_GE(rows.size(), 8);
  EXPECT_EQ(rows.at(5).kind, UI::MetaRowKind::kRule);
  EXPECT_EQ(rows.at(6).bytes, 1000 + 2000 + 4000 + 8000 + 16000);
  EXPECT_TRUE(rows.at(6).emphasis);

  // What never travels is a note about the whole list rather than a row of it:
  // squeezed into the value column it was cut down to three words, and what a
  // package leaves behind is the part somebody is checking for.
  EXPECT_EQ(rows.at(7).kind, UI::MetaRowKind::kNote);
  EXPECT_FALSE(rows.at(7).caption.isEmpty());
  EXPECT_FALSE(rows.at(7).detail.isEmpty());
}

TEST(ProfileExportSummaryTest, TheWorkspaceIsTheOnlyRowAnyoneCanTick) {
  // The workspace row *is* its checkbox: a checkbox below a list that never
  // mentions the workspace is how someone ends up unsure whether the list they
  // just read was the whole story.
  for (const auto include : {false, true}) {
    const auto rows =
        ToMetaListRows(BuildProfileExportContents(MeasuredAreas(), include));

    int checkable = 0;
    for (const auto& row : rows) {
      if (!row.checkable) continue;
      ++checkable;
      EXPECT_EQ(row.checked, include);
    }
    EXPECT_EQ(checkable, 1);
  }
}

TEST(ProfileExportSummaryTest, ARowCarryingNothingIsDimmed) {
  // Rows at equal weight make the reader work out which ones matter. An area
  // that is empty, and one that was left out, are both nothing to carry.
  const auto rows = ToMetaListRows(BuildProfileExportContents(
      {{"config", 1000}, {"workspace", 16000}}, false));

  EXPECT_FALSE(rows.at(0).dimmed);  // settings, 1000 bytes
  EXPECT_TRUE(rows.at(1).dimmed);   // saved state, measured nothing
  EXPECT_TRUE(rows.at(4).dimmed);   // workspace, left out
}

TEST(ProfileExportSummaryTest, TheProtectionRowsNameTheMechanism) {
  // Named rather than asserted: someone who knows what these are can check the
  // claim, and someone who does not still reads a specific mechanism rather
  // than a reassurance.
  const auto rows = BuildExportProtectionRows();
  ASSERT_FALSE(rows.isEmpty());

  QStringList values;
  for (const auto& row : rows) {
    EXPECT_FALSE(row.caption.isEmpty());
    values << row.value;
  }

  const auto all = values.join(" ");
  EXPECT_TRUE(all.contains("XChaCha20-Poly1305"));
  EXPECT_TRUE(all.contains("Argon2id"));
  // The keychain is named because its absence is the surprising part: a package
  // has to open on a computer this one's keychain knows nothing about.
  EXPECT_TRUE(all.contains("keychain", Qt::CaseInsensitive) ||
              all.contains("computer", Qt::CaseInsensitive));
}

}  // namespace GpgFrontend::Test
