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

#include "core/profile/ProfileAreaTraits.h"

namespace GpgFrontend::Test {

// The table is the single answer to "what is in a package", so these are the
// tests that decide what a user hands to another person. All of it is pure:
// there is no filesystem here, which is the point of having a table at all.

namespace {

auto Every() -> QList<ProfileArea> {
  return {ProfileArea::kRoot,        ProfileArea::kConfig,
          ProfileArea::kDataObjects, ProfileArea::kSecure,
          ProfileArea::kLogs,        ProfileArea::kModules,
          ProfileArea::kWorkspace,   ProfileArea::kScratch};
}

}  // namespace

TEST(ProfileAreaTraitsTest, EveryAreaHasARow) {
  // A new enumerator without a row would resolve to an empty directory name and
  // quietly collapse onto the profile root.
  for (const auto area : Every()) {
    EXPECT_NE(TraitsForArea(area), nullptr)
        << "area " << static_cast<int>(area) << " has no row in the table";
  }
}

TEST(ProfileAreaTraitsTest, DirectoryNamesAreTheLayoutOnDisk) {
  EXPECT_TRUE(ProfileAreaDirName(ProfileArea::kRoot).isEmpty());
  EXPECT_EQ(ProfileAreaDirName(ProfileArea::kConfig), "config");
  EXPECT_EQ(ProfileAreaDirName(ProfileArea::kDataObjects), "data_objs");
  EXPECT_EQ(ProfileAreaDirName(ProfileArea::kSecure), "secure");
  EXPECT_EQ(ProfileAreaDirName(ProfileArea::kLogs), "logs");
  EXPECT_EQ(ProfileAreaDirName(ProfileArea::kModules), "mods");
  EXPECT_EQ(ProfileAreaDirName(ProfileArea::kWorkspace), "workspace");
  EXPECT_EQ(ProfileAreaDirName(ProfileArea::kScratch), ".scratch");
}

TEST(ProfileAreaTraitsTest, AVirtualisableAreaMustBePackedFromTheAccessor) {
  // The contradiction worth a test of its own: once a driver may hold an area
  // in memory, a packer that walked the tree for it would find nothing and ship
  // a package silently missing that area. On a filesystem driver the same
  // combination would pack it twice.
  for (const auto& row : ProfileAreaTable()) {
    if (row.residency != AreaResidency::kVirtualisable) continue;
    EXPECT_EQ(row.pack_source, AreaPackSource::kAccessor)
        << QString(row.dir).toStdString()
        << " may be held in memory but is packed by walking the tree";
  }
}

TEST(ProfileAreaTraitsTest, OnlySecureMayBeHeldInMemory) {
  // Everything else is opened by something outside this process: GnuPG is given
  // a home directory, QSettings opens a file, modules are dlopen'd.
  for (const auto& row : ProfileAreaTable()) {
    if (row.area == ProfileArea::kSecure) {
      EXPECT_EQ(row.residency, AreaResidency::kVirtualisable);
      continue;
    }
    EXPECT_EQ(row.residency, AreaResidency::kPathRequired)
        << QString(row.dir).toStdString() << " claims it needs no real path";
  }
}

TEST(ProfileAreaTraitsTest, TheProfileMarkerTravels) {
  EXPECT_TRUE(IsIncludedInPackage("profile.json", false));
}

TEST(ProfileAreaTraitsTest, WhatTheProfileIsTravels) {
  EXPECT_TRUE(IsIncludedInPackage("config/config.ini", false));
  EXPECT_TRUE(IsIncludedInPackage("data_objs/abc123", false));
  EXPECT_TRUE(IsIncludedInPackage("db/pubring.kbx", false));
  EXPECT_TRUE(IsIncludedInPackage("dbs/work/gf_keydb.sqlite", false));
  EXPECT_TRUE(IsIncludedInPackage("rpgp_db/keys.db", false));
}

TEST(ProfileAreaTraitsTest, ThisMachinesHistoryStaysHere) {
  EXPECT_FALSE(IsIncludedInPackage("logs/gpgfrontend.log", false));
  EXPECT_FALSE(IsIncludedInPackage("mods/libwhatever.so", false));
  EXPECT_FALSE(IsIncludedInPackage(".scratch/gfp-extract-1/x", false));
}

TEST(ProfileAreaTraitsTest, AnythingUnknownIsRefusedRatherThanShipped) {
  // The whole reason the table replaced a deny-list. None of these are named
  // anywhere: they are refused because nothing said to include them.
  EXPECT_FALSE(IsIncludedInPackage("notes.txt", true));
  EXPECT_FALSE(IsIncludedInPackage("my-stuff/secret-plans.pdf", true));
  EXPECT_FALSE(
      IsIncludedInPackage("profiles/other-profile/profile.json", true));
  EXPECT_FALSE(IsIncludedInPackage("data_objs.quarantine/abc", true));
  EXPECT_FALSE(IsIncludedInPackage("data_objs.gc.json", true));
  EXPECT_FALSE(IsIncludedInPackage(".hidden/thing", true));
}

TEST(ProfileAreaTraitsTest, AHandPlacedKeyDatabaseDoesNotTravel) {
  // Manual mode lets the user point a key database anywhere, including inside
  // the profile. It is an arrangement on their machine, not part of the
  // profile, and shipping it would also ship the path it sits at.
  EXPECT_FALSE(IsIncludedInPackage("work-keys/pubring.kbx", true));
  EXPECT_FALSE(IsManagedKeyDatabasePath("work-keys"));

  EXPECT_TRUE(IsManagedKeyDatabasePath("db"));
  EXPECT_TRUE(IsManagedKeyDatabasePath("dbs/work"));
  EXPECT_TRUE(IsManagedKeyDatabasePath("rpgp_db"));
}

TEST(ProfileAreaTraitsTest, TheWorkspaceTravelsOnlyWhenAsked) {
  EXPECT_FALSE(IsIncludedInPackage("workspace/letter.txt", false));
  EXPECT_TRUE(IsIncludedInPackage("workspace/letter.txt", true));
}

TEST(ProfileAreaTraitsTest, SecureIsPackedFromTheAccessorNotTheTree) {
  // Not a refusal: secure/ does travel. It is emitted from the accessor, so a
  // tree walk must skip it or the area is packed twice.
  EXPECT_FALSE(IsIncludedInPackage("secure/app.key", true));
  EXPECT_FALSE(IsIncludedInPackage("secure/DEADBEEF.key", true));

  const auto* traits = TraitsForArea(ProfileArea::kSecure);
  ASSERT_NE(traits, nullptr);
  EXPECT_EQ(traits->packaging, AreaPackaging::kAlways);
  EXPECT_EQ(traits->pack_source, AreaPackSource::kAccessor);
}

TEST(ProfileAreaTraitsTest, ADeadProcessLeavesNothingInsideAnIncludedArea) {
  EXPECT_FALSE(IsIncludedInPackage("db/S.gpg-agent", false));
  EXPECT_FALSE(IsIncludedInPackage("db/S.gpg-agent.ssh", false));
  EXPECT_FALSE(IsIncludedInPackage("db/S.dirmngr", false));
  EXPECT_FALSE(IsIncludedInPackage("dbs/work/profile.lock", false));
  EXPECT_FALSE(IsIncludedInPackage("profile.lock", false));
  EXPECT_FALSE(IsIncludedInPackage("db/pubring.kbx~", false));
  EXPECT_FALSE(IsIncludedInPackage("db/.DS_Store", false));
  EXPECT_FALSE(IsIncludedInPackage("workspace/Thumbs.db", true));
}

TEST(ProfileAreaTraitsTest, APathThatClimbsOutIsRefused) {
  EXPECT_FALSE(IsIncludedInPackage("../elsewhere", true));
  EXPECT_FALSE(IsIncludedInPackage("db/../../etc/passwd", true));
  EXPECT_FALSE(IsIncludedInPackage("", true));
}

TEST(ProfileAreaTraitsTest, DirectoriesThemselvesAreAdmittedSoTheWalkDescends) {
  // The walk asks about "db" before it can ask about "db/pubring.kbx".
  EXPECT_TRUE(IsIncludedInPackage("db", false));
  EXPECT_TRUE(IsIncludedInPackage("config", false));
  EXPECT_FALSE(IsIncludedInPackage("logs", false));
  EXPECT_FALSE(IsIncludedInPackage("workspace", false));
  EXPECT_TRUE(IsIncludedInPackage("workspace", true));
}

TEST(ProfileAreaTraitsTest, ManagedKeyDatabaseDirsAreTheOnesThatTravel) {
  const auto dirs = ManagedKeyDatabaseDirs();
  EXPECT_TRUE(dirs.contains("db"));
  EXPECT_TRUE(dirs.contains("dbs"));
  EXPECT_TRUE(dirs.contains("rpgp_db"));

  // Areas are not key databases, however much they travel.
  EXPECT_FALSE(dirs.contains("config"));
  EXPECT_FALSE(dirs.contains("secure"));
}

}  // namespace GpgFrontend::Test
