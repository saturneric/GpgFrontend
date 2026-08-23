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
#include <QFile>
#include <QTemporaryDir>

#include "core/profile/MemoryAreaProfileAccessor.h"
#include "core/profile/ProfileMember.h"

namespace GpgFrontend::Test {

namespace {

void WriteFile(const QString& path, const QByteArray& bytes) {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(bytes);
}

auto Drain(ProfileMemberSource& source) -> QList<ProfileMember> {
  QList<ProfileMember> members;
  ProfileMember member;
  while (source.Next(member)) {
    members.append(member);
    member = {};
  }
  return members;
}

auto PathsOf(const QList<ProfileMember>& members) -> QStringList {
  QStringList paths;
  for (const auto& member : members) paths << member.path;
  paths.sort();
  return paths;
}

auto FindByPath(const QList<ProfileMember>& members, const QString& path)
    -> std::optional<ProfileMember> {
  for (const auto& member : members) {
    if (member.path == path) return member;
  }
  return {};
}

}  // namespace

// The rule these tests exist for was three `if (name == "app.key")` checks in
// three files. Getting the first wrong ships a package nobody can open; getting
// the second wrong ships one whose older data objects can never be read again,
// with nothing to show that anything is missing.

TEST(SecureAreaMembersTest, TheRootKeyComesFromTheKeyInHandNotFromStorage) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor storage(dir.path(), {});
  ASSERT_TRUE(storage.Ensure(ProfileArea::kSecure));

  // What is stored is sealed by this machine's credential store. Carrying it
  // would produce a package that will not open on the computer it was made for.
  ASSERT_TRUE(storage.Write(ProfileArea::kSecure, "app.key",
                            GFBuffer(QByteArray("SEALED-BY-THIS-MACHINE"))));

  const auto members =
      ResolveSecureAreaMembers(storage, GFBuffer(QByteArray("plaintext-root")));

  const auto root = FindByPath(members, "secure/app.key");
  ASSERT_TRUE(root.has_value());
  EXPECT_EQ(root->bytes.ConvertToQByteArray(), QByteArray("plaintext-root"));
  EXPECT_FALSE(root->IsFileOnStorage())
      << "the key must travel as bytes, never as a path to a stored file";
}

TEST(SecureAreaMembersTest, RotatedKeysAreCarriedExactlyAsStored) {
  // They are already encrypted under the root key, which is travelling beside
  // them, so rewriting them would mean decrypting key material for no reason.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor storage(dir.path(), {});
  ASSERT_TRUE(storage.Ensure(ProfileArea::kSecure));
  ASSERT_TRUE(storage.Write(ProfileArea::kSecure, "DEADBEEF.key",
                            GFBuffer(QByteArray("rotated-as-stored"))));
  ASSERT_TRUE(storage.Write(ProfileArea::kSecure, "app.key",
                            GFBuffer(QByteArray("stored-root"))));

  const auto members =
      ResolveSecureAreaMembers(storage, GFBuffer(QByteArray("plaintext-root")));

  EXPECT_EQ(PathsOf(members),
            QStringList({"secure/DEADBEEF.key", "secure/app.key"}));

  const auto rotated = FindByPath(members, "secure/DEADBEEF.key");
  ASSERT_TRUE(rotated.has_value());
  EXPECT_EQ(rotated->bytes.ConvertToQByteArray(),
            QByteArray("rotated-as-stored"));
}

TEST(SecureAreaMembersTest, ResolvedMembersSurviveTheStorageBeingReleased) {
  // The members are resolved on the window's thread and packed on a worker. A
  // GFBuffer copy shares its storage and Zeroize() wipes through every share,
  // so members that merely referenced the accessor's map were erased under the
  // packer by any release in between -- an unmount, or the shutdown watchdog.
  // The package came out with zeroes where the rotated keys should be: no
  // error, no short write, and every data object of an earlier period
  // permanently unreadable on the other machine.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto storage = QSharedPointer<MemoryAreaProfileAccessor>::create(
      QSharedPointer<FsProfileAccessor>::create(dir.path(), QString{}),
      QSet<ProfileArea>{ProfileArea::kSecure});
  ASSERT_TRUE(storage->Write(ProfileArea::kSecure, "DEADBEEF.key",
                             GFBuffer(QByteArray("rotated-as-stored"))));

  GFBuffer root_key(QByteArray("plaintext-root"));
  const auto members = ResolveSecureAreaMembers(*storage, root_key);
  ASSERT_EQ(members.size(), 2);

  // Everything the members could have been sharing, erased.
  storage->Release(ProfileStorageRelease::kSCRUB);
  root_key.Zeroize();

  const auto rotated = FindByPath(members, "secure/DEADBEEF.key");
  ASSERT_TRUE(rotated.has_value());
  EXPECT_EQ(rotated->bytes.ConvertToQByteArray(),
            QByteArray("rotated-as-stored"))
      << "a rotated key was wiped out from under the members";

  const auto root = FindByPath(members, "secure/app.key");
  ASSERT_TRUE(root.has_value());
  EXPECT_EQ(root->bytes.ConvertToQByteArray(), QByteArray("plaintext-root"))
      << "the application key was wiped out from under the members";
}

TEST(SecureAreaMembersTest, TheStoredRootKeyIsNeverEmittedTwice) {
  // app.key is in the same listing as the rotated keys, so the one exclusion
  // that stops it being emitted a second time -- as its sealed form -- lives
  // here rather than at each caller.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor storage(dir.path(), {});
  ASSERT_TRUE(storage.Ensure(ProfileArea::kSecure));
  ASSERT_TRUE(storage.Write(ProfileArea::kSecure, "app.key",
                            GFBuffer(QByteArray("stored-root"))));

  const auto members =
      ResolveSecureAreaMembers(storage, GFBuffer(QByteArray("plaintext")));

  int roots = 0;
  for (const auto& member : members) {
    if (member.path == "secure/app.key") ++roots;
  }
  EXPECT_EQ(roots, 1);
}

TEST(SecureAreaMembersTest, AMemoryHeldAreaResolvesTheSameAsAFilesystemOne) {
  // The property that makes this worth having: a packaged session holds the
  // secure area in memory and has no files to walk, and the packer must not
  // care which of the two it is looking at.
  QTemporaryDir fs_dir;
  QTemporaryDir mem_dir;
  ASSERT_TRUE(fs_dir.isValid());
  ASSERT_TRUE(mem_dir.isValid());

  FsProfileAccessor on_disk(fs_dir.path(), {});
  ASSERT_TRUE(on_disk.Ensure(ProfileArea::kSecure));
  ASSERT_TRUE(on_disk.Write(ProfileArea::kSecure, "CAFE01.key",
                            GFBuffer(QByteArray("rotated"))));

  MemoryAreaProfileAccessor in_memory(
      QSharedPointer<FsProfileAccessor>::create(mem_dir.path(), QString{}),
      {ProfileArea::kSecure});
  ASSERT_TRUE(in_memory.Write(ProfileArea::kSecure, "CAFE01.key",
                              GFBuffer(QByteArray("rotated"))));

  const GFBuffer root(QByteArray("root"));
  const auto from_disk = ResolveSecureAreaMembers(on_disk, root);
  const auto from_memory = ResolveSecureAreaMembers(in_memory, root);

  EXPECT_EQ(PathsOf(from_disk), PathsOf(from_memory));

  // And the memory one really had nothing on a filesystem to be read from.
  EXPECT_TRUE(in_memory.PathOf(ProfileArea::kSecure).isEmpty());
  EXPECT_FALSE(QDir(mem_dir.path() + "/secure").exists());
}

TEST(SecureAreaMembersTest, NoKeyMeansNoMembersRatherThanAnEmptyKeyMember) {
  // An export with no application key is not an export with a blank one: the
  // caller has to be able to tell, and a zero-length app.key in a package would
  // be a profile that opens to nothing.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor storage(dir.path(), {});
  EXPECT_TRUE(ResolveSecureAreaMembers(storage, GFBuffer()).isEmpty());
}

TEST(TreeMemberSourceTest, ItYieldsWhatTravelsAndReportsWhatItLeft) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  WriteFile(root + "/profile.json", "{}");
  WriteFile(root + "/config/config.ini", "[basic]\n");
  WriteFile(root + "/data_objs/abcd", "sealed");
  WriteFile(root + "/db/pubring.kbx", "keys");
  WriteFile(root + "/db/S.gpg-agent", "socket");
  WriteFile(root + "/secure/app.key", "must not be walked");
  WriteFile(root + "/logs/gpgfrontend.log", "history");
  WriteFile(root + "/workspace/notes.txt", "draft");
  WriteFile(root + "/notes-beside-the-profile.txt", "private");

  TreeMemberSource source(root, false);
  const auto members = Drain(source);

  // Directories are members too, so an empty one still travels. The areas
  // themselves therefore appear beside the files inside them.
  EXPECT_EQ(PathsOf(members),
            QStringList({"config", "config/config.ini", "data_objs",
                         "data_objs/abcd", "db", "db/pubring.kbx",
                         "profile.json"}));

  // The secure area is not walked at all, directory included: its objects come
  // from the accessor, and a walk would pack the stored form of the key rather
  // than the one that opens. A session holding it in memory has no directory
  // here to walk either.
  for (const auto& member : members) {
    EXPECT_FALSE(member.path.startsWith("secure"));
  }

  EXPECT_EQ(source.Skipped(),
            QStringList({"logs", "notes-beside-the-profile.txt", "workspace"}));
}

TEST(TreeMemberSourceTest, TheWorkspaceIsYieldedWhenItIsAskedFor) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  WriteFile(root + "/profile.json", "{}");
  WriteFile(root + "/workspace/notes.txt", "draft");

  TreeMemberSource source(root, true);
  EXPECT_TRUE(PathsOf(Drain(source)).contains("workspace/notes.txt"));
  EXPECT_FALSE(source.Skipped().contains("workspace"));
}

TEST(TreeMemberSourceTest, AFileIsCarriedAsAPathNotAsBytes) {
  // A workspace can hold files far larger than anything worth holding in
  // memory, so reading one into a buffer to write it straight back out would
  // make the size of somebody's files a limit on packaging them.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  WriteFile(root + "/profile.json", "{}");

  TreeMemberSource source(root, false);
  const auto members = Drain(source);
  ASSERT_EQ(members.size(), 1);
  EXPECT_TRUE(members.first().IsFileOnStorage());
  EXPECT_TRUE(members.first().bytes.Empty());
}

TEST(TreeMemberSourceTest, ANestedProfilesFolderIsNeverDescendedInto) {
  // A root profile has the profiles folder inside it, holding every other
  // profile on the machine. A walk that recursed before deciding would carry
  // all of them.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto root = dir.path() + "/profile";
  WriteFile(root + "/profile.json", "{}");
  WriteFile(root + "/profiles/other/secure/app.key", "someone else's key");

  TreeMemberSource source(root, true);
  for (const auto& member : Drain(source)) {
    EXPECT_FALSE(member.path.startsWith("profiles/"))
        << "another profile was carried: " << member.path.toStdString();
  }
}

TEST(MemberTransferTest, MembersReachStagingUnderTheTreePrefix) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto source_root = dir.path() + "/profile";
  WriteFile(source_root + "/profile.json", "{\"id\":\"x\"}");

  QList<ProfileMember> synthetic;
  ProfileMember key;
  key.path = "secure/app.key";
  key.area = ProfileArea::kSecure;
  key.bytes = GFBuffer(QByteArray("the key"));
  synthetic.append(key);

  const auto staging = dir.path() + "/staging";

  TreeMemberSource tree(source_root, false);
  StagingMemberSink sink(staging);
  EXPECT_TRUE(TransferProfileMembers(tree, sink).isEmpty());

  ListMemberSource extra(synthetic);
  EXPECT_TRUE(TransferProfileMembers(extra, sink).isEmpty());

  // The prefix is added by the sink and nowhere else, so members speak one path
  // namespace rather than three.
  QFile marker(staging + "/profile/profile.json");
  ASSERT_TRUE(marker.open(QIODevice::ReadOnly));
  EXPECT_EQ(marker.readAll(), QByteArray("{\"id\":\"x\"}"));

  QFile stored_key(staging + "/profile/secure/app.key");
  ASSERT_TRUE(stored_key.open(QIODevice::ReadOnly));
  EXPECT_EQ(stored_key.readAll(), QByteArray("the key"));
}

TEST(MemberTransferTest, AMissingProfileIsReportedRatherThanSilentlyEmpty) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  TreeMemberSource source(dir.path() + "/not-there", false);
  StagingMemberSink sink(dir.path() + "/staging");
  EXPECT_FALSE(TransferProfileMembers(source, sink).isEmpty());
}

}  // namespace GpgFrontend::Test
