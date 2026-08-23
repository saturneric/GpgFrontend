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
#include <QDirIterator>
#include <QTemporaryDir>

#include "core/profile/MemoryAreaProfileAccessor.h"
#include "core/profile/ProfileAreaTraits.h"
#include "core/profile/ProfileSecureKeyManager.h"

namespace GpgFrontend::Test {

// The object contract is asserted for both drivers in
// GpgCoreTestProfileAccessor.cpp. What is left here is what only this driver
// claims: that the bytes are not on a filesystem, that they are erased rather
// than dropped, and that everything else is still the wrapped driver's answer.

namespace {

auto Make(const QString& root, QSet<ProfileArea> resident)
    -> QSharedPointer<MemoryAreaProfileAccessor> {
  return QSharedPointer<MemoryAreaProfileAccessor>::create(
      QSharedPointer<FsProfileAccessor>::create(root, QString{}),
      std::move(resident));
}

auto CountFilesUnder(const QString& root) -> int {
  int files = 0;
  QDirIterator it(root, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    ++files;
  }
  return files;
}

}  // namespace

TEST(MemoryAreaAccessorTest, AResidentWriteReachesNoFilesystem) {
  // The whole point. A packaged session's application key arrives unprotected,
  // and this is what keeps it from being written to this machine's disk.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});
  ASSERT_TRUE(accessor->Ensure(ProfileArea::kSecure));
  ASSERT_TRUE(accessor->Write(ProfileArea::kSecure, "app.key",
                              GFBuffer(QByteArray(256, '\x41'))));

  EXPECT_EQ(CountFilesUnder(dir.path()), 0)
      << "a resident write put something on the filesystem";
  EXPECT_FALSE(QDir(dir.path() + "/secure").exists());

  // And it is still readable, which is the other half of being useful.
  const auto read = accessor->Read(ProfileArea::kSecure, "app.key");
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->Size(), 256U);
}

TEST(MemoryAreaAccessorTest, TheKeysProtectionCanStillBeChangedWithNoPath) {
  // KeyPath() is empty for a resident area, and the settings tab used to hand
  // that empty string to the file I/O that verifies and rewrites the key. The
  // read failed for a reason that had nothing to do with the PIN, so a packaged
  // session reported "the current PIN is not correct" to a user typing the
  // right one and never let them change it.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});
  ProfileSecureKeyManager manager(accessor);

  const GFBuffer key(QByteArray("0123456789abcdef0123456789abcdef"));
  const GFBuffer old_pin(QString("old-pin"));
  const GFBuffer new_pin(QString("new-pin"));

  ASSERT_TRUE(ProfileSecureKeyManager::ChangeProtection(
                  manager.KeySink(), nullptr, key, AppKeyProtection::kNONE,
                  AppKeyProtection::kPIN, old_pin)
                  .Ok());

  // What the PIN dialog does before it re-keys, and what used to fail.
  auto stored = manager.ReadStoredKey();
  ASSERT_TRUE(stored.has_value()) << "the sealed key could not be read back";
  auto opened = ProfileSecureKeyManager::UnsealKey(old_pin, {}, *stored);
  ASSERT_TRUE(opened.has_value()) << "the correct PIN was rejected";
  EXPECT_EQ(*opened, key);

  ASSERT_TRUE(ProfileSecureKeyManager::ChangeProtection(
                  manager.KeySink(), nullptr, key, AppKeyProtection::kPIN,
                  AppKeyProtection::kPIN, new_pin)
                  .Ok());

  auto rekeyed = manager.ReadStoredKey();
  ASSERT_TRUE(rekeyed.has_value());
  EXPECT_FALSE(ProfileSecureKeyManager::UnsealKey(old_pin, {}, *rekeyed))
      << "the old PIN still opens the key";
  auto reopened = ProfileSecureKeyManager::UnsealKey(new_pin, {}, *rekeyed);
  ASSERT_TRUE(reopened.has_value());
  EXPECT_EQ(*reopened, key);

  // And none of it reached a disk, which is the point of the area being
  // resident in the first place.
  EXPECT_EQ(CountFilesUnder(dir.path()), 0)
      << "changing the key's protection wrote it to the filesystem";
}

TEST(MemoryAreaAccessorTest, TheKeySetLoadsAndRotatesWithNoFileAnywhere) {
  // Nothing exercised Load() at all, and it is the path a packaged session
  // takes on every start: generate or open the profile's own key, mint the
  // rotating key for this period, and register every earlier one -- all of it
  // now against a storage that has no filenames to work from.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});
  const GFBuffer pin(QString("a-pin"));

  GFBuffer root_key;
  {
    ProfileSecureKeyManager manager(accessor);
    ASSERT_TRUE(manager.Load(pin, {}, true).Ok());

    EXPECT_EQ(manager.Mode(), ProfileKeyMode::kROTATING);
    EXPECT_FALSE(manager.RootKey().Empty());
    EXPECT_FALSE(manager.ActiveKeyId().Empty());
    EXPECT_FALSE(manager.KeyById(manager.ActiveKeyId()).Empty());

    // The location is reportable even though there is no path to report.
    EXPECT_TRUE(manager.KeyPath().isEmpty());
    EXPECT_FALSE(manager.KeyLocationForMessage().isEmpty());

    root_key = manager.RootKey();
  }

  // Both the profile's own key and the rotating key it minted are held here.
  EXPECT_EQ(CountFilesUnder(dir.path()), 0)
      << "loading the key set wrote something to the filesystem";
  EXPECT_TRUE(accessor->Exists(ProfileArea::kSecure, "app.key"));
  EXPECT_GE(accessor->List(ProfileArea::kSecure, "*.key").size(), 2);

  // Opening the same storage again finds the key that is already there rather
  // than generating a second one.
  ProfileSecureKeyManager again(accessor);
  ASSERT_TRUE(again.Load(pin, {}, true).Ok());
  EXPECT_EQ(again.RootKey(), root_key);
}

TEST(MemoryAreaAccessorTest, AResetWithNoPathIsRefusedNotReportedAsDone) {
  // A resident area has no directory to reset, and the join that used to happen
  // anyway addressed "/app.key" -- found nothing, and reported success to a
  // user who had just been told their key was destroyed.
  EXPECT_FALSE(ProfileSecureKeyManager::ResetKeyStorage({}));
}

TEST(MemoryAreaAccessorTest, AResidentAreaHasNoPathAndSaysSo) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});

  EXPECT_TRUE(accessor->IsAreaResident(ProfileArea::kSecure));
  EXPECT_TRUE(accessor->PathOf(ProfileArea::kSecure).isEmpty());
  EXPECT_TRUE(accessor->PathOf(ProfileArea::kSecure, "app.key").isEmpty());
}

TEST(MemoryAreaAccessorTest, EveryOtherAreaIsTheWrappedDriverUnchanged) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto inner = QSharedPointer<FsProfileAccessor>::create(dir.path(), QString{});
  MemoryAreaProfileAccessor accessor(inner, {ProfileArea::kSecure});

  for (const auto area :
       {ProfileArea::kRoot, ProfileArea::kConfig, ProfileArea::kDataObjects,
        ProfileArea::kLogs, ProfileArea::kModules, ProfileArea::kWorkspace,
        ProfileArea::kScratch}) {
    EXPECT_FALSE(accessor.IsAreaResident(area));
    EXPECT_EQ(accessor.PathOf(area), inner->PathOf(area));
  }

  // A delegated write really does land on the wrapped driver.
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kDataObjects));
  ASSERT_TRUE(accessor.Write(ProfileArea::kDataObjects, "abcd",
                             GFBuffer(QByteArray("sealed"))));
  EXPECT_TRUE(
      QFileInfo::exists(inner->PathOf(ProfileArea::kDataObjects, "abcd")));
}

TEST(MemoryAreaAccessorTest, TheStorageDescribesItselfNotTheDecorator) {
  // Reporting the storage as volatile because one area is in memory would be a
  // claim about the GnuPG home directory too, and that is where the user's own
  // private keys are.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto inner = QSharedPointer<FsProfileAccessor>::create(dir.path(), QString{});
  MemoryAreaProfileAccessor accessor(inner, {ProfileArea::kSecure});

  EXPECT_EQ(accessor.Label(), inner->Label());
  EXPECT_EQ(accessor.IsVolatile(), inner->IsVolatile());
  EXPECT_EQ(accessor.IsEncryptedAtRest(), inner->IsEncryptedAtRest());
  EXPECT_FALSE(accessor.IsVolatile());

  // The driver token does say what happened, because the startup log is where
  // that belongs.
  EXPECT_TRUE(accessor.Driver().startsWith(inner->Driver()));
  EXPECT_TRUE(accessor.Driver().contains("mem"));
}

TEST(MemoryAreaAccessorTest, ReleaseErasesRatherThanDrops) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});
  ASSERT_TRUE(accessor->Write(ProfileArea::kSecure, "app.key",
                              GFBuffer(QByteArray("key material"))));

  // A share of the very same storage. GFBuffer is copy-on-write and Zeroize()
  // deliberately wipes through every share, so this is how the erasure can be
  // observed at all -- and it is the reason a caller that needs its bytes to
  // outlive the storage has to take a copy of its own, rather than something
  // Release() is expected to protect it from.
  const auto shared = accessor->Read(ProfileArea::kSecure, "app.key");
  ASSERT_TRUE(shared.has_value());

  accessor->Release(ProfileStorageRelease::kSCRUB);

  EXPECT_FALSE(accessor->Exists(ProfileArea::kSecure, "app.key"));
  EXPECT_EQ(shared->ConvertToQByteArray(), QByteArray(12, '\0'))
      << "the key was dropped but not erased";
}

TEST(MemoryAreaAccessorTest, ReleaseTwiceIsHarmless) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});
  ASSERT_TRUE(accessor->Write(ProfileArea::kSecure, "app.key",
                              GFBuffer(QByteArray("x"))));

  accessor->Release(ProfileStorageRelease::kSCRUB);
  accessor->Release(ProfileStorageRelease::kFAST);
  EXPECT_TRUE(accessor->List(ProfileArea::kSecure, "*").isEmpty());
}

TEST(MemoryAreaAccessorTest, ReplacingAValueErasesTheOldOne) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});
  ASSERT_TRUE(accessor->Write(ProfileArea::kSecure, "app.key",
                              GFBuffer(QByteArray("the old key"))));
  const auto old = accessor->Read(ProfileArea::kSecure, "app.key");
  ASSERT_TRUE(old.has_value());

  ASSERT_TRUE(accessor->Write(ProfileArea::kSecure, "app.key",
                              GFBuffer(QByteArray("the new key"))));

  EXPECT_EQ(old->ConvertToQByteArray(), QByteArray(11, '\0'))
      << "the replaced key was left legible in memory";

  // And the replacement survived, which is what proves the wipe hit the old
  // storage rather than the new one.
  const auto current = accessor->Read(ProfileArea::kSecure, "app.key");
  ASSERT_TRUE(current.has_value());
  EXPECT_EQ(current->ConvertToQByteArray(), QByteArray("the new key"));
}

TEST(MemoryAreaAccessorTest, RemovingErasesToo) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});
  ASSERT_TRUE(accessor->Write(ProfileArea::kSecure, "rotated.key",
                              GFBuffer(QByteArray("rotated"))));
  const auto held = accessor->Read(ProfileArea::kSecure, "rotated.key");
  ASSERT_TRUE(held.has_value());

  EXPECT_TRUE(accessor->Remove(ProfileArea::kSecure, "rotated.key"));
  EXPECT_EQ(held->ConvertToQByteArray(), QByteArray(7, '\0'));
}

TEST(MemoryAreaAccessorTest, AnAreaThatNeedsAPathIsRefused) {
  // Asking for one is a programming error, and honouring it would hand callers
  // an empty path for a directory GnuPG is about to be given.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kRoot, ProfileArea::kSecure});

  EXPECT_FALSE(accessor->IsAreaResident(ProfileArea::kRoot));
  EXPECT_FALSE(accessor->PathOf(ProfileArea::kRoot).isEmpty());
  EXPECT_TRUE(accessor->IsAreaResident(ProfileArea::kSecure));
}

TEST(MemoryAreaAccessorTest, AnOversizedWriteIsRefusedRatherThanHeld) {
  // A package decides what its members are called and how big they are, and
  // this map is process heap rather than the storage the probe budgeted for.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  auto accessor = Make(dir.path(), {ProfileArea::kSecure});

  const QByteArray huge(static_cast<int>(kResidentAreaByteCeiling) + 1, 'x');
  EXPECT_FALSE(
      accessor->Write(ProfileArea::kSecure, "fat.key", GFBuffer(huge)));
  EXPECT_FALSE(accessor->Exists(ProfileArea::kSecure, "fat.key"));

  // The ceiling is on the area, not on one object: many small writes that add
  // up are refused too.
  const QByteArray half(static_cast<int>(kResidentAreaByteCeiling / 2) + 1,
                        'x');
  EXPECT_TRUE(accessor->Write(ProfileArea::kSecure, "a.key", GFBuffer(half)));
  EXPECT_FALSE(accessor->Write(ProfileArea::kSecure, "b.key", GFBuffer(half)));
}

TEST(MemoryAreaAccessorTest, SettingsStillComeFromTheWrappedDriver) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto ini = dir.path() + "/config/config.ini";
  ASSERT_TRUE(QDir().mkpath(dir.path() + "/config"));

  auto inner = QSharedPointer<FsProfileAccessor>::create(dir.path(), ini);
  MemoryAreaProfileAccessor accessor(inner, {ProfileArea::kSecure});

  auto settings = accessor.Settings();
  settings.setValue("advanced/thing", 42);
  settings.sync();

  EXPECT_TRUE(QFileInfo::exists(ini));
}

}  // namespace GpgFrontend::Test
