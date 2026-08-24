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
#include <QDirIterator>
#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <chrono>
#include <future>
#include <thread>

#include "core/function/AESCryptoHelper.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/profile/MemoryAreaProfileAccessor.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileMember.h"
#include "core/profile/ProfileMigration.h"
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

/// Total bytes of everything under a tree, for comparing against what a
/// package would actually carry.
auto DirectorySizeOfTreeForTest(const QString &root) -> qint64 {
  qint64 total = 0;
  QDirIterator it(root, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    total += it.fileInfo().size();
  }
  return total;
}

/// The application key every export in this file carries.
auto TestRootKey() -> GFBuffer {
  return GFBuffer(QByteArray("0123456789abcdef0123456789abcdef"));
}

/// It as a member, which is the only way a key reaches a package now.
auto TestRootKeyMember() -> ProfileMember {
  ProfileMember member;
  member.path = "secure/app.key";
  member.area = ProfileArea::kSecure;
  member.bytes = TestRootKey();
  return member;
}

/// @param profiles_root unused: an export no longer records one. Kept in the
/// signature so the call sites still read as "this profile, in that folder".
/// What StageProfileTree() used to do, kept here because the version 1 package
/// built by hand below needs a staged tree and nothing in production stages one
/// any more -- packing reads the live profile.
auto StageTreeForLegacyPackage(const QString &profile_root,
                               const QString &staging_dir) -> bool {
  if (!QDir().mkpath(staging_dir + "/" + kProfilePackageTreePrefix)) {
    return false;
  }

  TreeMemberSource source(
      profile_root, false,
      QDir::cleanPath(QFileInfo(staging_dir).absoluteFilePath()));
  StagingMemberSink sink(staging_dir);
  return TransferProfileMembers(source, sink).isEmpty();
}

auto ExportRequestFor(const QString &root, const QString & /*profiles_root*/,
                      const QString &destination) -> ProfileExportRequest {
  ProfileExportRequest request;
  request.profile_root = root;
  request.dest_path = destination;
  request.secure_members = {TestRootKeyMember()};
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

// The whole reason the format carries two numbers instead of one. A writer that
// adds a field we do not need to understand raises format_version and leaves
// min_reader alone, and its packages have to keep opening here -- otherwise
// min_reader is dead weight and every future addition is a hard break.
TEST(ProfilePackageHeaderTest, AFormatFromTheFutureIsReadWhenItSaysWeCan) {
  ProfilePackageHeader header;
  header.format_version = kProfilePackageFormatVersion + 1;
  header.min_reader = kProfilePackageMinReader;
  header.writer = "9.9.9";

  const auto view =
      ParseProfilePackageHeader(EncodeProfilePackageHeader(header));

  ASSERT_TRUE(view.Ok());
  EXPECT_EQ(view.header.format_version, kProfilePackageFormatVersion + 1);
  EXPECT_EQ(view.header.min_reader, kProfilePackageMinReader);
}

// The one judgement the plaintext header is allowed to make on its own, and it
// is a refusal: it saves the key derivation on a file we could not use anyway.
TEST(ProfilePackageHeaderTest, AMinReaderFromTheFutureIsRefusedBeforeAnyKdf) {
  ProfilePackageHeader header;
  header.format_version = kProfilePackageFormatVersion + 1;
  header.min_reader = kProfilePackageFormatVersion + 1;
  header.writer = "9.9.9";

  const auto view =
      ParseProfilePackageHeader(EncodeProfilePackageHeader(header));

  EXPECT_EQ(view.status, ProfilePackageHeaderStatus::kTOO_NEW);
  // The version that wrote it is the only actionable thing in the message.
  EXPECT_TRUE(view.detail.contains("9.9.9"));
}

// The magic is what the freedesktop <magic> rule, and any future file(1) entry,
// match on. A format bump that moved it would break every file manager
// silently, so it breaks here first.
TEST(ProfilePackageHeaderTest, TheMagicIsWhatTheDesktopMatchesOn) {
  ProfilePackageHeader header;
  header.writer = "2.2.2";

  const auto bytes = EncodeProfilePackageHeader(header);

  ASSERT_GE(bytes.size(), kProfilePackageMagicLength);
  EXPECT_EQ(bytes.left(kProfilePackageMagicLength),
            QByteArray(kProfilePackageMagic, kProfilePackageMagicLength));
}

// These three spellings are duplicated outside the compiler's reach -- a
// freedesktop glob, a Windows registry value, a macOS plist tag -- so changing
// one has to fail here until the others follow.
TEST(ProfilePackageHeaderTest, TheRegisteredNamesAreWhatTheDesktopWasTold) {
  EXPECT_EQ(QString(kProfilePackageExtension), QString(".gfp"));
  EXPECT_EQ(QString(kProfilePackageMimeType),
            QString("application/x-gpgfrontend-profile"));
  EXPECT_EQ(QString(kProfilePackageUti),
            QString("com.bktus.gpgfrontend.profile"));
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

// Now that a package from a newer build can be opened here, importing one and
// exporting it again must not quietly destroy what that build depends on.
TEST(ProfilePackageManifestTest, FieldsFromANewerBuildSurviveARoundTrip) {
  const auto original = QByteArray(
      R"({"manifest_version":1,"schema_version":3,"display_name":"Work",)"
      R"("something_from_the_future":{"a":1},"another":"kept"})");

  const auto parsed = ParseProfilePackageManifest(original);
  ASSERT_TRUE(parsed.has_value());

  // Recognised fields land in the struct, not in the passthrough.
  EXPECT_EQ(parsed->schema_version, 3);
  EXPECT_EQ(parsed->display_name, "Work");
  EXPECT_FALSE(parsed->unknown_fields.contains("schema_version"));

  ASSERT_EQ(parsed->unknown_fields.size(), 2);
  EXPECT_EQ(parsed->unknown_fields.value("another").toString(), "kept");

  const auto again =
      ParseProfilePackageManifest(EncodeProfilePackageManifest(*parsed));
  ASSERT_TRUE(again.has_value());
  EXPECT_EQ(again->unknown_fields.value("another").toString(), "kept");
  EXPECT_EQ(again->unknown_fields.value("something_from_the_future")
                .toObject()
                .value("a")
                .toInt(),
            1);
  // ...and the fields this build does understand are still written from the
  // struct, not left to whatever the passthrough happened to carry.
  EXPECT_EQ(again->schema_version, 3);
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

// -------------------------------------------------------------- round trips

TEST(ProfilePackageRoundTripTest, AProtectedPackageComesBackByteForByte) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
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
      QDir(dir.path()).entryList({".gfp-*"}, QDir::Dirs | QDir::Hidden);
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

TEST(ProfilePackageRoundTripTest, RotatedKeysTravelEvenWhenTheAreaHasNoFiles) {
  // The regression this guards against is silent and unrecoverable: rotated
  // keys are what open data objects an earlier period sealed, and the secure
  // area is no longer walked on disk. If nothing carried them from the accessor
  // the package would come out looking perfectly fine and the objects would be
  // permanently unreadable on the other machine.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;

  // What the export sites read out of the session's accessor. Deliberately not
  // the same bytes as the file MakeProfile() left on disk, so that a copy taken
  // from the tree instead of from here would be visible rather than identical.
  ProfileMember rotated_member;
  rotated_member.path = "secure/DEADBEEF.key";
  rotated_member.area = ProfileArea::kSecure;
  rotated_member.bytes = GFBuffer(QByteArray("from-the-accessor"));
  request.secure_members.append(rotated_member);

  const auto written = ExportProfilePackage(request);
  ASSERT_TRUE(written.ok) << written.error.toStdString();

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  QFile rotated(extracted + "/profile/secure/DEADBEEF.key");
  ASSERT_TRUE(rotated.open(QIODevice::ReadOnly))
      << "the rotated key did not travel";
  EXPECT_EQ(rotated.readAll(), QByteArray("from-the-accessor"));

  // app.key still comes from the key in hand rather than from this map, since
  // the stored form may be sealed by this machine's credential store.
  QFile key(extracted + "/profile/secure/app.key");
  ASSERT_TRUE(key.open(QIODevice::ReadOnly));
  EXPECT_EQ(key.readAll(), QByteArray("0123456789abcdef0123456789abcdef"));
}

TEST(ProfilePackageRoundTripTest, AnEmptyDirectorySurvivesTheRoundTrip) {
  // The paths of file members create every directory that has files in it, so
  // an empty one was the single case with nothing to imply it and it silently
  // did not travel. GnuPG keeps several -- private-keys-v1.d and
  // openpgp-revocs.d are empty on a profile with no secret keys -- and the
  // profile that arrived was quietly not the profile that was sent.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);
  ASSERT_TRUE(QDir().mkpath(root + "/db/private-keys-v1.d"));

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  EXPECT_TRUE(QDir(extracted + "/profile/db/private-keys-v1.d").exists())
      << "an empty directory did not travel";
}

TEST(ProfilePackageRoundTripTest, AProtectedPackageIsNeverWrittenWithoutOne) {
  // The header decides how a reader frames the body, so a request that declares
  // kPIN and carries no passphrase used to produce a file whose header said
  // "protected" over a body that was plaintext -- the whole profile and its
  // application key in the clear, in a package the user was told was sealed.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer();

  const auto written = ExportProfilePackage(request);
  EXPECT_FALSE(written.ok);
  EXPECT_FALSE(QFileInfo::exists(package))
      << "a package declaring a passphrase it does not have was written anyway";
}

TEST(ProfilePackageRoundTripTest, AnEmptyPassphraseIsAWrongOneNotACorruptFile) {
  // Leaving the passphrase box blank is the commonest thing a user does wrong,
  // and it has to come back as "wrong passphrase". The framing used to be
  // inferred from whether a passphrase was supplied, so an empty one made the
  // reader treat a protected body as a plain one and hand ciphertext to the
  // archive walker, which reported a malformed package.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(QString("the-real-passphrase"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto read = ReadProfilePackage(package, dir.path() + "/extracted", {});
  EXPECT_EQ(read.status, ProfilePackageReadStatus::kBAD_PASSPHRASE);

  const auto peeked = PeekProfilePackageManifest(package, {});
  EXPECT_EQ(peeked.status, ProfilePackageReadStatus::kBAD_PASSPHRASE);
}

TEST(ProfilePackageRoundTripTest, TheSettingsSnapshotBeatsTheCopyOnDisk) {
  // The area table is explicit that config.ini is regenerated from the live
  // store rather than copied, and on a session write-back the snapshot is the
  // fresher of the two: settings changed during the session have not
  // necessarily reached the file yet. The tree walk offered the on-disk copy as
  // well, both went into the archive under one name, and the one that landed
  // last -- the stale one -- was what the recipient got.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  // Deliberately different from the snapshot below, so whichever copy travels
  // is identifiable rather than indistinguishable.
  WriteFile(root + "/config/config.ini", "[basic]\nlanguage=stale_ON_DISK\n");

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  request.settings.insert("basic/language", "fresh_IN_MEMORY");

  const auto written = ExportProfilePackage(request);
  ASSERT_TRUE(written.ok) << written.error.toStdString();

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  QFile config(extracted + "/profile/config/config.ini");
  ASSERT_TRUE(config.open(QIODevice::ReadOnly));
  const auto contents = QString::fromUtf8(config.readAll());
  EXPECT_TRUE(contents.contains("fresh_IN_MEMORY"))
      << "the settings snapshot did not travel; got: "
      << contents.toStdString();
  EXPECT_FALSE(contents.contains("stale_ON_DISK"))
      << "the on-disk copy overwrote the snapshot";
}

TEST(ProfilePackageRoundTripTest,
     AHandPlacedKeyDatabaseIsNotNamedInTheManifest) {
  // Dropping the directory but keeping the reference would make the package
  // claim contents it does not have -- and the recipient creates a missing key
  // database directory rather than complaining, so they would find a keyring
  // with no keys and no error anywhere.
  QContainer<KeyDatabaseItemSO> databases;

  KeyDatabaseItemSO managed;
  managed.name = "DEFAULT";
  managed.path = "/profiles/work/db";
  databases.push_back(managed);

  KeyDatabaseItemSO sandboxed;
  sandboxed.name = "Work";
  sandboxed.path = "/profiles/work/dbs/work";
  databases.push_back(sandboxed);

  KeyDatabaseItemSO by_hand;
  by_hand.name = "Hand Placed";
  by_hand.path = "/profiles/work/work-keys";
  databases.push_back(by_hand);

  KeyDatabaseItemSO outside;
  outside.name = "Elsewhere";
  outside.path = "/home/someone/keys";
  databases.push_back(outside);

  const auto packed =
      RewriteKeyDatabaseListForPacking(databases, "/profiles/work");

  QStringList names;
  for (const auto &item : packed) names << item.name;

  EXPECT_TRUE(names.contains("DEFAULT"));
  EXPECT_TRUE(names.contains("Work"));
  EXPECT_FALSE(names.contains("Hand Placed"))
      << "a hand-placed key database was carried into the package";

  // The one outside the profile keeps its absolute path and is marked external,
  // which is the case this has always handled.
  EXPECT_TRUE(names.contains("Elsewhere"));

  const auto entries = DescribeKeyDatabasesForManifest(packed);
  for (const auto &entry : entries) {
    if (entry.name == "Elsewhere") {
      EXPECT_TRUE(entry.external);
      continue;
    }
    EXPECT_FALSE(entry.external);
    EXPECT_TRUE(entry.stored_path.startsWith("@profile"))
        << "an absolute path from this machine reached the manifest: "
        << entry.stored_path.toStdString();
  }
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
  const auto package = dir.path() + "/root.gfp";
  auto request = ExportRequestFor(root, profiles_root, package);
  request.protection = ProfilePackageProtection::kNONE;

  const auto written = ExportProfilePackage(request);
  ASSERT_TRUE(written.ok) << written.error.toStdString();

  const auto leftovers =
      QDir(profiles_root).entryList({".gfp-*"}, QDir::Dirs | QDir::Hidden);
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

  const auto package = dir.path() + "/open.gfp";
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

  const auto package = dir.path() + "/work.gfp";
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

  const auto package = dir.path() + "/open.gfp";
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

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.passphrase = GFBuffer(QString("pass"));
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

TEST(ProfileExportTest, AFailureBeforeTheFirstByteStillReturns) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  // Larger than the exchanger's ring and genuinely incompressible, so the
  // archive producer really does block waiting for room. Compressible bulk
  // would fit in the pipe, the producer would finish on its own, and the test
  // would pass without touching what it is here for.
  {
    QByteArray bulk(8 * 1024 * 1024, Qt::Uninitialized);
    quint64 state = 0x2545F4914F6CDD1DULL;
    for (qsizetype i = 0; i < bulk.size(); ++i) {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      bulk[i] = static_cast<char>(state & 0xFF);
    }
    QFile file(root + "/db/bulk.kbx");
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(bulk), bulk.size());
  }

  // The temporary is made beside the destination, so a destination whose folder
  // is not there fails at the first open -- before a single byte is drained,
  // while the producer is still blocked on a full pipe. Joining it there is a
  // deadlock, and the export never came back.
  auto request =
      ExportRequestFor(root, dir.path(), dir.path() + "/nowhere/work.gfp");

  std::promise<bool> finished;
  auto result = finished.get_future();
  std::thread runner([request, &finished]() mutable {
    finished.set_value(ExportProfilePackage(request).ok);
  });

  if (result.wait_for(std::chrono::seconds(20)) != std::future_status::ready) {
    // Detached rather than joined: the point of the failure is that the thread
    // is never coming back. It captured everything it touches by value, so
    // leaving it parked is safe.
    runner.detach();
    FAIL() << "the export never returned -- the archive producer was joined "
              "while it was blocked writing into a full pipe";
  }

  runner.join();
  EXPECT_FALSE(result.get());
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

  const auto package = dir.path() + "/work.gfp";
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

  const auto package = dir.path() + "/work.gfp";
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

TEST(ProfileAdoptionTest, TheCopyBranchProducesTheSameProfile) {
  // An import genuinely crosses a filesystem boundary once staging lives in
  // protected storage, and QDir::rename cannot. There is no portable way to
  // make a rename fail with EXDEV on demand, so the mover is a seam and this
  // forces the branch that only fires on a real crossing.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);
  WriteFile(root + "/db/pubring.kbx", "keys");

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  int renames = 0;
  const ProfileTreeMover copy_only = [&renames](const QString &source,
                                                const QString &destination) {
    ++renames;
    // Never the rename, so the fallback carries the whole import.
    if (QFileInfo(source).isDir()) {
      if (!QDir().mkpath(destination)) return false;
      QDir from(source);
      for (const auto &entry :
           from.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot |
                              QDir::Hidden)) {
        QFile::copy(entry.absoluteFilePath(),
                    destination + "/" + entry.fileName());
      }
      return true;
    }
    return QFile::copy(source, destination);
  };

  const auto imported = dir.path() + "/profiles/copy";
  EXPECT_TRUE(AdoptExtractedProfile(extracted, imported, "copy", "Copy",
                                    read.manifest, copy_only)
                  .isEmpty());

  EXPECT_GT(renames, 0);

  const auto marker = ReadProfileMarker(ProfileMarkerPathFor(imported));
  ASSERT_TRUE(marker.has_value());
  EXPECT_EQ(marker->profile_id, "copy");

  QFile keys(imported + "/db/pubring.kbx");
  ASSERT_TRUE(keys.open(QIODevice::ReadOnly));
  EXPECT_EQ(keys.readAll(), QByteArray("keys"));
}

TEST(ProfileAdoptionTest, AFailedMoveLeavesNoHalfProfile) {
  // Half an import would be adopted as a whole profile: a marker and a config
  // with the keyring missing looks like a profile whose keys were deleted.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto extracted = dir.path() + "/extracted";
  ASSERT_TRUE(ReadProfilePackage(package, extracted, {}).Ok());

  const ProfileTreeMover always_fails = [](const QString &, const QString &) {
    return false;
  };

  const auto imported = dir.path() + "/profiles/copy";
  EXPECT_FALSE(AdoptExtractedProfile(extracted, imported, "copy", "Copy", {},
                                     always_fails)
                   .isEmpty());

  EXPECT_FALSE(QFileInfo::exists(ProfileMarkerPathFor(imported)));
}

TEST(ProfileMoveTest, ARenameAcrossNothingStillMoves) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  WriteFile(dir.path() + "/from/a.txt", "hello");

  ASSERT_TRUE(
      MoveTreeAcrossFilesystems(dir.path() + "/from", dir.path() + "/to"));

  EXPECT_FALSE(QFileInfo::exists(dir.path() + "/from"));

  QFile moved(dir.path() + "/to/a.txt");
  ASSERT_TRUE(moved.open(QIODevice::ReadOnly));
  EXPECT_EQ(moved.readAll(), QByteArray("hello"));
}

TEST(ProfileMoveTest, MovingOntoSomethingThatExistsFailsWithoutDestroyingIt) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  WriteFile(dir.path() + "/from/a.txt", "new");
  WriteFile(dir.path() + "/to/a.txt", "old");

  EXPECT_FALSE(MoveTreeAcrossFilesystems(dir.path() + "/from/a.txt",
                                         dir.path() + "/to/a.txt"));

  // And the thing already there is untouched. The cleanup after a failed copy
  // removes the destination, so refusing has to happen before anything is
  // attempted — otherwise this deletes the profile the caller was protecting.
  QFile kept(dir.path() + "/to/a.txt");
  ASSERT_TRUE(kept.open(QIODevice::ReadOnly));
  EXPECT_EQ(kept.readAll(), QByteArray("old"));
}

// -------------------------------------------------------- temporary sessions

TEST(ProfileSessionRootTest, IsDerivedFromThePackageAndIsTransient) {
  const auto a = ProfileSessionRoot("/srv/profiles", "/tmp/work.gfp");
  const auto b = ProfileSessionRoot("/srv/profiles", "/tmp/other.gfp");

  // Derived rather than minted, so two windows work out the same directory for
  // the same file and the second one can be told it is already open.
  EXPECT_EQ(a, ProfileSessionRoot("/srv/profiles", "/tmp/work.gfp"));
  EXPECT_NE(a, b);

  // Dot-prefixed: transient, and never adopted by the profile scan.
  EXPECT_TRUE(a.startsWith("/srv/profiles/."));
  EXPECT_EQ(QFileInfo(a).fileName().size(), 33);  // the dot and 32 hex

  EXPECT_TRUE(ProfileSessionRoot({}, "/tmp/work.gfp").isEmpty());
  EXPECT_TRUE(ProfileSessionRoot("/srv/profiles", {}).isEmpty());
}

TEST(PackagedProfileTest, OpensAPackageAndWritesItBackToTheSameFile) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.passphrase = GFBuffer(QString("pass"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  PackagedProfile packaged(package, dir.path());
  EXPECT_EQ(packaged.Inspect().status, ProfileMountStatus::kNEEDS_PASSPHRASE);

  // Before storage exists there is no root, only a lock to take. Anything that
  // assumed the two were the same thing should fail here rather than quietly
  // read the anchor.
  EXPECT_TRUE(packaged.Root().isEmpty());
  EXPECT_EQ(packaged.LockRoot(), ProfileSessionRoot(dir.path(), package));

  const auto mounted = packaged.Mount({request.passphrase, 3});
  ASSERT_TRUE(mounted.Ok()) << mounted.detail.toStdString();
  ASSERT_FALSE(packaged.Root().isEmpty());

  // The whole point: the tree is not in the profiles folder. Where it *is*
  // depends on what this machine offers, so what is asserted is that it went
  // somewhere the driver chose and said so.
  EXPECT_FALSE(packaged.Root().startsWith(dir.path() + "/."));
  std::cerr << "  session storage: " << packaged.Root().toStdString()
            << std::endl;
  EXPECT_EQ(packaged.Manifest().display_name, "Work");

  // The session is a profile root like any other: the tree is at the top, not
  // under `profile/`, and the extraction scratch did not outlive the call.
  EXPECT_TRUE(QFileInfo::exists(packaged.Root() + "/data_objs/abcd"));
  EXPECT_TRUE(QDir(packaged.Root() + "/.scratch")
                  .entryList({".gfp-*"}, QDir::Dirs | QDir::Hidden)
                  .isEmpty());

  // The key is not on the storage at all. A package carries it unprotected, so
  // unpacking it to a file and rewriting that file in place would leave the
  // plaintext recoverable -- ToFileAtomic unlinks, it does not overwrite.
  EXPECT_FALSE(QFileInfo::exists(packaged.Root() + "/secure/app.key"));
  EXPECT_FALSE(QDir(packaged.Root() + "/secure").exists())
      << "the secure area was written to the storage after all";

  auto storage = packaged.MakeAccessor();
  ASSERT_FALSE(storage.isNull());
  EXPECT_TRUE(storage->IsAreaResident(ProfileArea::kSecure));
  EXPECT_TRUE(storage->PathOf(ProfileArea::kSecure, "app.key").isEmpty());

  // One secret, and it protects the stored form here too: inside the session
  // the package's passphrase is still the only secret in play.
  const auto extracted = storage->Read(ProfileArea::kSecure, "app.key");
  ASSERT_TRUE(extracted.has_value());
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(*extracted));

  const auto opened_key =
      ProfileSecureKeyManager::UnsealKey(request.passphrase, {}, *extracted);
  ASSERT_TRUE(opened_key.has_value());
  EXPECT_EQ(*opened_key, TestRootKey());

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
  back.secure_members = {TestRootKeyMember()};
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

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.passphrase = GFBuffer(QString("pass"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  PackagedProfile packaged(package, dir.path());
  const auto mounted = packaged.Mount({GFBuffer(QString("wrong")), 3});
  EXPECT_EQ(mounted.status, ProfileMountStatus::kBAD_PASSPHRASE);

  // Nothing was extracted, so there is nothing holding an unprotected key —
  // neither where the tree would have gone nor in the profiles folder.
  const auto claimed = packaged.Root();
  packaged.DiscardSessionStorage();
  if (!claimed.isEmpty()) EXPECT_FALSE(QFileInfo::exists(claimed));
  EXPECT_FALSE(QFileInfo::exists(ProfileSessionRoot(dir.path(), package)));
  EXPECT_TRUE(QDir(dir.path())
                  .entryList({".gfp-*"}, QDir::Dirs | QDir::Hidden)
                  .isEmpty());
}

TEST(PackagedProfileTest, ALayoutFromTheFutureIsRefusedBeforeAdoption) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  request.manifest.min_reader_version = 99;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  PackagedProfile packaged(package, dir.path());
  const auto mounted = packaged.Mount({{}, 3});
  EXPECT_EQ(mounted.status, ProfileMountStatus::kTOO_NEW);

  // Refused before anything was adopted: a package this build must not touch
  // leaves no trace of having been opened, wherever the storage was claimed.
  const auto claimed = packaged.Root();
  packaged.DiscardSessionStorage();
  if (!claimed.isEmpty()) EXPECT_FALSE(QFileInfo::exists(claimed));
  EXPECT_FALSE(QFileInfo::exists(ProfileSessionRoot(dir.path(), package)));
}

// The mirror of the test above, and the case the format actually exists for: a
// package from an older build opens here. Refusing this one would make every
// exported profile a hostage to the version that wrote it.
TEST(PackagedProfileTest, ALayoutFromThePastIsMountedNotRefused) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/old.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  request.manifest.schema_version = kOldestSupportedProfileSchema;
  request.manifest.min_reader_version = kOldestSupportedProfileSchema;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto before = [&package] {
    QFile file(package);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
  }();
  ASSERT_FALSE(before.isEmpty());

  PackagedProfile packaged(package, dir.path());
  EXPECT_EQ(packaged.Inspect().status, ProfileMountStatus::kOK);

  const auto mounted = packaged.Mount({{}, 3});
  ASSERT_TRUE(mounted.Ok()) << mounted.detail.toStdString();

  // Opening a package never rewrites it. It is extracted into a session root
  // and the migration ladder runs on *that*; the file on disk is frequently
  // the user's only backup. Re-exporting is what upgrades a package -- "open
  // it and it upgrades" is the natural assumption and it is wrong.
  const auto after = [&package] {
    QFile file(package);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
  }();
  EXPECT_EQ(after, before);
}

TEST(PackagedProfileTest, AStaleSessionFromACrashedProcessIsReplaced) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  // What a process that died mid-session leaves behind. Where that is depends
  // on which storage was chosen, and the storage root is derived from the
  // package the same way the lock is — so a second process finds the first
  // one's leftovers without being told where to look, which is the whole point.
  //
  // Kept alive rather than destroyed, because the driver destroys its tree in
  // its own destructor — which is exactly right for a process that exits and
  // exactly wrong for one that dies. Holding it is how the tree stays put.
  PackagedProfile crashed(package, dir.path());
  ASSERT_TRUE(crashed.Mount({{}, 3}).Ok());
  WriteFile(crashed.Root() + "/data_objs/abcd", "from a process that is gone");
  // No Unmount(): that is what dying mid-session means.

  PackagedProfile packaged(package, dir.path());
  // An unprotected package needs no secret, and says so.
  EXPECT_EQ(packaged.Inspect().status, ProfileMountStatus::kOK);

  const auto mounted = packaged.Mount({{}, 3});
  ASSERT_TRUE(mounted.Ok()) << mounted.detail.toStdString();

  // Nothing protected the package, so there is no secret to protect the key
  // with either. Inventing one would be a second thing to forget.
  //
  // It is still not on the storage: an unprotected package's key is plaintext,
  // which is all the more reason for it not to become a file here.
  EXPECT_FALSE(QFileInfo::exists(packaged.Root() + "/secure/app.key"));

  auto storage = packaged.MakeAccessor();
  ASSERT_FALSE(storage.isNull());
  const auto extracted = storage->Read(ProfileArea::kSecure, "app.key");
  ASSERT_TRUE(extracted.has_value());
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(*extracted));

  QSettings session_settings(packaged.Root() + "/config/config.ini",
                             QSettings::IniFormat);
  EXPECT_EQ(session_settings.value("advanced/app_key_protection").toString(),
            QString("none"));

  // Replaced, not merged: what the dead process left is another machine's key
  // material with no owner, and adopting half of it would be worse than either.
  QFile object(packaged.Root() + "/data_objs/abcd");
  ASSERT_TRUE(object.open(QIODevice::ReadOnly));
  EXPECT_EQ(object.readAll(), QByteArray("encrypted-object"));

  packaged.Unmount(ProfileUnmountMode::kNORMAL);
}

TEST(ProfileSweepTest, OnlyTransientRootsNobodyIsUsingAreRemoved) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto stale = dir.path() + "/.aaaa";
  const auto keep = dir.path() + "/.bbbb";
  const auto scratch = dir.path() + "/.gfp-staging-cccc";
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

TEST(ProfileSweepTest, AnAnchorLeadsTheSweepToStrandedStorage) {
  // The case the pointer exists for. The anchor is in the profiles folder and
  // the storage is not — it may be in memory, or an encrypted volume, or a
  // temporary directory — so a process that dies leaves a tree full of somebody
  // else's key material somewhere nothing else would think to look.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto anchor = dir.path() + "/.abcd";
  const auto stranded = dir.path() + "/elsewhere/gf-abcd";

  WriteFile(stranded + "/secure/app.key", "another machine's key");

  QJsonObject state;
  state["driver"] = "fs-tmpfs";
  state["root"] = stranded;
  ASSERT_TRUE(WriteSessionPointer(anchor, state));

  EXPECT_EQ(SweepTransientProfileRoots(dir.path(), {}), 1);

  EXPECT_FALSE(QFileInfo::exists(stranded));
  EXPECT_FALSE(QFileInfo::exists(anchor));
}

TEST(ProfileSweepTest, AnAnchorSomebodyHoldsIsLeftEntirelyAlone) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto anchor = dir.path() + "/.abcd";
  const auto live = dir.path() + "/elsewhere/gf-abcd";

  WriteFile(live + "/secure/app.key", "a running session's key");

  QJsonObject state;
  state["driver"] = "fs-tmpfs";
  state["root"] = live;
  ASSERT_TRUE(WriteSessionPointer(anchor, state));

  // Named as the one to keep, exactly as the loader names its own.
  EXPECT_EQ(SweepTransientProfileRoots(dir.path(), anchor), 0);

  EXPECT_TRUE(QFileInfo::exists(live));
  EXPECT_TRUE(QFileInfo::exists(anchor));
}

TEST(ProfileSweepTest, APointerIsNeverFollowedSomewhereItShouldNotGo) {
  // The pointer is read off disk and a corrupted or malicious one must not be
  // able to talk the sweep into deleting an arbitrary tree.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto precious = dir.path() + "/precious";
  WriteFile(precious + "/keys", "not a session");

  const auto anchor = dir.path() + "/.abcd";

  QJsonObject relative;
  relative["driver"] = "fs-tmpfs";
  relative["root"] = "precious";  // not absolute
  ASSERT_TRUE(WriteSessionPointer(anchor, relative));

  EXPECT_EQ(SweepTransientProfileRoots(dir.path(), {}), 1);

  // The anchor goes; the thing it pointed at does not.
  EXPECT_FALSE(QFileInfo::exists(anchor));
  EXPECT_TRUE(QFileInfo::exists(precious + "/keys"));
}

TEST(ProfileSweepTest, ALegacySessionRootIsStillCollected) {
  // A package opened by an older build, or by one running the `disk` policy,
  // is the tree itself with a marker in it and no pointer at all.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto legacy = dir.path() + "/.aaaa";
  WriteFile(legacy + "/profile.json", R"({"schema_version":3})");

  EXPECT_EQ(SweepTransientProfileRoots(dir.path(), {}), 1);
  EXPECT_FALSE(QFileInfo::exists(legacy));
}

TEST(PackagedProfileTest, CarriesItsOwnSecretAndWritesBackWhereItCameFrom) {
  // The session state used to be a process-global struct beside the packing
  // code. It is now the profile object itself, which is what makes "one
  // session per process" a consequence of the design rather than a convention.
  PackagedProfile packaged("/tmp/work.gfp", "/srv/profiles");

  EXPECT_EQ(packaged.PackagePath(), "/tmp/work.gfp");
  EXPECT_EQ(packaged.ProfilesRoot(), "/srv/profiles");
  EXPECT_TRUE(packaged.IsTransient());

  // The lock is where it always was, and is known before anything is opened.
  EXPECT_EQ(packaged.LockRoot(),
            ProfileSessionRoot("/srv/profiles", "/tmp/work.gfp"));

  // The root is not, because nothing has decided where the tree goes yet.
  EXPECT_TRUE(packaged.Root().isEmpty());

  // A write-back aims at the file this session came from, never at a new one.
  const auto request = packaged.WriteBackRequest();
  EXPECT_EQ(request.dest_path, "/tmp/work.gfp");
  EXPECT_EQ(request.profile_root, packaged.Root());
}

TEST(ProfilePackageCapTest, TheOneShotCapIsAReadableNumber) {
  const auto cap = ProfilePackagePayloadCap();

  // Derived from this machine's locked-memory allowance, and bounded so that a
  // container with almost none still exports and one with none does not try to
  // hold something enormous in memory.
  EXPECT_GE(cap, 16LL * 1024 * 1024);
  EXPECT_LE(cap, 256LL * 1024 * 1024);
}

TEST(ProfilePackageDeclaredSizeTest, APackageRecordsWhatItWillUnpackTo) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  request.include_workspace = true;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  ASSERT_GT(read.manifest.uncompressed_bytes, 0);

  // The number has one job: be enough. Measure what actually landed and check
  // the declaration covers it -- a figure that under-counts is worse than none,
  // because the budget built from it would be confidently too small.
  qint64 actual = 0;
  QDirIterator it(extracted + "/profile",
                  QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    actual += it.fileInfo().size();
  }

  ASSERT_GT(actual, 0);
  EXPECT_GE(read.manifest.uncompressed_bytes, actual)
      << "the package under-declared its own size";
}

TEST(ProfilePackageDeclaredSizeTest, TheManifestIsReadableWithoutUnpacking) {
  // What lets a session be sized before it is provisioned. Storage has to exist
  // before there is anywhere to unpack into, so a size learned after unpacking
  // would arrive too late to be of any use.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(QString("a passphrase"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto peeked = PeekProfilePackageManifest(package, request.passphrase);
  ASSERT_TRUE(peeked.Ok()) << peeked.detail.toStdString();
  EXPECT_EQ(peeked.manifest.display_name, "Work");
  EXPECT_GT(peeked.manifest.uncompressed_bytes, 0);

  // And nothing was written anywhere on the way to finding out.
  EXPECT_FALSE(QDir(dir.path() + "/extracted").exists());
}

TEST(ProfilePackageDeclaredSizeTest, PeekingNeedsTheRightPassphrase) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(QString("right"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto peeked =
      PeekProfilePackageManifest(package, GFBuffer(QString("wrong")));
  EXPECT_FALSE(peeked.Ok());
}

TEST(ProfilePackageDeclaredSizeTest,
     ALegacyPackageDeclaresNothingAndStillOpens) {
  // Version 1 never carried the field. Peeking one would cost a full pass to
  // learn nothing, so it is not attempted -- and zero is exactly what the
  // budget's heuristic already handles.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  ProfilePackageHeader header;
  header.format_version = 1;
  header.min_reader = 1;
  header.writer = "2.1.0";
  header.created = "2024-01-01T00:00:00Z";
  header.protection = ProfilePackageProtection::kNONE;

  const auto path = dir.path() + "/legacy.gfp";
  {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    const auto bytes = EncodeProfilePackageHeader(header);
    ASSERT_EQ(file.write(bytes), bytes.size());
  }

  const auto peeked = PeekProfilePackageManifest(path, {});
  ASSERT_TRUE(peeked.Ok()) << peeked.detail.toStdString();
  EXPECT_EQ(peeked.manifest.uncompressed_bytes, 0);
}

TEST(ProfilePackageDeclaredSizeTest, AnUnknownFieldStillSurvivesARoundTrip) {
  // The field joined the known set, so the forward-compatibility path has to
  // keep working alongside it.
  ProfilePackageManifest manifest;
  manifest.uncompressed_bytes = 4096;
  manifest.unknown_fields = QJsonObject{{"from_the_future", 7}};

  const auto parsed =
      ParseProfilePackageManifest(EncodeProfilePackageManifest(manifest));
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->uncompressed_bytes, 4096);
  EXPECT_EQ(parsed->unknown_fields.value("from_the_future").toInt(), 7);
}

TEST(ProfilePackageStagingFreeTest, AnExportWritesNoPlaintextCopyAnywhere) {
  // Until now an export built a full plaintext copy of the profile first, with
  // an *unprotected* application key in it, and packed that. It was the last
  // place the key reached a disk, and it meant a session needed twice its own
  // size in free space to save itself.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto watched = dir.path() + "/watched";
  ASSERT_TRUE(QDir().mkpath(watched));

  const auto package = watched + "/work.gfp";
  auto request = ExportRequestFor(root, watched, package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(QString("a passphrase"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  // Nothing beside the package: no scratch directory, and so no copy of the
  // key to have to erase afterwards.
  auto leftovers = QDir(watched).entryList(QDir::Files | QDir::Dirs |
                                           QDir::NoDotAndDotDot | QDir::Hidden);
  leftovers.removeAll("work.gfp");
  EXPECT_TRUE(leftovers.isEmpty())
      << "export left something behind: " << leftovers.join(", ").toStdString();

  // And the profile it read from is untouched.
  EXPECT_FALSE(QFileInfo::exists(root + "/.gfp-staging"));
}

TEST(ProfilePackageStagingFreeTest, TheManifestIsTheFirstMemberOfTheArchive) {
  // A reader has to know what a package is before it can decide where to put
  // it, and with a streamed body that answer should arrive in the first chunk
  // rather than after the whole file has been unpacked.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  // Order, not presence. Every entry is claimed so that nothing is written and
  // each one is announced to the sink in the order the archive holds it, which
  // is the only way to see the property PeekProfilePackageManifest() rests on.
  QFile file(package);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  // Comfortably more than a header can be; the parser reads what it needs and
  // reports where the body starts.
  const auto view = ParseProfilePackageHeader(file.read(64 * 1024));
  ASSERT_TRUE(view.Ok());
  ASSERT_TRUE(file.seek(view.body_offset));

  auto exchanger = CreateStandardGFDataExchanger();
  std::thread feeder([&]() {
    QByteArray chunk(64 * 1024, Qt::Uninitialized);
    while (true) {
      const auto read = file.read(chunk.data(), chunk.size());
      if (read <= 0) break;
      exchanger->Write(reinterpret_cast<const std::byte *>(chunk.constData()),
                       read);
    }
    exchanger->CloseWrite();
  });

  QStringList order;
  QTemporaryDir nowhere;
  ASSERT_TRUE(nowhere.isValid());
  ArchiveFileOperator::ExtractArchiveFromDataExchangerSync(
      exchanger, nowhere.path(), ArchiveExtractPolicy::Strict(),
      [](const QString &) { return true; },
      [&order](const QString &path, const GFBuffer &) {
        order << path;
        return true;
      });
  exchanger->CloseWrite();
  feeder.join();

  ASSERT_FALSE(order.isEmpty());
  EXPECT_EQ(order.first(), QString("manifest.json"))
      << "the manifest was not the first member; got: "
      << order.join(", ").toStdString();
}

TEST(ProfilePackageStagingFreeTest, AnExportReportsWhatItLeftBehind) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  QFile stray(root + "/notes.txt");
  ASSERT_TRUE(stray.open(QIODevice::WriteOnly));
  stray.write("private");
  stray.close();

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;

  const auto written = ExportProfilePackage(request);
  ASSERT_TRUE(written.ok) << written.error.toStdString();
  EXPECT_TRUE(written.skipped.contains("notes.txt"))
      << "the sender was not told their file stayed home";

  const auto extracted = dir.path() + "/extracted";
  ASSERT_TRUE(ReadProfilePackage(package, extracted, {}).Ok());
  EXPECT_FALSE(QFileInfo::exists(extracted + "/profile/notes.txt"));
}

TEST(ProfilePackageStreamingTest, APackageLargerThanTheOldCapRoundTrips) {
  // The property the streamed body exists for. Version 1 held the payload and
  // its ciphertext in memory at once and refused anything past
  // ProfilePackagePayloadCap(); this is comfortably past what that allowed on a
  // machine with a small locked-memory allowance, and it must simply work.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  // Genuinely incompressible, so the package on disk really is this big rather
  // than gzip'd back under the limit -- which is what would make this test pass
  // without proving anything.
  {
    QByteArray bulk(24 * 1024 * 1024, Qt::Uninitialized);
    quint64 state = 0x9E3779B97F4A7C15ULL;
    for (qsizetype i = 0; i < bulk.size(); ++i) {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      bulk[i] = static_cast<char>(state & 0xFF);
    }
    QDir().mkpath(root + "/workspace");
    QFile file(root + "/workspace/bulk.bin");
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(bulk), bulk.size());
  }

  const auto package = dir.path() + "/big.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(QString("a long enough passphrase"));
  request.include_workspace = true;

  const auto written = ExportProfilePackage(request);
  ASSERT_TRUE(written.ok) << written.error.toStdString();

  // The point of the test: the file really is past what a version 1 body was
  // allowed to be on this machine, so a cap left applying to streamed packages
  // would refuse it here.
  ASSERT_GT(QFileInfo(package).size(), ProfilePackagePayloadCap())
      << "the package compressed under the old ceiling; the test proves "
         "nothing";

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, request.passphrase);
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();

  EXPECT_EQ(QFileInfo(extracted + "/profile/workspace/bulk.bin").size(),
            24 * 1024 * 1024);
}

TEST(ProfilePackageStreamingTest,
     ADamagedBodyIsRefusedRatherThanPartlyAdopted) {
  // Streaming writes members as they arrive, so the invariant that a package is
  // adopted only after a *complete* extract is what keeps a corrupt file from
  // leaving half a profile behind.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kPIN;
  request.passphrase = GFBuffer(QString("pass"));
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  // Flip a byte deep in the body, past the header and the stream preamble.
  {
    QFile file(package);
    ASSERT_TRUE(file.open(QIODevice::ReadWrite));
    const auto at = file.size() - 32;
    ASSERT_TRUE(file.seek(at));
    char byte = 0;
    ASSERT_EQ(file.read(&byte, 1), 1);
    ASSERT_TRUE(file.seek(at));
    byte = static_cast<char>(byte ^ 0x55);
    ASSERT_EQ(file.write(&byte, 1), 1);
  }

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, request.passphrase);
  EXPECT_FALSE(read.Ok());
  EXPECT_FALSE(QDir(extracted).exists())
      << "a damaged package left a partial tree behind";
}

TEST(ProfilePackageLegacyTest, AVersionOnePackageStillOpens) {
  // The writer only emits the streamed format now, so nothing else in this
  // suite exercises the one-shot body -- and packages in that shape already
  // exist on people's disks. Built by hand here for exactly that reason.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto staging = dir.path() + "/staging";
  ASSERT_TRUE(StageTreeForLegacyPackage(root, staging));

  // A version 1 package carried the key inside the tree; nothing stages it
  // there now, so it is placed by hand.
  WriteFile(staging + "/profile/secure/app.key", "0123456789abcdef");

  // The manifest a version 1 reader expects to find beside the tree.
  ProfilePackageManifest manifest;
  manifest.format_version = 1;
  manifest.min_reader = 1;
  manifest.schema_version = 3;
  manifest.min_reader_version = 2;
  manifest.app_profile = "GpgFrontend";
  manifest.display_name = "Legacy";
  manifest.protection = "none";
  manifest.app_key_protection = "none";
  manifest.package_id = "legacypackageid";
  {
    QFile file(staging + "/manifest.json");
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(EncodeProfilePackageManifest(manifest));
  }

  // gzip'd tar of the staging tree, exactly as version 1 wrote it.
  auto exchanger = CreateStandardGFDataExchanger();
  GFError archive_error = 0;
  std::thread producer([&]() {
    archive_error = ArchiveFileOperator::NewArchive2DataExchangerSync(
        staging, exchanger, ArchiveCompression::kGZIP);
  });

  QByteArray payload;
  QByteArray chunk(64 * 1024, Qt::Uninitialized);
  while (true) {
    const auto read = exchanger->Read(
        reinterpret_cast<std::byte *>(chunk.data()), chunk.size());
    if (read <= 0) break;
    payload.append(chunk.constData(), static_cast<qsizetype>(read));
  }
  producer.join();
  ASSERT_GE(archive_error, 0);
  ASSERT_FALSE(payload.isEmpty());

  ProfilePackageHeader header;
  header.format_version = 1;
  header.min_reader = 1;
  header.writer = "2.1.0";
  header.writer_stable = true;
  header.created = "2024-01-01T00:00:00Z";
  header.protection = ProfilePackageProtection::kNONE;

  // The digest binds the header to the manifest, so it has to be recomputed
  // for the header actually written.
  manifest.header_digest =
      ProfilePackageHeaderDigest(EncodeProfilePackageHeader(header));
  {
    QFile file(staging + "/manifest.json");
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(EncodeProfilePackageManifest(manifest));
  }

  // Repack now that the manifest is final.
  auto exchanger2 = CreateStandardGFDataExchanger();
  GFError archive_error2 = 0;
  std::thread producer2([&]() {
    archive_error2 = ArchiveFileOperator::NewArchive2DataExchangerSync(
        staging, exchanger2, ArchiveCompression::kGZIP);
  });
  payload.clear();
  while (true) {
    const auto read = exchanger2->Read(
        reinterpret_cast<std::byte *>(chunk.data()), chunk.size());
    if (read <= 0) break;
    payload.append(chunk.constData(), static_cast<qsizetype>(read));
  }
  producer2.join();
  ASSERT_GE(archive_error2, 0);

  const auto package = dir.path() + "/legacy.gfp";
  {
    QFile file(package);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    const auto header_bytes = EncodeProfilePackageHeader(header);
    ASSERT_EQ(file.write(header_bytes), header_bytes.size());
    ASSERT_EQ(file.write(payload), payload.size());
  }

  const auto extracted = dir.path() + "/extracted";
  const auto read = ReadProfilePackage(package, extracted, {});
  ASSERT_TRUE(read.Ok()) << read.detail.toStdString();
  EXPECT_EQ(read.manifest.display_name, "Legacy");
  EXPECT_EQ(read.header.format_version, 1);

  QFile object(extracted + "/profile/data_objs/abcd");
  ASSERT_TRUE(object.open(QIODevice::ReadOnly));
  EXPECT_EQ(object.readAll(), QByteArray("encrypted-object"));
}

TEST(ProfilePackageLegacyTest, ThisBuildWritesTheStreamedFormat) {
  // The other half of the compatibility story: what goes out is version 2, and
  // min_reader says so, so an older build refuses cleanly rather than reporting
  // a package it cannot parse as damaged.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/work";
  MakeProfile(root);

  const auto package = dir.path() + "/work.gfp";
  auto request = ExportRequestFor(root, dir.path(), package);
  request.protection = ProfilePackageProtection::kNONE;
  ASSERT_TRUE(ExportProfilePackage(request).ok);

  const auto inspected = InspectProfilePackage(package);
  ASSERT_TRUE(inspected.Ok()) << inspected.detail.toStdString();
  EXPECT_GE(inspected.header.format_version, kProfilePackageStreamedFrom);
  EXPECT_GE(inspected.header.min_reader, kProfilePackageStreamedFrom);
}

TEST(ProfilePackageOversizeTest, AHugeLegacyFileIsRefusedBeforeItIsRead) {
  // Opening a version 1 body holds it whole and then the plaintext beside it,
  // so a file several times larger than memory used to take the process down
  // part way through mounting, with the profile lock already held.
  //
  // A sparse file makes this cheap, and never reading it is itself the
  // assertion: reading would show up as an out-of-memory kill rather than a
  // returned status.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.path() + "/huge.gfp";
  const auto cap = ProfilePackagePayloadCap();

  ProfilePackageHeader header;
  header.format_version = 1;  // the shape that must be held whole
  header.min_reader = 1;
  header.writer = "2.1.0";
  header.writer_stable = true;
  header.created = "2024-01-01T00:00:00Z";
  header.protection = ProfilePackageProtection::kNONE;

  {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    const auto header_bytes = EncodeProfilePackageHeader(header);
    ASSERT_EQ(file.write(header_bytes), header_bytes.size());
    ASSERT_TRUE(file.resize(cap + 1));
  }
  ASSERT_GT(QFileInfo(path).size(), cap);

  const auto read = ReadProfilePackage(path, dir.path() + "/out", {});
  EXPECT_EQ(read.status, ProfilePackageReadStatus::kTOO_LARGE);

  // Both numbers, because "too large" alone is not something a user can act on.
  EXPECT_TRUE(read.detail.contains("/"));

  // And nothing was unpacked on the way to saying so.
  EXPECT_FALSE(QDir(dir.path() + "/out").exists());
}

TEST(ProfilePackageOversizeTest, AHugeStreamedFileIsNotRefusedForItsSize) {
  // The ceiling belongs to the legacy shape alone. Applying it to a streamed
  // body would put back the very limit streaming exists to remove -- so a big
  // version 2 file must get past the guard and fail, if at all, on its
  // contents.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.path() + "/huge-v2.gfp";
  const auto cap = ProfilePackagePayloadCap();

  ProfilePackageHeader header;  // defaults to the streamed format
  header.writer = "9.9.9";
  header.created = "2024-01-01T00:00:00Z";
  header.protection = ProfilePackageProtection::kNONE;
  ASSERT_GE(header.format_version, kProfilePackageStreamedFrom);

  {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    const auto header_bytes = EncodeProfilePackageHeader(header);
    ASSERT_EQ(file.write(header_bytes), header_bytes.size());
    ASSERT_TRUE(file.resize(cap + 1));
  }

  const auto read = ReadProfilePackage(path, dir.path() + "/out", {});
  EXPECT_NE(read.status, ProfilePackageReadStatus::kTOO_LARGE)
      << "a streamed package was refused for its size";
}

TEST(ProfilePackageOversizeTest, ASmallFileIsNotRefusedForItsSize) {
  // The guard must bound what this build cannot hold, not become a second,
  // stricter limit that rejects packages it could have opened.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.path() + "/atlimit.gfp";
  {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write("not a package at all"), 20);
  }

  const auto read = ReadProfilePackage(path, dir.path() + "/out", {});
  EXPECT_NE(read.status, ProfilePackageReadStatus::kTOO_LARGE);
}

TEST(ProfileMeasureTest, MeasuresEveryAreaThroughTheStorage) {
  // The figures behind the export dialog's contents list, which is the last
  // thing a user reads before handing the file to somebody.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  MakeProfile(root);
  WriteFile(root + "/dbs/Key DB 2/pubring.kbx", "second keyring");

  FsProfileAccessor storage(root, root + "/config/config.ini");
  const auto areas = MeasureProfileAreas(storage);

  EXPECT_GT(areas.value("config"), 0);
  EXPECT_GT(areas.value("data_objs"), 0);
  EXPECT_GT(areas.value("secure"), 0);
  EXPECT_GT(areas.value("workspace"), 0);

  // Both database directories, since both travel.
  EXPECT_EQ(areas.value("key_databases"),
            QByteArray("keyring").size() + QByteArray("second keyring").size());

  // Areas a package never carries are not measured into any row, or the total
  // would promise a file bigger than the one that gets written.
  qint64 total = 0;
  for (const auto &bytes : areas) total += bytes;
  EXPECT_LT(total, DirectorySizeOfTreeForTest(root));
}

TEST(ProfileMeasureTest, ReportsAKeyHeldInMemoryRatherThanZero) {
  // The defect this is the guard for: `secure` was measured by walking
  // <root>/secure, and a packaged session holds that area in memory, where a
  // walk finds nothing. The row is "This profile's own key" -- the one entry
  // in the list whose whole purpose is to say that the file about to be
  // written contains it -- and it read 0 B.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  MakeProfile(root);

  auto inner = QSharedPointer<FsProfileAccessor>::create(
      root, root + "/config/config.ini");
  MemoryAreaProfileAccessor storage(inner, {ProfileArea::kSecure});

  // Nothing on the storage: this is the state a packaged session is in.
  ASSERT_TRUE(QDir(root + "/secure").removeRecursively());

  const GFBuffer key(QByteArray(256, 'k'));
  ASSERT_TRUE(storage.Write(ProfileArea::kSecure, "app.key", key));

  const auto areas = MeasureProfileAreas(storage);
  EXPECT_EQ(areas.value("secure"), 256);

  // And the rest still comes from the tree, unchanged by the decorator.
  EXPECT_GT(areas.value("data_objs"), 0);
}

TEST(ProfileMeasureTest, CountsASettingsFileKeptOutsideTheProfile) {
  // An installed profile on Windows keeps its INI in AppConfigLocation, and on
  // POSIX writes through Qt's native store; neither is under the profile root,
  // so measuring `config/` reported 0 B beside "Settings" for exactly the
  // profiles people export most.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  QDir().mkpath(root);
  const auto ini = dir.path() + "/elsewhere/config.ini";
  WriteFile(ini, "[basic]\nlanguage=de_DE\n");

  FsProfileAccessor storage(root, ini);
  EXPECT_EQ(MeasureProfileAreas(storage).value("config"),
            QFileInfo(ini).size());
}

TEST(ProfileMeasureTest, CountsTheSettingsFileInsideTheAreaExactlyOnce) {
  // The ordinary case, where the file is in the area that was just measured.
  // Adding it again would be a different wrong number in the same row.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  const auto ini = root + "/config/config.ini";
  WriteFile(ini, "[basic]\nlanguage=en_US\n");

  FsProfileAccessor storage(root, ini);
  EXPECT_EQ(MeasureProfileAreas(storage).value("config"),
            QFileInfo(ini).size());
}

}  // namespace GpgFrontend::Test
