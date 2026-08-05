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

#include <QDir>

#include "core/profile/Profile.h"
#include "core/profile/Profile.h"

namespace GpgFrontend::Test {

// Every question below used to be a comparison against a kind enum somewhere
// else in the codebase — in the settings station, in the key manager, in the
// launcher — and each one was a place where a new shape of profile could be
// forgotten. They are now answered by the profile itself, and this is the
// table.

namespace {

constexpr auto kInstalledRoot = "/data/classic";
constexpr auto kPortableRoot = "/media/usb/GpgFrontend";

}  // namespace

// ------------------------------------------------------------- the keychain

TEST(ProfileKindsTest, OnlyProfilesThatStayHereMayUseTheKeychain) {
  // A key sealed with one machine's credential store cannot be opened on
  // another, so honouring the request would strand the profile rather than
  // protect it.
  EXPECT_TRUE(InstalledRootProfile(kInstalledRoot).AllowsSystemKeychain());
  EXPECT_TRUE(PersistProfile("work", QString(kInstalledRoot) + "/profiles/work")
                  .AllowsSystemKeychain());

  EXPECT_FALSE(PortableRootProfile(kPortableRoot).AllowsSystemKeychain());
  EXPECT_FALSE(PackagedProfile("/tmp/work.gfprofile", "/srv/profiles")
                   .AllowsSystemKeychain());
}

// -------------------------------------------------------------- key rotation

TEST(ProfileKindsTest, APackagedProfileNeverRotatesItsKey) {
  // A rotated key would be written into storage this process deletes on the
  // way out, taking everything it had encrypted with it.
  EXPECT_FALSE(PackagedProfile("/tmp/work.gfprofile", "/srv/profiles")
                   .AllowsKeyRotation());

  EXPECT_TRUE(InstalledRootProfile(kInstalledRoot).AllowsKeyRotation());
  EXPECT_TRUE(PortableRootProfile(kPortableRoot).AllowsKeyRotation());
  EXPECT_TRUE(PersistProfile("work", QString(kInstalledRoot) + "/profiles/work")
                  .AllowsKeyRotation());
}

// ------------------------------------------------------------ where settings

TEST(ProfileKindsTest, RootedProfilesAreIniBackedOnEveryPlatform) {
  // Assertable unconditionally, unlike the platform branch it replaced: a
  // native store is keyed only by organization and application name, so every
  // profile would share one registry key or plist, and the file would sit
  // outside the profile root where no package could carry it.
  PortableRootProfile portable("/srv/p");
  PersistProfile persist("work", "/srv/p");
  PackagedProfile packaged("/tmp/work.gfprofile", "/srv/profiles");

  EXPECT_EQ(portable.SettingsFilePath(), QString("/srv/p/config/config.ini"));
  EXPECT_EQ(persist.SettingsFilePath(), QString("/srv/p/config/config.ini"));
  EXPECT_EQ(packaged.SettingsFilePath(),
            packaged.Root() + "/config/config.ini");
}

TEST(ProfileKindsTest, TheInstalledRootKeepsItsPlatformStore) {
  const auto path = InstalledRootProfile("/srv/p").SettingsFilePath();
#ifdef Q_OS_WINDOWS
  EXPECT_TRUE(path.endsWith("/config.ini"));
#else
  // empty means "use the native QSettings store", which is what every existing
  // POSIX installation already writes to
  EXPECT_TRUE(path.isEmpty());
#endif
}

// ------------------------------------------------------------- the registry

TEST(ProfileKindsTest, OnlyAPersistedProfileBelongsInTheRegistry) {
  // The two roots exist whether or not the registry has heard of them, and a
  // packaged profile is deliberately never recorded — the profile manager lists
  // what this machine keeps.
  EXPECT_TRUE(PersistProfile("work", QString(kInstalledRoot) + "/profiles/work")
                  .IsRegistrable());

  EXPECT_FALSE(InstalledRootProfile(kInstalledRoot).IsRegistrable());
  EXPECT_FALSE(PortableRootProfile(kPortableRoot).IsRegistrable());
  EXPECT_FALSE(
      PackagedProfile("/tmp/work.gfprofile", "/srv/profiles").IsRegistrable());
}

// ---------------------------------------------------------- disposable trees

TEST(ProfileKindsTest, OnlyAPackagedProfileIsTransient) {
  EXPECT_TRUE(
      PackagedProfile("/tmp/work.gfprofile", "/srv/profiles").IsTransient());

  EXPECT_FALSE(InstalledRootProfile(kInstalledRoot).IsTransient());
  EXPECT_FALSE(PortableRootProfile(kPortableRoot).IsTransient());
  EXPECT_FALSE(
      PersistProfile("work", QString(kInstalledRoot) + "/profiles/work")
          .IsTransient());
}

// --------------------------------------------------------------- relaunching

TEST(ProfileKindsTest, EachShapeKnowsWhatReopensItInANewProcess) {
  // Two profiles cannot share a process, so opening one is always a launch, and
  // the profile is the one place that knows what to put on the command line.
  EXPECT_EQ(PersistProfile("work", "/srv/p").LaunchArguments(),
            QStringList({"--profile", "work"}));

  EXPECT_EQ(
      PackagedProfile("/tmp/work.gfprofile", "/srv/profiles").LaunchArguments(),
      QStringList({"/tmp/work.gfprofile"}));

  // The two implicit profiles are what the resolver picks when nothing is
  // named, so naming them would only pin a decision that is already made.
  EXPECT_TRUE(InstalledRootProfile(kInstalledRoot).LaunchArguments().isEmpty());
  EXPECT_TRUE(PortableRootProfile(kPortableRoot).LaunchArguments().isEmpty());
}

// ------------------------------------------------------------------ policy

TEST(ProfileKindsTest, PortableIsSelfContainedWhateverItsMarkerSays) {
  // For this shape the location *is* the decision: a marker saying otherwise
  // came from a profile that was copied here.
  PortableRootProfile portable(kPortableRoot);
  EXPECT_TRUE(portable.Policy().self_contained);

  ProfileMarker marker;
  marker.self_contained = false;
  portable.ApplyMarkerPolicy(marker);
  EXPECT_TRUE(portable.Policy().self_contained);
}

TEST(ProfileKindsTest, EveryOtherShapeTakesThePolicyFromItsMarker) {
  PersistProfile persist("work", "/srv/p");
  EXPECT_FALSE(persist.Policy().self_contained);

  ProfileMarker marker;
  marker.self_contained = true;
  persist.ApplyMarkerPolicy(marker);
  EXPECT_TRUE(persist.Policy().self_contained);
}

// ------------------------------------------------------- roots hold profiles

TEST(ProfileKindsTest, ARootIsWhereThePersistedProfilesLive) {
  // This is what makes a root profile special: it is the directory that
  // contains the others, which is why there are only ever two of them.
  EXPECT_EQ(InstalledRootProfile(kInstalledRoot).ProfilesDir(),
            QString(kInstalledRoot) + "/profiles");
  EXPECT_EQ(PortableRootProfile(kPortableRoot).ProfilesDir(),
            QString(kPortableRoot) + "/profiles");
}

// --------------------------------------------------------- selection to type

TEST(ProfileKindsTest, EverySelectionProducesTheMatchingProfile) {
  ProfileSelection selection;
  selection.profiles_root = QString(kInstalledRoot) + "/profiles";

  selection.kind = ProfileKind::kINSTALLED_ROOT;
  selection.root = kInstalledRoot;
  EXPECT_EQ(MakeProfile(selection)->Kind(), ProfileKind::kINSTALLED_ROOT);

  selection.kind = ProfileKind::kPORTABLE_ROOT;
  selection.root = kPortableRoot;
  EXPECT_EQ(MakeProfile(selection)->Kind(), ProfileKind::kPORTABLE_ROOT);

  selection.kind = ProfileKind::kPERSIST;
  selection.id = "work";
  selection.root = QString(kInstalledRoot) + "/profiles/work";
  EXPECT_EQ(MakeProfile(selection)->Kind(), ProfileKind::kPERSIST);
  EXPECT_EQ(MakeProfile(selection)->Id(), QString("work"));

  selection.kind = ProfileKind::kPACKAGED;
  selection.package_path = "/tmp/work.gfprofile";
  const auto packaged = MakeProfile(selection);
  EXPECT_EQ(packaged->Kind(), ProfileKind::kPACKAGED);
  // A package's root is derived from its own path, so it is known before
  // anything is extracted — which is what lets the loader lock it first.
  EXPECT_EQ(packaged->Root(), ProfileSessionRoot(selection.profiles_root,
                                                 selection.package_path));
}

}  // namespace GpgFrontend::Test
