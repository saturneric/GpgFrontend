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
#include "ui/dialog/profile/ProfilePackageMeta.h"

namespace GpgFrontend::UI::Test {

namespace {

auto SealedHeader() -> ProfilePackageHeader {
  ProfilePackageHeader header;
  header.created = "2026-08-01T09:00:00Z";
  header.writer = "2.1.9";
  header.writer_stable = true;
  header.protection = ProfilePackageProtection::kPIN;
  return header;
}

auto IndexOf(const QVector<MetaListRow>& rows, const QString& caption) -> int {
  for (int i = 0; i < rows.size(); ++i) {
    if (rows.at(i).caption == caption) return i;
  }
  return -1;
}

auto CaptionOf(const QVector<MetaListRow>& rows, const QString& caption)
    -> std::optional<MetaListRow> {
  const auto at = IndexOf(rows, caption);
  return at < 0 ? std::optional<MetaListRow>{} : rows.at(at);
}

}  // namespace

TEST(ProfilePackageMetaTest, EveryClaimIsMarkedAsOne) {
  // The header is attacker-controllable, and ProfilePackage.h is explicit that
  // it may reject but must never be believed. Marking each of its rows is what
  // makes the panel render the caveat, so an unmarked one would put a claim on
  // screen as a fact.
  const auto rows =
      BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"), SealedHeader(), true);

  ASSERT_TRUE(MetaListNeedsCaveat(rows));

  bool past_the_heading = false;
  for (const auto& row : rows) {
    if (row.kind == MetaRowKind::kSection) {
      past_the_heading = true;
      continue;
    }
    if (past_the_heading)
      EXPECT_TRUE(row.unverified) << row.caption.toStdString();
    if (!past_the_heading)
      EXPECT_FALSE(row.unverified) << row.caption.toStdString();
  }
  EXPECT_TRUE(past_the_heading) << "the claims were never separated out";
}

TEST(ProfilePackageMetaTest, FactsComeBeforeClaims) {
  // A fact and a claim shown as one list read as two facts, so the heading is
  // not decoration: nothing this application established for itself may appear
  // below it.
  const auto rows =
      BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"), SealedHeader(), true);

  int heading = -1;
  for (int i = 0; i < rows.size(); ++i) {
    if (rows.at(i).kind == MetaRowKind::kSection) heading = i;
  }
  ASSERT_GT(heading, 0);

  EXPECT_EQ(rows.first().value, "work.gfp");
  EXPECT_TRUE(rows.first().emphasis);
  EXPECT_LT(IndexOf(rows, "Folder"), heading);
}

TEST(ProfilePackageMetaTest, AnUnreadableHeaderClaimsNothing) {
  // A file that is not a package at all still has a name and a place, and those
  // are worth naming; a heading over an empty group is not.
  const auto rows = BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"),
                                            SealedHeader(), false);

  EXPECT_FALSE(MetaListNeedsCaveat(rows));
  for (const auto& row : rows) {
    EXPECT_NE(row.kind, MetaRowKind::kSection);
  }
  EXPECT_TRUE(CaptionOf(rows, "Folder").has_value());
}

TEST(ProfilePackageMetaTest, ADevelopmentBuildSaysSo) {
  auto header = SealedHeader();
  header.writer_stable = false;

  const auto rows =
      BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"), header, true);
  const auto written_by = CaptionOf(rows, "Written by");
  ASSERT_TRUE(written_by.has_value());

  EXPECT_TRUE(written_by->value.contains("2.1.9"));
  EXPECT_TRUE(written_by->value.contains("development", Qt::CaseInsensitive));

  header.writer_stable = true;
  EXPECT_FALSE(CaptionOf(BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"),
                                                 header, true),
                         "Written by")
                   ->value.contains("development", Qt::CaseInsensitive));
}

TEST(ProfilePackageMetaTest, AnUnreadableDateIsShownVerbatim) {
  // A claim this application cannot parse is still a claim somebody may
  // recognise as wrong, which is the entire point of showing it.
  auto header = SealedHeader();
  header.created = "not-a-date";

  const auto created = CaptionOf(
      BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"), header, true),
      "Created");
  ASSERT_TRUE(created.has_value());
  EXPECT_EQ(created->value, "not-a-date");
}

TEST(ProfilePackageMetaTest, ADateThisMachineCanReadIsSaidInItsOwnWords) {
  const auto created = CaptionOf(
      BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"), SealedHeader(), true),
      "Created");
  ASSERT_TRUE(created.has_value());

  // Not asserted against a formatting of its own: what a short date looks like
  // is the locale's business, and this machine's locale writes the year with
  // two digits. What matters is that the ISO instant the file stores is not
  // what the reader is shown.
  EXPECT_NE(created->value, "2026-08-01T09:00:00Z");
  EXPECT_FALSE(created->value.contains("T09:00:00Z"));
  EXPECT_FALSE(created->value.isEmpty());
}

TEST(ProfilePackageMetaTest, OnlyAnUnsealedFileGetsAProtectionRow) {
  // Every file this application writes is sealed, so saying so on each of them
  // is a row that never varies -- and a row that never varies is one the eye
  // stops reading, including on the one file where it would have mattered.
  auto header = SealedHeader();
  EXPECT_FALSE(CaptionOf(BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"),
                                                 header, true),
                         "Protection")
                   .has_value());

  header.protection = ProfilePackageProtection::kNONE;
  const auto protection = CaptionOf(
      BuildProfilePackageRows(QFileInfo("/tmp/work.gfp"), header, true),
      "Protection");
  ASSERT_TRUE(protection.has_value());

  EXPECT_TRUE(protection->danger);
  EXPECT_TRUE(protection->unverified);
  EXPECT_FALSE(protection->detail.isEmpty());
}

TEST(ProfilePackageMetaTest, AFileThatIsNotThereYetSaysNothingAboutItsSize) {
  // The write-back and export prompts describe a file before it exists. A zero
  // is not what "not written yet" looks like.
  const auto rows = BuildProfilePackageRows(
      QFileInfo("/tmp/gpgfrontend-not-there-at-all.gfp"), {}, false);

  EXPECT_FALSE(CaptionOf(rows, "Size").has_value());
  EXPECT_FALSE(CaptionOf(rows, "Modified").has_value());
  EXPECT_TRUE(CaptionOf(rows, "File").has_value());
}

TEST(ProfilePackageMetaTest, AnUnreadableVolumeSaysNothingAboutFreeSpace) {
  const auto rows =
      BuildProfilePackageDestinationRows(QFileInfo("/tmp/out.gfp"), -1);

  EXPECT_FALSE(CaptionOf(rows, "Free space").has_value());
  EXPECT_EQ(CaptionOf(rows, "File")->value, "out.gfp");

  const auto with_room =
      BuildProfilePackageDestinationRows(QFileInfo("/tmp/out.gfp"), 4096);
  EXPECT_TRUE(CaptionOf(with_room, "Free space").has_value());
}

}  // namespace GpgFrontend::UI::Test
