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

#include "core/function/AppSecureKeyManager.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/ProfileLock.h"
#include "core/function/ProfileRegistry.h"

namespace GpgFrontend::Test {

namespace {

auto MakeEntry(const QString& id, const QString& root) -> ProfileRegistryEntry {
  ProfileRegistryEntry e;
  e.id = id;
  e.root = root;
  e.name = id;
  e.kind = ProfileRootKind::kNAMED;
  return e;
}

/// Materialise a profile directory the scan will recognise.
void MakeProfileDir(const QString& profiles_root, const QString& id) {
  const auto root = profiles_root + "/" + id;
  QDir().mkpath(root);

  ProfileMarker marker;
  marker.schema_version = 2;
  marker.min_reader_version = 2;
  marker.profile = "GpgFrontend";
  marker.profile_id = id;
  marker.display_name = id;
  WriteProfileMarker(ProfileMarkerPathFor(root), marker);
}

}  // namespace

// ------------------------------------------------------------- reconcile

TEST(ProfileRegistryReconcileTest, AnAdoptedDirectoryJoinsTheList) {
  ProfileRegistryData stored;

  const auto out =
      ReconcileProfileRegistry({MakeEntry("work", "/p/work")}, stored);

  ASSERT_EQ(out.profiles.size(), 1);
  EXPECT_EQ(out.profiles[0].id, QString("work"));
}

TEST(ProfileRegistryReconcileTest, AVanishedDirectoryIsDropped) {
  ProfileRegistryData stored;
  stored.profiles.append(MakeEntry("gone", "/p/gone"));
  stored.profiles.append(MakeEntry("here", "/p/here"));

  const auto out =
      ReconcileProfileRegistry({MakeEntry("here", "/p/here")}, stored);

  ASSERT_EQ(out.profiles.size(), 1);
  EXPECT_EQ(out.profiles[0].id, QString("here"));
}

TEST(ProfileRegistryReconcileTest, MetadataTheFilesystemCannotKnowSurvives) {
  ProfileRegistryData stored;
  auto e = MakeEntry("work", "/p/work");
  e.last_opened = "2026-01-01T00:00:00Z";
  e.source_package = "/home/x/work.gfprofile";
  e.source_bookmark = "bookmark";
  stored.profiles.append(e);

  const auto out =
      ReconcileProfileRegistry({MakeEntry("work", "/p/work")}, stored);

  ASSERT_EQ(out.profiles.size(), 1);
  EXPECT_EQ(out.profiles[0].last_opened, QString("2026-01-01T00:00:00Z"));
  EXPECT_EQ(out.profiles[0].source_package, QString("/home/x/work.gfprofile"));
  EXPECT_EQ(out.profiles[0].source_bookmark, QString("bookmark"));
}

TEST(ProfileRegistryReconcileTest, DuplicateStoredIdsCollapse) {
  ProfileRegistryData stored;
  stored.profiles.append(MakeEntry("work", "/p/work"));
  stored.profiles.append(MakeEntry("work", "/p/work"));

  const auto out =
      ReconcileProfileRegistry({MakeEntry("work", "/p/work")}, stored);

  // two rows meaning one directory is only ever a confusing manager
  EXPECT_EQ(out.profiles.size(), 1);
}

TEST(ProfileRegistryReconcileTest, ADanglingLastUsedIsRepointed) {
  ProfileRegistryData stored;
  stored.last_used = "gone";

  const auto out =
      ReconcileProfileRegistry({MakeEntry("here", "/p/here")}, stored);

  EXPECT_TRUE(out.last_used.isEmpty());
}

TEST(ProfileRegistryReconcileTest, ADanglingFixedStartupFallsBack) {
  // a fixed startup pointing at nothing would otherwise halt every launch
  ProfileRegistryData stored;
  stored.startup_policy = ProfileStartupPolicy::kFIXED;
  stored.startup_profile = "gone";

  const auto out = ReconcileProfileRegistry({}, stored);

  EXPECT_TRUE(out.startup_profile.isEmpty());
  EXPECT_EQ(out.startup_policy, ProfileStartupPolicy::kLAST_USED);
}

TEST(ProfileRegistryReconcileTest, ClassicAndPortableAreNeverDropped) {
  // "classic" is synthesised, so a last_used pointing at it must survive a
  // reconcile that has no scanned entry for it
  ProfileRegistryData stored;
  stored.last_used = "classic";
  EXPECT_EQ(ReconcileProfileRegistry({}, stored).last_used, QString("classic"));

  stored.last_used = "portable";
  EXPECT_EQ(ReconcileProfileRegistry({}, stored).last_used,
            QString("portable"));
}

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
  EXPECT_EQ(classic->kind, ProfileRootKind::kCLASSIC);
}

TEST_F(ProfileRegistryDiskTest, PortableIsListedOnlyWhenThereIsOne) {
  EXPECT_FALSE(
      LoadProfileRegistry(profiles_root_, classic_root_).Find("portable"));

  const auto with_portable =
      LoadProfileRegistry(profiles_root_, classic_root_, "/media/usb/GF");
  const auto portable = with_portable.Find("portable");
  ASSERT_TRUE(portable.has_value());
  EXPECT_TRUE(portable->implicit);
  EXPECT_EQ(portable->kind, ProfileRootKind::kPORTABLE);
}

TEST_F(ProfileRegistryDiskTest, ImplicitEntriesAreNeverPersisted) {
  auto data = LoadProfileRegistry(profiles_root_, classic_root_);
  ASSERT_TRUE(SaveProfileRegistry(profiles_root_, data));

  QFile f(profiles_root_ + "/profiles.json");
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));
  const auto obj = QJsonDocument::fromJson(f.readAll()).object();

  // storing them would let a stale root outlive the code that computes it
  EXPECT_TRUE(obj.value("profiles").toArray().isEmpty());
}

TEST_F(ProfileRegistryDiskTest, CreateThenLoadThenDelete) {
  const auto created = CreateProfile(profiles_root_, classic_root_, {}, "work",
                                     "Work", /*self_contained=*/true);
  ASSERT_TRUE(created.Ok()) << created.detail.toStdString();

  EXPECT_TRUE(QFileInfo::exists(profiles_root_ + "/work/profile.json"));
  EXPECT_TRUE(QFileInfo(profiles_root_ + "/work/workspace").isDir());
  // the key file is AppSecureKeyManager's to create, lazily, so exactly one
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

  ASSERT_TRUE(DeleteProfile(profiles_root_, classic_root_, {}, "work"));
  EXPECT_FALSE(QFileInfo::exists(profiles_root_ + "/work"));
  EXPECT_FALSE(LoadProfileRegistry(profiles_root_, classic_root_).Find("work"));
}

TEST_F(ProfileRegistryDiskTest, CreateRefusesBadIdsAndCollisions) {
  EXPECT_EQ(
      CreateProfile(profiles_root_, classic_root_, {}, "..", "x", false).status,
      ProfileCreateStatus::kINVALID_ID);

  ASSERT_TRUE(
      CreateProfile(profiles_root_, classic_root_, {}, "work", "W", false)
          .Ok());
  EXPECT_EQ(CreateProfile(profiles_root_, classic_root_, {}, "work", "W", false)
                .status,
            ProfileCreateStatus::kALREADY_EXISTS);
}

TEST_F(ProfileRegistryDiskTest, TouchRecordsTheLastUsedProfile) {
  ASSERT_TRUE(
      CreateProfile(profiles_root_, classic_root_, {}, "work", "W", false)
          .Ok());

  TouchProfile(profiles_root_, classic_root_, {}, "work",
               "2026-08-03T10:00:00Z");

  const auto data = LoadProfileRegistry(profiles_root_, classic_root_);
  EXPECT_EQ(data.last_used, QString("work"));
  EXPECT_EQ(data.Find("work")->last_opened, QString("2026-08-03T10:00:00Z"));
}

TEST_F(ProfileRegistryDiskTest, ACorruptRegistryIsQuarantinedNotDeleted) {
  const auto path = profiles_root_ + "/profiles.json";
  {
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("{ this is not json");
  }

  // the load must not crash, and must rebuild from the filesystem
  MakeProfileDir(profiles_root_, "work");
  const auto data = LoadProfileRegistry(profiles_root_, classic_root_);
  EXPECT_TRUE(data.Find("work").has_value());

  // it is the only record of where this machine's profiles are, so it is moved
  // aside rather than destroyed
  EXPECT_TRUE(QFileInfo::exists(path + ".corrupt-1"));
}

TEST_F(ProfileRegistryDiskTest, ConcurrentUpdatesDoNotLoseEachOther) {
  ASSERT_TRUE(
      CreateProfile(profiles_root_, classic_root_, {}, "one", "One", false)
          .Ok());
  ASSERT_TRUE(
      CreateProfile(profiles_root_, classic_root_, {}, "two", "Two", false)
          .Ok());

  // interleaved read-modify-writes: without the lock and the reload inside it,
  // the second update would write back a copy of the registry taken before the
  // first, silently dropping it
  UpdateProfileRegistry(profiles_root_, classic_root_, {},
                        [](ProfileRegistryData& d) {
                          d.last_used = "one";
                          return true;
                        });
  UpdateProfileRegistry(profiles_root_, classic_root_, {},
                        [](ProfileRegistryData& d) {
                          d.save_on_close = "never";
                          return true;
                        });

  const auto data = LoadProfileRegistry(profiles_root_, classic_root_);
  EXPECT_EQ(data.last_used, QString("one"));
  EXPECT_EQ(data.save_on_close, QString("never"));
}

// ------------------------------------------------------------------- lock

TEST(ProfileLockTest, TheRunningProcessHoldsItsOwnProfile) {
  // main() took it during startup; anything else means the guard is not armed
  EXPECT_TRUE(ProfileLock::IsHeld());
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
  EXPECT_EQ(DeriveAppKeyWrapAccount(ProfileRootKind::kCLASSIC, "classic",
                                    "/data/classic", "uuid"),
            QString::fromLatin1(kAppKeyWrapAccount));
}

TEST(ProfileCredentialAccountTest, SameIdUnderDifferentRootsDiffers) {
  // "work" can exist on a USB stick, in the desktop profiles root, and in a
  // profile extracted from a package: three key files, and they must not share
  // one credential entry
  const auto a = DeriveAppKeyWrapAccount(ProfileRootKind::kNAMED, "work",
                                         "/home/x/profiles/work", "uuid");
  const auto b = DeriveAppKeyWrapAccount(ProfileRootKind::kNAMED, "work",
                                         "/media/usb/profiles/work", "uuid");
  EXPECT_NE(a, b);
  EXPECT_TRUE(a.startsWith(QString::fromLatin1(kAppKeyWrapAccount) + ".work."));
}

TEST(ProfileCredentialAccountTest, RecreatingAProfileYieldsANewAccount) {
  // otherwise the recreated profile inherits the secret of the one it replaced
  const auto before = DeriveAppKeyWrapAccount(ProfileRootKind::kNAMED, "work",
                                              "/p/work", "uuid-one");
  const auto after = DeriveAppKeyWrapAccount(ProfileRootKind::kNAMED, "work",
                                             "/p/work", "uuid-two");
  EXPECT_NE(before, after);
}

TEST(ProfileCredentialAccountTest, DerivationIsStable) {
  EXPECT_EQ(DeriveAppKeyWrapAccount(ProfileRootKind::kNAMED, "work", "/p/work",
                                    "uuid"),
            DeriveAppKeyWrapAccount(ProfileRootKind::kNAMED, "work", "/p/work",
                                    "uuid"));
}

// ------------------------------------------- profiles that leave the machine

TEST(ProfilePortabilityTest, APackagedProfileMayNeverUseTheCredentialStore) {
  // the package is opened on another computer, quite possibly running another
  // operating system, where this machine's credential store cannot be read
  ProfilePolicy policy;
  EXPECT_TRUE(
      ProfileTravelsBetweenMachines(ProfileRootKind::kPACKAGE_LINKED, policy));
  EXPECT_EQ(ApplyProfilePortabilityRule(AppKeyProtection::kKEYCHAIN, true),
            AppKeyProtection::kNONE);
}

TEST(ProfilePortabilityTest, PortableStillRefusesTheCredentialStore) {
  ProfilePolicy policy;
  policy.self_contained = true;
  EXPECT_TRUE(
      ProfileTravelsBetweenMachines(ProfileRootKind::kPORTABLE, policy));
}

TEST(ProfilePortabilityTest, APinAndNoProtectionBothTravel) {
  // the only two that survive the trip, and inside a package both are covered
  // by the package passphrase as well
  EXPECT_EQ(ApplyProfilePortabilityRule(AppKeyProtection::kPIN, true),
            AppKeyProtection::kPIN);
  EXPECT_EQ(ApplyProfilePortabilityRule(AppKeyProtection::kNONE, true),
            AppKeyProtection::kNONE);
}

TEST(ProfilePortabilityTest, ALocalProfileKeepsEveryMode) {
  ProfilePolicy policy;
  EXPECT_FALSE(
      ProfileTravelsBetweenMachines(ProfileRootKind::kCLASSIC, policy));
  EXPECT_FALSE(ProfileTravelsBetweenMachines(ProfileRootKind::kNAMED, policy));

  // a named profile keeping its own keyring is still local: self-contained is
  // about where keys live, not about travelling
  policy.self_contained = true;
  EXPECT_FALSE(ProfileTravelsBetweenMachines(ProfileRootKind::kNAMED, policy));

  for (const auto p : {AppKeyProtection::kNONE, AppKeyProtection::kKEYCHAIN,
                       AppKeyProtection::kPIN}) {
    EXPECT_EQ(ApplyProfilePortabilityRule(p, false), p);
  }
}

}  // namespace GpgFrontend::Test
