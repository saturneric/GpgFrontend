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
#include "ui/dialog/profile/ProfileListSummary.h"
#include "ui/function/DateTimeText.h"

namespace GpgFrontend::Test {

namespace {

using UI::ProfileLastOpenedText;
using UI::ProfileRowStatus;
using UI::ProfileRowViewFor;

auto MakeEntry(const QString& id, bool implicit = false)
    -> ProfileRegistryEntry {
  ProfileRegistryEntry e;
  e.id = id;
  e.name = u"Name of "_s + id;
  e.root = u"/nowhere/"_s + id;
  e.kind = implicit ? ProfileKind::kINSTALLED_ROOT : ProfileKind::kPERSIST;
  e.implicit = implicit;
  return e;
}

}  // namespace

TEST(ProfileListSummaryTest, TheCurrentProfileIsThisWindow) {
  const auto row = ProfileRowViewFor(MakeEntry(u"a"_s), u"a"_s, true, false);
  EXPECT_TRUE(row.is_current);
  EXPECT_EQ(row.status_kind, ProfileRowStatus::kThisWindow);
  EXPECT_FALSE(row.status.isEmpty());
  EXPECT_FALSE(row.can_open);
  EXPECT_FALSE(row.can_delete);
  EXPECT_TRUE(row.can_reveal);
}

TEST(ProfileListSummaryTest, TheNameIsShownAsIs) {
  // The "(open)" suffix it used to carry repeated the Status column.
  const auto row = ProfileRowViewFor(MakeEntry(u"a"_s), u"a"_s, true, false);
  EXPECT_EQ(row.name, u"Name of a"_s);
}

TEST(ProfileListSummaryTest, AnotherProfileCanBeOpenedAndDeleted) {
  const auto row = ProfileRowViewFor(MakeEntry(u"b"_s), u"a"_s, true, false);
  EXPECT_FALSE(row.is_current);
  EXPECT_EQ(row.status_kind, ProfileRowStatus::kNone);
  EXPECT_TRUE(row.status.isEmpty());
  EXPECT_TRUE(row.can_open);
  EXPECT_TRUE(row.can_delete);
  EXPECT_TRUE(row.can_reveal);
}

TEST(ProfileListSummaryTest, AnImplicitProfileCannotBeDeleted) {
  const auto row =
      ProfileRowViewFor(MakeEntry(u"classic"_s, true), u"a"_s, true, false);
  EXPECT_TRUE(row.can_open);
  EXPECT_FALSE(row.can_delete);
}

TEST(ProfileListSummaryTest, AMissingFolderIsNotCreatedAndCannotBeShown) {
  const auto row = ProfileRowViewFor(MakeEntry(u"b"_s), u"a"_s, false, false);
  EXPECT_EQ(row.status_kind, ProfileRowStatus::kNotCreated);
  EXPECT_FALSE(row.can_reveal);
}

TEST(ProfileListSummaryTest, AHeldLockIsOpenElsewhere) {
  const auto row = ProfileRowViewFor(MakeEntry(u"b"_s), u"a"_s, true, true);
  EXPECT_EQ(row.status_kind, ProfileRowStatus::kOpenElsewhere);
  EXPECT_FALSE(row.status.isEmpty());
}

TEST(ProfileListSummaryTest, ALastOpenedStampIsLocalNotIso) {
  const QLocale locale(QLocale::English, QLocale::UnitedStates);
  const auto iso = u"2026-10-27T07:53:58Z"_s;
  const auto text = ProfileLastOpenedText(iso, locale);
  EXPECT_NE(text, iso);
  EXPECT_EQ(text,
            UI::ShortDateTimeText(
                QDateTime::fromString(iso, Qt::ISODate).toLocalTime(), locale));
}

TEST(ProfileListSummaryTest, NoUsableStampReadsNever) {
  const auto never = ProfileLastOpenedText({});
  EXPECT_FALSE(never.isEmpty());
  EXPECT_EQ(ProfileLastOpenedText(u"not a date"_s), never);
}

}  // namespace GpgFrontend::Test
