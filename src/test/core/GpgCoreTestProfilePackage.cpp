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

#include "core/function/AESCryptoHelper.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfilePackage.h"
#include "core/profile/ProfileSecureKeyManager.h"

namespace GpgFrontend::Test {

namespace {

void WriteFile(const QString &path, const QByteArray &content) {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(content);
}

/// A profile with one file in every area a package carries, plus a couple that
/// it must leave behind.
void MakeProfile(const QString &root) {
  WriteFile(root + "/profile.json", R"({"schema_version":3})");
  WriteFile(root + "/config/config.ini", "[basic]\nlanguage=en_US\n");
  WriteFile(root + "/data_objs/abcd", "encrypted-object");
  WriteFile(root + "/secure/app.key", "SEALED-BY-THIS-MACHINE");
  WriteFile(root + "/secure/DEADBEEF.key", "rotated-key");
  WriteFile(root + "/db/pubring.kbx", "keyring");
  WriteFile(root + "/workspace/notes.txt", "cleartext draft");

  WriteFile(root + "/logs/gpgfrontend.log", "log line");
  WriteFile(root + "/mods/module.so", "module");
  WriteFile(root + "/data_objs.quarantine/broken", "broken");
  WriteFile(root + "/profile.lock", "1234");
}

auto ExportRequestFor(const QString &root, const QString &profiles_root,
                      const QString &destination) -> ProfileExportRequest {
  ProfileExportRequest request;
  request.profile_root = root;
  request.profiles_root = profiles_root;
  request.dest_path = destination;
  request.app_key = GFBuffer(QByteArray("0123456789abcdef0123456789abcdef"));
  request.settings.insert("basic/language", "en_US");
  request.manifest.schema_version = 3;
  request.manifest.min_reader_version = 2;
  request.manifest.app_profile = "GpgFrontend";
  request.manifest.display_name = "Work";
  request.manifest.profile_id = "work";
  return request;
}

}  // namespace

// ------------------------------------------------------------- header rules

TEST(ProfilePackageHeaderTest, RoundTrips) {
  ProfilePackageHeader header;
  header.writer = "2.2.2";
  header.writer_stable = true;
  header.created = "2026-08-03T10:00:00Z";
  header.protection = ProfilePackageProtection::kPIN;

  const auto bytes = EncodeProfilePackageHeader(header);
  const auto view = ParseProfilePackageHeader(bytes);

  ASSERT_TRUE(view.Ok());
  EXPECT_EQ(view.header.writer, "2.2.2");
  EXPECT_TRUE(view.header.writer_stable);
  EXPECT_EQ(view.header.created, "2026-08-03T10:00:00Z");
  EXPECT_EQ(view.header.protection, ProfilePackageProtection::kPIN);
  EXPECT_EQ(view.header_bytes, bytes);
  EXPECT_EQ(view.body_offset, bytes.size());
}

TEST(ProfilePackageHeaderTest, SomethingElseEntirelyIsNotAPackage) {
  EXPECT_EQ(ParseProfilePackageHeader("hello there").status,
            ProfilePackageHeaderStatus::kNOT_A_PACKAGE);
  EXPECT_EQ(ParseProfilePackageHeader({}).status,
            ProfilePackageHeaderStatus::kNOT_A_PACKAGE);
}

TEST(ProfilePackageHeaderTest, ATruncatedFileIsNotMistakenForACorruptOne) {
  const auto bytes = EncodeProfilePackageHeader({});

  EXPECT_EQ(ParseProfilePackageHeader(bytes.left(10)).status,
            ProfilePackageHeaderStatus::kTRUNCATED);
  EXPECT_EQ(ParseProfilePackageHeader(bytes.left(bytes.size() - 5)).status,
            ProfilePackageHeaderStatus::kTRUNCATED);
}

TEST(ProfilePackageHeaderTest, AnAbsurdLengthIsRefusedRatherThanAllocated) {
  auto bytes = EncodeProfilePackageHeader({});
  bytes[kProfilePackageMagicLength] = static_cast<char>(0x7F);

  EXPECT_EQ(ParseProfilePackageHeader(bytes).status,
            ProfilePackageHeaderStatus::kMALFORMED);
}

// The one judgement the plaintext header is allowed to make on its own, and it
// is a refusal: it saves the key derivation on a file we could not use anyway.
TEST(ProfilePackageHeaderTest, AFormatFromTheFutureIsRefusedBeforeAnyKdf) {
  ProfilePackageHeader header;
  header.format_version = kProfilePackageFormatVersion + 1;
  header.writer = "9.9.9";

  const auto view =
      ParseProfilePackageHeader(EncodeProfilePackageHeader(header));

  EXPECT_EQ(view.status, ProfilePackageHeaderStatus::kTOO_NEW);
  EXPECT_TRUE(view.detail.contains("9.9.9"));
}

TEST(ProfilePackageHeaderTest, AMinReaderFromTheFutureIsRefusedToo) {
  ProfilePackageHeader header;
  header.min_reader = kProfilePackageFormatVersion + 1;

  EXPECT_EQ(
      ParseProfilePackageHeader(EncodeProfilePackageHeader(header)).status,
      ProfilePackageHeaderStatus::kTOO_NEW);
}

// ----------------------------------------------------------------- manifest

TEST(ProfilePackageManifestTest, RoundTrips) {
  ProfilePackageManifest manifest;
  manifest.schema_version = 3;
  manifest.min_reader_version = 2;
  manifest.app_profile = "GpgFrontend";
  manifest.display_name = "Work";
  manifest.profile_id = "work";
  manifest.package_id = "9f3c";
  manifest.workspace_included = true;
  manifest.self_contained = true;
  manifest.key_databases.append({"Default", "@profile/db", "gpg", false});
  manifest.key_databases.append({"System", "/home/x/.gnupg", "gpg", true});

  const auto parsed =
      ParseProfilePackageManifest(EncodeProfilePackageManifest(manifest));

  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->schema_version, 3);
  EXPECT_EQ(parsed->display_name, "Work");
  EXPECT_EQ(parsed->package_id, "9f3c");
  EXPECT_TRUE(parsed->workspace_included);
  EXPECT_TRUE(parsed->self_contained);
  ASSERT_EQ(parsed->key_databases.size(), 2);
  EXPECT_EQ(parsed->key_databases.at(0).stored_path, "@profile/db");
  EXPECT_TRUE(parsed->key_databases.at(1).external);
}

TEST(ProfilePackageManifestTest, RubbishIsNotAManifest) {
  EXPECT_FALSE(ParseProfilePackageManifest("not json").has_value());
  EXPECT_FALSE(ParseProfilePackageManifest("[1,2,3]").has_value());
}

// Anyone can edit the plaintext header of a file they hold; nobody can edit the
// sealed manifest. A disagreement is therefore tampering.
TEST(ProfilePackageManifestTest, AnEditedHeaderDisagreesWithTheSealedManifest) {
  ProfilePackageHeader header;
  header.created = "2026-08-03T10:00:00Z";
  const auto bytes = EncodeProfilePackageHeader(header);

  ProfilePackageManifest manifest;
  manifest.protection = ProfilePackageProtectionToString(header.protection);
  manifest.header_digest = ProfilePackageHeaderDigest(bytes);

  EXPECT_TRUE(
      CheckPackageHeaderAgainstManifest(header, bytes, manifest).isEmpty());

  ProfilePackageHeader edited = header;
  edited.created = "1999-01-01T00:00:00Z";
  const auto edited_bytes = EncodeProfilePackageHeader(edited);

  EXPECT_FALSE(CheckPackageHeaderAgainstManifest(edited, edited_bytes, manifest)
                   .isEmpty());
}

TEST(ProfilePackageManifestTest, AVersionThatDisagreesIsTampering) {
  ProfilePackageHeader header;
  ProfilePackageManifest manifest;
  manifest.protection = ProfilePackageProtectionToString(header.protection);
  manifest.format_version = header.format_version + 1;

  EXPECT_FALSE(CheckPackageHeaderAgainstManifest(
                   header, EncodeProfilePackageHeader(header), manifest)
                   .isEmpty());
}

// ------------------------------------------------------- key database paths

TEST(ProfilePackagePathTest, PathsInsideTheProfileAreMadeToTravel) {
  QContainer<KeyDatabaseItemSO> databases;

  KeyDatabaseItemSO inside;
  inside.name = "Default";
  inside.path = "/srv/profiles/work/db";
  databases.push_back(inside);

  const auto packed =
      RewriteKeyDatabaseListForPacking(databases, "/srv/profiles/work");

  ASSERT_EQ(packed.size(), 1);
  EXPECT_EQ(packed.at(0).path, "@profile/db");
}

TEST(ProfilePackagePathTest, PathsOutsideTheProfileAreLeftExactlyAsTheyAre) {
  // Inventing a path here would produce an empty database wearing the name of
  // a real one, which is worse than a database that says it is unavailable.
  QContainer<KeyDatabaseItemSO> databases;

  KeyDatabaseItemSO outside;
  outside.name = "System";
  outside.path = "/home/eric/.gnupg";
  databases.push_back(outside);

  const auto packed =
      RewriteKeyDatabaseListForPacking(databases, "/srv/profiles/work");

  ASSERT_EQ(packed.size(), 1);
  EXPECT_EQ(packed.at(0).path, "/home/eric/.gnupg");
  EXPECT_TRUE(DescribeKeyDatabasesForManifest(packed).at(0).external);
}

TEST(ProfilePackagePathTest, RewritingIsIdempotent) {
  QContainer<KeyDatabaseItemSO> databases;

  KeyDatabaseItemSO already;
  already.name = "Default";
  already.path = "@profile/db";
  databases.push_back(already);

  const auto once = RewriteKeyDatabaseListForPacking(databases, "/srv/p/work");
  const auto twice = RewriteKeyDatabaseListForPacking(once, "/srv/p/work");

  EXPECT_EQ(once.at(0).path, "@profile/db");
  EXPECT_EQ(twice.at(0).path, "@profile/db");
  EXPECT_FALSE(DescribeKeyDatabasesForManifest(twice).at(0).external);
}

// ------------------------------------------------------------------ staging

TEST(ProfileStagingTest, EverythingThatTravelsIsCopiedAndNothingElseIs) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto staging = dir.path() + "/staging";
  ASSERT_TRUE(StageProfileTree(root, staging, false).ok);

  const auto tree = staging + "/profile";
  EXPECT_TRUE(QFileInfo::exists(tree + "/profile.json"));
  EXPECT_TRUE(QFileInfo::exists(tree + "/data_objs/abcd"));
  EXPECT_TRUE(QFileInfo::exists(tree + "/db/pubring.kbx"));
  EXPECT_TRUE(QFileInfo::exists(tree + "/secure/DEADBEEF.key"));

  // logs and modules are this machine's, not the profile's; the quarantine and
  // the lock describe a history and a process that do not travel
  EXPECT_FALSE(QFileInfo::exists(tree + "/logs"));
  EXPECT_FALSE(QFileInfo::exists(tree + "/mods"));
  EXPECT_FALSE(QFileInfo::exists(tree + "/data_objs.quarantine"));
  EXPECT_FALSE(QFileInfo::exists(tree + "/profile.lock"));

  // the on-disk key may be sealed by this machine's credential store, so the
  // unwrapped one is written separately and this copy must never be taken
  EXPECT_FALSE(QFileInfo::exists(tree + "/secure/app.key"));

  // off unless asked for
  EXPECT_FALSE(QFileInfo::exists(tree + "/workspace"));
}

TEST(ProfileStagingTest, TheWorkspaceComesWhenItIsAskedFor) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto staging = dir.path() + "/staging";
  ASSERT_TRUE(StageProfileTree(root, staging, true).ok);

  EXPECT_TRUE(QFileInfo::exists(staging + "/profile/workspace/notes.txt"));
}

TEST(ProfileStagingTest, ARootProfileLeavesTheProfilesItContainsBehind) {
  // The root profiles — installed and portable — have the profiles root inside
  // them, so the scratch directory of their own export lands in the tree being
  // copied. Following it copied the staging tree into itself until the path
  // grew too long to open, which is how this reached the user: an export of
  // the default profile that could not finish.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  // a sibling profile and a live window's session root, both under the root
  // profile and neither one part of it
  WriteFile(root + "/profiles/other/profile.json", R"({"schema_version":3})");
  WriteFile(root + "/profiles/.abcd/profile.json", R"({"schema_version":3})");

  const auto staging = root + "/profiles/.gfprofile-staging-eeee";
  const auto result = StageProfileTree(root, staging, false);
  ASSERT_TRUE(result.ok) << result.error.toStdString();

  const auto tree = staging + "/profile";
  EXPECT_TRUE(QFileInfo::exists(tree + "/profile.json"));
  EXPECT_FALSE(QFileInfo::exists(tree + "/profiles"));
}

TEST(ProfileStagingTest, AnExistingStagingDirectoryIsRefused) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  ASSERT_TRUE(QDir().mkpath(dir.path() + "/staging"));
  EXPECT_FALSE(StageProfileTree(root, dir.path() + "/staging", false).ok);
}

// -------------------------------------------------------------- round trips

TEST(ProfilePackageRoundTripTest, AProtectedPackageComesBackByteForByte) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(QString("correct horse battery staple"));
  request.include_workspace = true;

  const auto written = ExportProfilePackage(request);
  ASSERT_TRUE(written.ok) << written.error.toStdString();
  ASSERT_TRUE(QFileInfo::exists(package));

  // the staging tree held an unprotected copy of the application key; it must
  // not have outlived the export
  const auto leftovers =
      QDir(dir.path()).entryList({".gfprofile-*"}, QDir::Dirs | QDir::Hidden);
  EXPECT_TRUE(leftovers.isEmpty());

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, request.passphrase);
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  EXPECT_EQ(read.manifest.display_name, "Work");
  EXPECT_EQ(read.manifest.schema_version, 3);
  EXPECT_TRUE(read.manifest.workspace_included);
  EXPECT_EQ(read.manifest.app_key_protection, "none");
  EXPECT_FALSE(read.manifest.package_id.isEmpty());

  const auto tree = extracted + "/profile";
  QFile object(tree + "/data_objs/abcd");
  ASSERT_TRUE(object.open(QIODevice::ReadOnly));
  EXPECT_EQ(object.readAll(), QByteArray("encrypted-object"));

  QFile notes(tree + "/workspace/notes.txt");
  ASSERT_TRUE(notes.open(QIODevice::ReadOnly));
  EXPECT_EQ(notes.readAll(), QByteArray("cleartext draft"));

  // written unwrapped from the key in hand, not copied from disk
  QFile key(tree + "/secure/app.key");
  ASSERT_TRUE(key.open(QIODevice::ReadOnly));
  EXPECT_EQ(key.readAll(), QByteArray("0123456789abcdef0123456789abcdef"));

  QSettings settings(tree + "/config/config.ini", QSettings::IniFormat);
  EXPECT_EQ(settings.value("basic/language").toString(), "en_US");
}

TEST(ProfilePackageRoundTripTest,
     ARootProfileExportsWithItsScratchInsideItself) {
  // The layout of the installed and portable root profiles: the scratch
  // directory is made under <root>/profiles, which is inside the tree being
  // packed. Every export of the default profile went through here.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);
  WriteFile(root + "/profiles/other/data_objs/ffff", "another profile");

  const auto profiles_root = root + "/profiles";
  const auto package = dir.path() + "/root.gfprofile";
  auto request = ExportRequestFor(root, profiles_root, package);
  request.protection = ProfilePackageProtection::kNONE;

  const auto written = ExportProfilePackage(request);
  ASSERT_TRUE(written.ok) << written.error.toStdString();

  const auto leftovers =
      QDir(profiles_root)
          .entryList({".gfprofile-*"}, QDir::Dirs | QDir::Hidden);
  EXPECT_TRUE(leftovers.isEmpty());

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  const auto tree = extracted + "/profile";
  EXPECT_TRUE(QFileInfo::exists(tree + "/data_objs/abcd"));

  // the other profiles on this machine are not part of this one
  EXPECT_FALSE(QFileInfo::exists(tree + "/profiles"));
}

TEST(ProfilePackageRoundTripTest, AnUnprotectedPackageNeedsNoPassphrase) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/open.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;

  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto inspected = InspectProfilePackage(package);
  ASSERT_TRUE(inspected.Ok());
  EXPECT_EQ(inspected.header.protection, ProfilePackageProtection::kNONE);

  const auto read = ReadProfilePackage(package, dir.path() + "/out", {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();
  EXPECT_EQ(read.manifest.protection, "none");
}

TEST(ProfilePackageRoundTripTest, TheWrongPassphraseFailsCleanly) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.passphrase = GFBuffer(QString("right"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto extracted = dir.path() + "/extracted";
  const auto read =
      ReadProfilePackage(package, extracted, GFBuffer(QString("wrong")));

  EXPECT_EQ(read.status, ProfilePackageReadStatus::kBAD_PASSPHRASE);
  // nothing half-unpacked is left where it could later be adopted
  EXPECT_FALSE(QFileInfo::exists(extracted));
}

TEST(ProfilePackageRoundTripTest, EditingTheHeaderIsReportedAsTampering) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/open.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  // Only an unprotected package can be edited and still parse — with a
  // passphrase the Poly1305 tag fails first, which is the stronger answer.
  QByteArray bytes;
  {
    QFile file(package);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    bytes = file.readAll();
  }

  const auto view = ParseProfilePackageHeader(bytes);
  ASSERT_TRUE(view.Ok());

  auto header = view.header;
  header.writer = "somebody-else";
  const auto forged = EncodeProfilePackageHeader(header);

  {
    QFile file(package);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(forged);
    file.write(bytes.mid(view.body_offset));
  }

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});

  EXPECT_EQ(read.status, ProfilePackageReadStatus::kTAMPERED);
  EXPECT_FALSE(QFileInfo::exists(extracted));
}

TEST(ProfilePackageRoundTripTest, TheOriginalSurvivesAFailedExport) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  QByteArray before;
  {
    QFile file(package);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    before = file.readAll();
  }

  // a profile that is not there fails after the destination is already known,
  // which is exactly the moment an in-place writer would have truncated it
  auto doomed = ExportRequestFor(dir.path() + "/gone", dir.path(), package);
  EXPECT_FALSE(ExportProfilePackage(doomed).ok);

  QFile file(package);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(file.readAll(), before);
}

// ------------------------------------------------------------------ adopting

TEST(ProfileAdoptionTest, AnImportedProfileGetsAFreshIdentity) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);
  WriteFile(root + "/profile.json",
            R"({"schema_version":3,"min_reader_version":2,)"
            R"("profile":"GpgFrontend","profile_uuid":"source-uuid",)"
            R"("credential_account":"app-key-wrap.work.deadbeef"})");

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.passphrase = GFBuffer(QString("pass"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, request.passphrase);
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  const auto imported = dir.path() + "/profiles/copy";
  EXPECT_TRUE(
      AdoptExtractedProfile(extracted, imported, "copy", "Copy", read.manifest)
          .isEmpty());

  const auto marker = ReadProfileMarker(ProfileMarkerPathFor(imported));
  ASSERT_TRUE(marker.has_value());

  // Two roots sharing an identity would fight over one credential-store entry,
  // and deleting one would take the other's data with it. The identity is the
  // directory name, so there is only ever one of them to keep in step.
  EXPECT_NE(marker->profile_uuid, "source-uuid");
  EXPECT_EQ(marker->profile_uuid, "copy");
  EXPECT_TRUE(marker->credential_account.isEmpty());
  EXPECT_EQ(marker->profile_id, "copy");
  EXPECT_EQ(marker->display_name, "Copy");

  // the key inside a package is unprotected, and the machine that wrote it may
  // have recorded a keychain that does not exist here
  QSettings settings(imported + "/config/config.ini", QSettings::IniFormat);
  EXPECT_EQ(settings.value("advanced/app_key_protection").toString(), "none");
}

TEST(ProfileAdoptionTest, AnOccupiedRootIsNeverOverwritten) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto extracted = dir.path() + "/extracted";
  ASSERT_TRUE(ReadProfilePackage(package, extracted, {}).Ok());

  const auto occupied = dir.path() + "/occupied";
  WriteFile(occupied + "/secure/app.key", "somebody's key");

  EXPECT_FALSE(
      AdoptExtractedProfile(extracted, occupied, "occupied", "Taken", {})
          .isEmpty());

  QFile key(occupied + "/secure/app.key");
  ASSERT_TRUE(key.open(QIODevice::ReadOnly));
  EXPECT_EQ(key.readAll(), QByteArray("somebody's key"));
}

// -------------------------------------------------------- temporary sessions

TEST(ProfileSessionRootTest, IsDerivedFromThePackageAndIsTransient) {
  const auto a = ProfileSessionRoot("/srv/profiles", "/tmp/work.gfprofile");
  const auto b = ProfileSessionRoot("/srv/profiles", "/tmp/other.gfprofile");

  // Derived rather than minted, so two windows work out the same directory for
  // the same file and the second one can be told it is already open.
  EXPECT_EQ(a, ProfileSessionRoot("/srv/profiles", "/tmp/work.gfprofile"));
  EXPECT_NE(a, b);

  // Dot-prefixed: transient, and never adopted by the profile scan.
  EXPECT_TRUE(a.startsWith("/srv/profiles/."));
  EXPECT_EQ(QFileInfo(a).fileName().size(), 33);  // the dot and 32 hex

  EXPECT_TRUE(ProfileSessionRoot({}, "/tmp/work.gfprofile").isEmpty());
  EXPECT_TRUE(ProfileSessionRoot("/srv/profiles", {}).isEmpty());
}

TEST(PackagedProfileTest, OpensAPackageAndWritesItBackToTheSameFile) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.passphrase = GFBuffer(QString("pass"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  PackagedProfile packaged(package, dir.path());
  EXPECT_EQ(packaged.Inspect().status, ProfileMountStatus::kNEEDS_PASSPHRASE);

  const auto mounted = packaged.Mount({request.passphrase, 3});
  ASSERT_TRUE(mounted.Ok()) << mounted.detail.toStdString();
  EXPECT_EQ(packaged.Root(), ProfileSessionRoot(dir.path(), package));
  EXPECT_EQ(packaged.Manifest().display_name, "Work");

  // The session is a profile root like any other: the tree is at the top, not
  // under `profile/`, and the extraction scratch did not outlive the call.
  EXPECT_TRUE(QFileInfo::exists(packaged.Root() + "/data_objs/abcd"));
  EXPECT_TRUE(QDir(dir.path())
                  .entryList({".gfprofile-*"}, QDir::Dirs | QDir::Hidden)
                  .isEmpty());

  // One secret, and it protects the key here too: a package carries its key
  // unprotected, and leaving that copy plaintext on this disk for as long as
  // the window is open would be strictly worse than the file it came from.
  const auto extracted =
      GFBufferFactory::FromFile(packaged.Root() + "/secure/app.key");
  ASSERT_TRUE(extracted.has_value());
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(*extracted));

  const auto opened_key =
      ProfileSecureKeyManager::UnsealKey(request.passphrase, {}, *extracted);
  ASSERT_TRUE(opened_key.has_value());
  EXPECT_EQ(*opened_key, request.app_key);

  // And the stored protection says what the file actually is, or the next
  // thing to read it resolves a protection the key does not have.
  QSettings session_settings(packaged.Root() + "/config/config.ini",
                             QSettings::IniFormat);
  EXPECT_EQ(session_settings.value("advanced/app_key_protection").toString(),
            QString("pin"));

  WriteFile(packaged.Root() + "/data_objs/abcd", "changed-in-the-session");

  // Everything a write-back needs that is not in the running process comes
  // from the profile itself, aimed back at the file it was opened from.
  auto back = packaged.WriteBackRequest();
  back.app_key = request.app_key;
  back.settings = request.settings;
  back.manifest.schema_version = request.manifest.schema_version;
  back.manifest.min_reader_version = request.manifest.min_reader_version;
  back.manifest.app_profile = request.manifest.app_profile;
  ASSERT_TRUE(ExportProfilePackage(back).ok);

  const auto reread = dir.path() + "/reread";
  const auto read = ReadProfilePackage(package, reread, request.passphrase);
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  // Same file, same identity, new contents.
  EXPECT_EQ(read.manifest.package_id, packaged.Manifest().package_id);
  QFile object(reread + "/profile/data_objs/abcd");
  ASSERT_TRUE(object.open(QIODevice::ReadOnly));
  EXPECT_EQ(object.readAll(), QByteArray("changed-in-the-session"));

  // And the tree does not outlive the session that unpacked it: what is in it
  // is a copy of the profile's key material.
  const auto session_root = packaged.Root();
  packaged.Unmount(ProfileUnmountMode::kNORMAL);
  EXPECT_FALSE(QFileInfo::exists(session_root));
}

TEST(PackagedProfileTest, TheWrongPassphraseLeavesNothingBehind) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.passphrase = GFBuffer(QString("pass"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  PackagedProfile packaged(package, dir.path());
  const auto mounted = packaged.Mount({GFBuffer(QString("wrong")), 3});
  EXPECT_EQ(mounted.status, ProfileMountStatus::kBAD_PASSPHRASE);

  // Nothing was extracted, so there is nothing holding an unprotected key.
  EXPECT_FALSE(QFileInfo::exists(ProfileSessionRoot(dir.path(), package)));
  EXPECT_TRUE(QDir(dir.path())
                  .entryList({".gfprofile-*"}, QDir::Dirs | QDir::Hidden)
                  .isEmpty());
}

TEST(PackagedProfileTest, ALayoutFromTheFutureIsRefusedBeforeAdoption) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  request.manifest.min_reader_version = 99;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  PackagedProfile packaged(package, dir.path());
  const auto mounted = packaged.Mount({{}, 3});
  EXPECT_EQ(mounted.status, ProfileMountStatus::kTOO_NEW);

  // Refused before anything was adopted: a package this build must not touch
  // leaves no trace of having been opened.
  EXPECT_FALSE(QFileInfo::exists(ProfileSessionRoot(dir.path(), package)));
}

TEST(PackagedProfileTest, AStaleSessionFromACrashedProcessIsReplaced) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfprofile";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  // What a process that died mid-session leaves: the directory is there and
  // nobody holds it. Opening the same package again must not be refused.
  const auto session = ProfileSessionRoot(dir.path(), package);
  WriteFile(session + "/data_objs/abcd", "from a process that is gone");

  PackagedProfile packaged(package, dir.path());
  // An unprotected package needs no secret, and says so.
  EXPECT_EQ(packaged.Inspect().status, ProfileMountStatus::kOK);

  const auto mounted = packaged.Mount({{}, 3});
  ASSERT_TRUE(mounted.Ok()) << mounted.detail.toStdString();

  // Nothing protected the package, so there is no secret to protect the key
  // with either. Inventing one would be a second thing to forget.
  const auto extracted =
      GFBufferFactory::FromFile(packaged.Root() + "/secure/app.key");
  ASSERT_TRUE(extracted.has_value());
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(*extracted));

  QSettings session_settings(packaged.Root() + "/config/config.ini",
                             QSettings::IniFormat);
  EXPECT_EQ(session_settings.value("advanced/app_key_protection").toString(),
            QString("none"));

  QFile object(session + "/data_objs/abcd");
  ASSERT_TRUE(object.open(QIODevice::ReadOnly));
  EXPECT_EQ(object.readAll(), QByteArray("encrypted-object"));
}

TEST(ProfileSweepTest, OnlyTransientRootsNobodyIsUsingAreRemoved) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto stale = dir.path() + "/.aaaa";
  const auto keep = dir.path() + "/.bbbb";
  const auto scratch = dir.path() + "/.gfprofile-staging-cccc";
  const auto real = dir.path() + "/dddd";

  WriteFile(stale + "/profile.json", R"({"schema_version":3})");
  WriteFile(keep + "/profile.json", R"({"schema_version":3})");
  WriteFile(scratch + "/profile/profile.json", R"({"schema_version":3})");
  WriteFile(real + "/profile.json", R"({"schema_version":3})");

  EXPECT_EQ(SweepTransientProfileRoots(dir.path(), keep), 1);

  EXPECT_FALSE(QFileInfo::exists(stale));
  EXPECT_TRUE(QFileInfo::exists(keep));

  // Scratch carries no marker of its own, and an export running in another
  // window is producing one right now.
  EXPECT_TRUE(QFileInfo::exists(scratch));

  // A profile this machine keeps is not transient, whatever else is true.
  EXPECT_TRUE(QFileInfo::exists(real));
}

TEST(PackagedProfileTest, CarriesItsOwnSecretAndWritesBackWhereItCameFrom) {
  // The session state used to be a process-global struct beside the packing
  // code. It is now the profile object itself, which is what makes "one
  // session per process" a consequence of the design rather than a convention.
  PackagedProfile packaged("/tmp/work.gfprofile", "/srv/profiles");

  EXPECT_EQ(packaged.PackagePath(), "/tmp/work.gfprofile");
  EXPECT_EQ(packaged.ProfilesRoot(), "/srv/profiles");
  EXPECT_EQ(packaged.Root(),
            ProfileSessionRoot("/srv/profiles", "/tmp/work.gfprofile"));
  EXPECT_TRUE(packaged.IsTransient());

  // A write-back aims at the file this session came from, never at a new one.
  const auto request = packaged.WriteBackRequest();
  EXPECT_EQ(request.dest_path, "/tmp/work.gfprofile");
  EXPECT_EQ(request.profile_root, packaged.Root());
  EXPECT_EQ(request.profiles_root, "/srv/profiles");
}

TEST(ProfilePackageCapTest, TheOneShotCapIsAReadableNumber) {
  const auto cap = ProfilePackagePayloadCap();

  // Derived from this machine's locked-memory allowance, and bounded so that a
  // container with almost none still exports and one with none does not try to
  // hold something enormous in memory.
  EXPECT_GE(cap, 16LL * 1024 * 1024);
  EXPECT_LE(cap, 256LL * 1024 * 1024);
}

}  // namespace GpgFrontend::Test
