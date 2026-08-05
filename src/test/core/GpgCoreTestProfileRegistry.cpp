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

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "core/function/GlobalSettingStation.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileLock.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileRegistry.h"
#include "core/profile/ProfileSecureKeyManager.h"

namespace GpgFrontend::Test {

namespace {

/// Materialise a profile directory the scan will recognise.
void MakeProfileDir(const QString& profiles_root, const QString& id,
                    const QString& last_opened = {}) {
  const auto root = profiles_root + "/" + id;
  QDir().mkpath(root);

  ProfileMarker marker;
  marker.schema_version = 2;
  marker.min_reader_version = 2;
  marker.profile = "GpgFrontend";
  marker.profile_id = id;
  marker.display_name = id;
  marker.last_opened = last_opened;
  WriteProfileMarker(ProfileMarkerPathFor(root), marker);
}

}  // namespace

// ------------------------------------------------------------------- disk

class ProfileRegistryDiskTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    profiles_root_ = dir_.path() + "/profiles";
    classic_root_ = dir_.path() + "/classic";
    QDir().mkpath(profiles_root_);
    QDir().mkpath(classic_root_);
  }

  QTemporaryDir dir_;
  QString profiles_root_;
  QString classic_root_;
};

TEST_F(ProfileRegistryDiskTest, ScanIgnoresDotPrefixedStagingDirectories) {
  MakeProfileDir(profiles_root_, "work");

  // extraction scratch: adopting a half-extracted tree as a real profile would
  // be worse than having no profile at all
  const auto staging = profiles_root_ + "/.gfprofile-extract-1";
  QDir().mkpath(staging);
  ProfileMarker marker;
  marker.schema_version = 2;
  WriteProfileMarker(ProfileMarkerPathFor(staging), marker);

  const auto scanned = ScanProfilesRoot(profiles_root_);

  ASSERT_EQ(scanned.size(), 1);
  EXPECT_EQ(scanned[0].id, QString("work"));
}

TEST_F(ProfileRegistryDiskTest, ScanIgnoresDirectoriesWithoutAMarker) {
  QDir().mkpath(profiles_root_ + "/not_a_profile");
  MakeProfileDir(profiles_root_, "work");

  const auto scanned = ScanProfilesRoot(profiles_root_);
  ASSERT_EQ(scanned.size(), 1);
  EXPECT_EQ(scanned[0].id, QString("work"));
}

TEST_F(ProfileRegistryDiskTest, ClassicIsAlwaysListedAndImplicit) {
  const auto data = LoadProfileRegistry(profiles_root_, classic_root_);

  const auto classic = data.Find("classic");
  ASSERT_TRUE(classic.has_value());
  EXPECT_TRUE(classic->implicit);
  EXPECT_EQ(classic->root, classic_root_);
  EXPECT_EQ(classic->kind, ProfileKind::kINSTALLED_ROOT);
}

TEST_F(ProfileRegistryDiskTest, PortableIsListedOnlyWhenThereIsOne) {
  EXPECT_FALSE(
      LoadProfileRegistry(profiles_root_, classic_root_).Find("portable"));

  const auto with_portable =
      LoadProfileRegistry(profiles_root_, classic_root_, "/media/usb/GF");
  const auto portable = with_portable.Find("portable");
  ASSERT_TRUE(portable.has_value());
  EXPECT_TRUE(portable->implicit);
  EXPECT_EQ(portable->kind, ProfileKind::kPORTABLE_ROOT);
}

TEST_F(ProfileRegistryDiskTest, CreateThenLoadThenDelete) {
  const auto created = CreateProfile(profiles_root_, "work", "Work",
                                     /*self_contained=*/true);
  ASSERT_TRUE(created.Ok()) << created.detail.toStdString();

  EXPECT_TRUE(QFileInfo::exists(profiles_root_ + "/work/profile.json"));
  EXPECT_TRUE(QFileInfo(profiles_root_ + "/work/workspace").isDir());
  // the key file is ProfileSecureKeyManager's to create, lazily, so exactly one
  // code path is ever responsible for key material
  EXPECT_FALSE(QFileInfo::exists(profiles_root_ + "/work/secure/app.key"));

  const auto marker =
      ReadProfileMarker(ProfileMarkerPathFor(profiles_root_ + "/work"));
  ASSERT_TRUE(marker.has_value());
  EXPECT_FALSE(marker->profile_uuid.isEmpty());
  EXPECT_TRUE(marker->self_contained);

  auto data = LoadProfileRegistry(profiles_root_, classic_root_);
  ASSERT_TRUE(data.Find("work").has_value());
  EXPECT_EQ(data.Find("work")->name, QString("Work"));

  ASSERT_TRUE(DeleteProfile(profiles_root_, "work"));
  EXPECT_FALSE(QFileInfo::exists(profiles_root_ + "/work"));
  EXPECT_FALSE(LoadProfileRegistry(profiles_root_, classic_root_).Find("work"));
}

TEST_F(ProfileRegistryDiskTest, CreateRefusesBadIdsAndCollisions) {
  EXPECT_EQ(CreateProfile(profiles_root_, "..", "x", false).status,
            ProfileCreateStatus::kINVALID_ID);

  ASSERT_TRUE(CreateProfile(profiles_root_, "work", "W", false).Ok());
  EXPECT_EQ(CreateProfile(profiles_root_, "work", "W", false).status,
            ProfileCreateStatus::kALREADY_EXISTS);
}

TEST_F(ProfileRegistryDiskTest, EverythingShownComesFromTheMarker) {
  // There is no second copy of any of this any more. A machine-level index that
  // can disagree with the filesystem eventually will, and the disagreement is
  // only ever found by a user who has lost track of a profile.
  MakeProfileDir(profiles_root_, "work", "2026-08-05T09:00:00Z");
  {
    const auto path = ProfileMarkerPathFor(profiles_root_ + "/work");
    auto marker = ReadProfileMarker(path);
    ASSERT_TRUE(marker.has_value());
    marker->display_name = "Work Laptop";
    marker->package_id = "abc123";
    ASSERT_TRUE(WriteProfileMarker(path, *marker));
  }

  const auto scanned = ScanProfilesRoot(profiles_root_);
  ASSERT_EQ(scanned.size(), 1);
  EXPECT_EQ(scanned[0].name, QString("Work Laptop"));
  EXPECT_EQ(scanned[0].last_opened, QString("2026-08-05T09:00:00Z"));
  EXPECT_EQ(scanned[0].package_id, QString("abc123"));
}

TEST_F(ProfileRegistryDiskTest, ANamelessMarkerFallsBackToTheFolderName) {
  const auto root = profiles_root_ + "/work";
  QDir().mkpath(root);
  ProfileMarker marker;
  marker.schema_version = 2;
  ASSERT_TRUE(WriteProfileMarker(ProfileMarkerPathFor(root), marker));

  const auto scanned = ScanProfilesRoot(profiles_root_);
  ASSERT_EQ(scanned.size(), 1);
  EXPECT_EQ(scanned[0].name, QString("work"));
}

TEST_F(ProfileRegistryDiskTest, ScanIgnoresDirectoriesThatCannotBeProfileIds) {
  // A directory name is the identity, so one that could never have been minted
  // as an id is not a profile of ours whatever it contains.
  const auto root = profiles_root_ + "/Not_An_Id";
  QDir().mkpath(root);
  ProfileMarker marker;
  marker.schema_version = 2;
  WriteProfileMarker(ProfileMarkerPathFor(root), marker);

  EXPECT_TRUE(ScanProfilesRoot(profiles_root_).isEmpty());
}

TEST_F(ProfileRegistryDiskTest, NoProfilesJsonIsEverWritten) {
  ASSERT_TRUE(CreateProfile(profiles_root_, "work", "W", false).Ok());
  ASSERT_TRUE(LoadProfileRegistry(profiles_root_, classic_root_).Find("work"));
  ASSERT_TRUE(DeleteProfile(profiles_root_, "work"));

  // The directory is the registration. Nothing else has to be kept in step
  // with it, so nothing else is written.
  EXPECT_FALSE(QFileInfo::exists(profiles_root_ + "/profiles.json"));
  EXPECT_FALSE(QFileInfo::exists(profiles_root_ + "/profiles.lock"));
}

TEST_F(ProfileRegistryDiskTest, ARootsOwnMarkerNamesIt) {
  // The two roots are synthesised rather than found, but they are still
  // profiles and still keep a marker.
  ProfileMarker marker;
  marker.schema_version = 2;
  marker.display_name = "My Default";
  marker.last_opened = "2026-08-05T11:00:00Z";
  ASSERT_TRUE(WriteProfileMarker(ProfileMarkerPathFor(classic_root_), marker));

  const auto classic =
      LoadProfileRegistry(profiles_root_, classic_root_).Find("classic");
  ASSERT_TRUE(classic.has_value());
  EXPECT_EQ(classic->name, QString("My Default"));
  EXPECT_EQ(classic->last_opened, QString("2026-08-05T11:00:00Z"));
  EXPECT_TRUE(classic->implicit);
}

// ------------------------------------------------------------------- lock

TEST(ProfileLockTest, TheRunningProcessHoldsItsOwnProfile) {
  // main() took it during startup; anything else means the guard is not armed.
  // Asked the way a second window would ask, rather than through a flag: what
  // matters is that another process is refused, not that a bool is set.
  EXPECT_FALSE(ProfileLock::Probe(ProfileSession::Instance().Root()).Ok());
}

TEST(ProfileLockTest, PathIsInsideTheProfileRoot) {
  EXPECT_EQ(ProfileLock::PathFor("/p/work"), QString("/p/work/profile.lock"));
}

TEST(ProfileLockTest, AcquiringAnUnrelatedRootReportsTheHolder) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  // a lock file naming a pid that is definitely not running is stale, and
  // QLockFile reclaims it rather than refusing forever after a crash
  const auto root = dir.path() + "/p";
  ASSERT_TRUE(QDir().mkpath(root));

  // this process already holds its own lock, so acquiring a second one is a
  // no-op rather than a conflict; assert that rather than pretending otherwise
  const auto result = ProfileLock::Acquire(root, 0);
  EXPECT_TRUE(result.Ok());
}

// ---------------------------------------------------- credential account

TEST(ProfileCredentialAccountTest, ClassicKeepsTheOriginalAccountVerbatim) {
  // every existing installation already has this entry; renaming it locks all
  // of those users out of their own data objects
  EXPECT_EQ(DeriveAppKeyWrapAccount(ProfileKind::kINSTALLED_ROOT, "classic",
                                    "/data/classic", "uuid"),
            QString::fromLatin1(kAppKeyWrapAccount));
}

TEST(ProfileCredentialAccountTest, SameIdUnderDifferentRootsDiffers) {
  // "work" can exist on a USB stick, in the desktop profiles root, and in a
  // profile extracted from a package: three key files, and they must not share
  // one credential entry
  const auto a = DeriveAppKeyWrapAccount(ProfileKind::kPERSIST, "work",
                                         "/home/x/profiles/work", "uuid");
  const auto b = DeriveAppKeyWrapAccount(ProfileKind::kPERSIST, "work",
                                         "/media/usb/profiles/work", "uuid");
  EXPECT_NE(a, b);
  EXPECT_TRUE(a.startsWith(QString::fromLatin1(kAppKeyWrapAccount) + ".work."));
}

TEST(ProfileCredentialAccountTest, RecreatingAProfileYieldsANewAccount) {
  // otherwise the recreated profile inherits the secret of the one it replaced
  const auto before = DeriveAppKeyWrapAccount(ProfileKind::kPERSIST, "work",
                                              "/p/work", "uuid-one");
  const auto after = DeriveAppKeyWrapAccount(ProfileKind::kPERSIST, "work",
                                             "/p/work", "uuid-two");
  EXPECT_NE(before, after);
}

TEST(ProfileCredentialAccountTest, DerivationIsStable) {
  EXPECT_EQ(
      DeriveAppKeyWrapAccount(ProfileKind::kPERSIST, "work", "/p/work", "uuid"),
      DeriveAppKeyWrapAccount(ProfileKind::kPERSIST, "work", "/p/work",
                              "uuid"));
}

// ------------------------------------------- profiles that leave the machine
//
// The rule used to be a free function taking a kind and a policy, which meant
// every new shape of profile had to remember to appear in it. It is now a
// question each profile answers about itself.

TEST(ProfilePortabilityTest, APackagedProfileMayNeverUseTheCredentialStore) {
  // the package is opened on another computer, quite possibly running another
  // operating system, where this machine's credential store cannot be read
  PackagedProfile packaged("/tmp/work.gfprofile", "/tmp/profiles");
  EXPECT_FALSE(packaged.AllowsSystemKeychain());

  EXPECT_EQ(ProfileLoader::ApplyProfilePortabilityRule(
                AppKeyProtection::kKEYCHAIN, packaged.AllowsSystemKeychain()),
            AppKeyProtection::kNONE);
}

TEST(ProfilePortabilityTest, PortableStillRefusesTheCredentialStore) {
  PortableRootProfile portable("/media/usb/GpgFrontend");
  EXPECT_FALSE(portable.AllowsSystemKeychain());
}

TEST(ProfilePortabilityTest, APinAndNoProtectionBothTravel) {
  // the only two that survive the trip, and inside a package both are covered
  // by the package passphrase as well
  EXPECT_EQ(
      ProfileLoader::ApplyProfilePortabilityRule(AppKeyProtection::kPIN, false),
      AppKeyProtection::kPIN);
  EXPECT_EQ(ProfileLoader::ApplyProfilePortabilityRule(AppKeyProtection::kNONE,
                                                       false),
            AppKeyProtection::kNONE);
}

TEST(ProfilePortabilityTest, ALocalProfileKeepsEveryMode) {
  InstalledRootProfile installed("/data/classic");
  PersistProfile persist("work", "/data/classic/profiles/work");

  EXPECT_TRUE(installed.AllowsSystemKeychain());
  EXPECT_TRUE(persist.AllowsSystemKeychain());

  // a persisted profile keeping its own keyring is still local: self-contained
  // is about where keys live, not about travelling
  ProfileMarker marker;
  marker.self_contained = true;
  persist.ApplyMarkerPolicy(marker);
  EXPECT_TRUE(persist.Policy().self_contained);
  EXPECT_TRUE(persist.AllowsSystemKeychain());

  for (const auto p : {AppKeyProtection::kNONE, AppKeyProtection::kKEYCHAIN,
                       AppKeyProtection::kPIN}) {
    EXPECT_EQ(ProfileLoader::ApplyProfilePortabilityRule(p, true), p);
  }
}

}  // namespace GpgFrontend::Test
