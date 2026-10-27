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
#include "ui/dialog/profile/ProfileCreateSummary.h"
#include "ui/function/KeyDatabaseDisplayNames.h"

namespace GpgFrontend::Test {

namespace {

using UI::CanAdvanceProfileCreate;
using UI::DescribeProfileKeyringChoice;
using UI::IsProfileNameAcceptable;
using UI::ProfileCreateStep;
using UI::ProfileCreateStepTitle;

}  // namespace

TEST(ProfileCreateSummaryTest, EveryStepHasATitle) {
  for (const auto step : {ProfileCreateStep::kName, ProfileCreateStep::kKeys,
                          ProfileCreateStep::kSettings}) {
    EXPECT_FALSE(ProfileCreateStepTitle(step).isEmpty());
  }
}

TEST(ProfileCreateSummaryTest, ABlankNameIsNotAName) {
  for (const auto* blank : {"", " ", "\t", "  \n "}) {
    EXPECT_FALSE(IsProfileNameAcceptable(QString::fromLatin1(blank)))
        << '"' << blank << '"';
  }
}

TEST(ProfileCreateSummaryTest, AnyVisibleTextIsAName) {
  // Nothing is derived from the name, so nothing about it can collide.
  for (const auto& name : {u"Work"_s, u"  padded  "_s, u"a/b"_s, u"."_s}) {
    EXPECT_TRUE(IsProfileNameAcceptable(name)) << name.toStdString();
  }
}

TEST(ProfileCreateSummaryTest, TheNameStepWaitsForAName) {
  EXPECT_FALSE(CanAdvanceProfileCreate(ProfileCreateStep::kName, {}));
  EXPECT_FALSE(CanAdvanceProfileCreate(ProfileCreateStep::kName, u"  "_s));
  EXPECT_TRUE(CanAdvanceProfileCreate(ProfileCreateStep::kName, u"Work"_s));
}

TEST(ProfileCreateSummaryTest, TheKeysStepAlwaysHasAnAnswer) {
  // One of the two keyrings is always selected.
  EXPECT_TRUE(CanAdvanceProfileCreate(ProfileCreateStep::kKeys, {}));
}

TEST(ProfileCreateSummaryTest, CreatingStillNeedsAName) {
  // The last step's button creates the profile; it must not be more lenient
  // than the dialog this wizard replaced.
  EXPECT_FALSE(CanAdvanceProfileCreate(ProfileCreateStep::kSettings, {}));
  EXPECT_TRUE(CanAdvanceProfileCreate(ProfileCreateStep::kSettings, u"Work"_s));
}

TEST(ProfileCreateSummaryTest, TheKeyringChoiceUsesTheSettingsNames) {
  // The wizard and the Key Databases settings must call the built-in database
  // the same thing, or the choice made here cannot be found again there.
  for (const auto gnupg : {false, true}) {
    EXPECT_EQ(DescribeProfileKeyringChoice(true, gnupg).title,
              UI::KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, true));
    EXPECT_EQ(DescribeProfileKeyringChoice(false, gnupg).title,
              UI::KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, false));
  }
}

TEST(ProfileCreateSummaryTest, GnuPGIsOnlyPromisedWhenItIsThere) {
  EXPECT_TRUE(
      DescribeProfileKeyringChoice(false, true).detail.contains("GnuPG"));
  EXPECT_FALSE(
      DescribeProfileKeyringChoice(false, false).detail.contains("GnuPG"));
  EXPECT_FALSE(
      DescribeProfileKeyringChoice(true, true).detail.contains("GnuPG"));
}

TEST(ProfileCreateSummaryTest, EveryKeyringChoiceIsExplained) {
  for (const auto self_contained : {false, true}) {
    for (const auto gnupg : {false, true}) {
      const auto text = DescribeProfileKeyringChoice(self_contained, gnupg);
      EXPECT_FALSE(text.title.isEmpty());
      EXPECT_FALSE(text.detail.isEmpty());
    }
  }
}

}  // namespace GpgFrontend::Test
