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
#include "ui/function/KeyDatabaseDisplayNames.h"

namespace GpgFrontend::Test {

namespace {

using UI::KeyDatabaseKindDisplayName;
using UI::KeyDatabaseTravelsWithProfile;

constexpr auto kRoot = "/home/someone/.local/share/GpgFrontend";

}  // namespace

TEST(KeyDatabaseDisplayNamesTest, EveryKindReadsDifferently) {
  const auto local_default =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, false);
  const auto own_default =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, true);
  const auto managed =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kMANAGED, true);
  const auto external =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kEXTERNAL, false);

  for (const auto& phrase : {local_default, own_default, managed, external}) {
    EXPECT_FALSE(phrase.isEmpty());
  }

  EXPECT_NE(local_default, own_default);
  EXPECT_NE(local_default, managed);
  EXPECT_NE(managed, external);
}

TEST(KeyDatabaseDisplayNamesTest, TheDefaultKindAloneCannotWordTheRow) {
  // ClassifyKeyDatabase() settles DEFAULT on the reserved name alone, because
  // its stored path is replaced on every read. But a self-contained profile's
  // default database is `@profile/db`: it travels, and calling it this
  // computer's would be exactly backwards.
  EXPECT_NE(KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, true),
            KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, false));
}

TEST(KeyDatabaseDisplayNamesTest, TheManagedDirectoriesTravel) {
  EXPECT_TRUE(KeyDatabaseTravelsWithProfile("@profile/db", kRoot));
  EXPECT_TRUE(
      KeyDatabaseTravelsWithProfile(QString("%1/dbs/Work").arg(kRoot), kRoot));
}

TEST(KeyDatabaseDisplayNamesTest, InsideTheProfileIsNotTheSameAsTravelling) {
  // The documented trap. A database under `workspace/` is inside the profile
  // root and still never travels, because the packer does not carry that
  // directory -- and a phrase promising a recipient a keyring that never
  // shipped is worse than no phrase.
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile(
      QString("%1/workspace/keys").arg(kRoot), kRoot));
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile("@profile/workspace/keys", kRoot));
}

TEST(KeyDatabaseDisplayNamesTest, BothSpellingsOfOnePathAgree) {
  // Which spelling reached the settings file is an accident of which build
  // wrote it; they name one directory and must answer identically.
  EXPECT_EQ(
      KeyDatabaseTravelsWithProfile("@profile/dbs/Work", kRoot),
      KeyDatabaseTravelsWithProfile(QString("%1/dbs/Work").arg(kRoot), kRoot));
}

TEST(KeyDatabaseDisplayNamesTest, AnythingOutsideTheProfileStaysBehind) {
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile("/home/someone/.gnupg", kRoot));
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile({}, kRoot));
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile("@profile/db", {}));
}

}  // namespace GpgFrontend::Test
