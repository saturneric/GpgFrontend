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
#include "core/profile/ProfileAccessor.h"
#include "core/profile/ProfileSession.h"

namespace GpgFrontend::Test {

// The filesystem driver is the only one today, and it is the layout every
// existing installation already has on disk. What is asserted here is the
// contract the interface promises, so that a memory- or FUSE-backed driver has
// something to be measured against rather than a shape to guess at.

namespace {

auto Every() -> QList<ProfileArea> {
  return {ProfileArea::kRoot,        ProfileArea::kConfig,
          ProfileArea::kDataObjects, ProfileArea::kSecure,
          ProfileArea::kLogs,        ProfileArea::kModules,
          ProfileArea::kWorkspace,   ProfileArea::kScratch};
}

}  // namespace

TEST(ProfileAccessorTest, EveryAreaResolvesUnderTheRoot) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), dir.path() + "/config/config.ini");
  EXPECT_EQ(accessor.Driver(), QString("fs"));

  EXPECT_EQ(accessor.PathOf(ProfileArea::kRoot), dir.path());
  for (const auto area : Every()) {
    EXPECT_TRUE(accessor.PathOf(area).startsWith(dir.path()))
        << accessor.PathOf(area).toStdString();
  }

  // The names are a wire format of their own: every existing profile on disk
  // already has these directories, and renaming one strands them.
  EXPECT_EQ(accessor.PathOf(ProfileArea::kDataObjects),
            dir.path() + "/data_objs");
  EXPECT_EQ(accessor.PathOf(ProfileArea::kSecure, "app.key"),
            dir.path() + "/secure/app.key");
  EXPECT_EQ(accessor.PathOf(ProfileArea::kLogs), dir.path() + "/logs");
  EXPECT_EQ(accessor.PathOf(ProfileArea::kModules), dir.path() + "/mods");
  EXPECT_EQ(accessor.PathOf(ProfileArea::kWorkspace),
            dir.path() + "/workspace");
  EXPECT_EQ(accessor.PathOf(ProfileArea::kConfig), dir.path() + "/config");
  EXPECT_EQ(accessor.PathOf(ProfileArea::kScratch), dir.path() + "/.scratch");
}

TEST(ProfileAccessorTest, EnsureIsIdempotent) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});

  for (const auto area : Every()) {
    EXPECT_TRUE(accessor.Ensure(area));
    EXPECT_TRUE(accessor.Ensure(area)) << "second call must also succeed";
    EXPECT_TRUE(QDir(accessor.PathOf(area)).exists());
  }
}

// The object contract, run against every driver rather than against the one
// that happened to be written first. A decorator that holds an area in memory
// has to behave identically through this interface or the callers above it --
// the key manager especially -- would need to know which driver they had.
//
// The area under test is kSecure: it is the one the memory driver may hold, so
// parameterising on it exercises the resident path in one case and the
// delegating path in the other.

namespace {

struct DriverUnderTest {
  QString name;
  std::function<QSharedPointer<ProfileAccessor>(const QString&)> make;
};

auto Drivers() -> QList<DriverUnderTest> {
  return {
      {"fs",
       [](const QString& root) -> QSharedPointer<ProfileAccessor> {
         return QSharedPointer<FsProfileAccessor>::create(root, QString{});
       }},
      {"memory",
       [](const QString& root) -> QSharedPointer<ProfileAccessor> {
         return QSharedPointer<MemoryAreaProfileAccessor>::create(
             QSharedPointer<FsProfileAccessor>::create(root, QString{}),
             QSet<ProfileArea>{ProfileArea::kSecure});
       }},
  };
}

}  // namespace

class ProfileAccessorContractTest
    : public ::testing::TestWithParam<DriverUnderTest> {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    accessor_ = GetParam().make(dir_.path());
    ASSERT_FALSE(accessor_.isNull());
  }

  QTemporaryDir dir_;
  QSharedPointer<ProfileAccessor> accessor_;
};

INSTANTIATE_TEST_SUITE_P(EveryDriver, ProfileAccessorContractTest,
                         ::testing::ValuesIn(Drivers()), [](const auto& info) {
                           return info.param.name.toStdString();
                         });

TEST_P(ProfileAccessorContractTest, WhatIsWrittenIsWhatIsRead) {
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  const GFBuffer value(QByteArray("some sealed bytes"));
  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "abcd", value));

  EXPECT_TRUE(accessor_->Exists(ProfileArea::kSecure, "abcd"));

  const auto read = accessor_->Read(ProfileArea::kSecure, "abcd");
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(*read, value);
}

TEST_P(ProfileAccessorContractTest, AWriteReplacesRatherThanAppends) {
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "abcd",
                               GFBuffer(QByteArray("first, and longer"))));
  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "abcd",
                               GFBuffer(QByteArray("second"))));

  const auto read = accessor_->Read(ProfileArea::kSecure, "abcd");
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->ConvertToQByteArray(), QByteArray("second"));
}

TEST_P(ProfileAccessorContractTest, ReadingSomethingAbsentIsEmptyNotAnError) {
  EXPECT_FALSE(accessor_->Exists(ProfileArea::kSecure, "nothing"));
  EXPECT_FALSE(accessor_->Read(ProfileArea::kSecure, "nothing").has_value());

  // Listing an area that was never made is empty rather than a failure: a
  // profile on its first start has none of them yet.
  EXPECT_TRUE(accessor_->List(ProfileArea::kSecure, "*").isEmpty());
  EXPECT_EQ(accessor_->TotalSize(ProfileArea::kSecure, "*"), 0);
}

TEST_P(ProfileAccessorContractTest, RemovingSomethingAbsentSucceeds) {
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  // A clear-out that stops on the first missing file would leave the rest
  // behind, so absence is success.
  EXPECT_TRUE(accessor_->Remove(ProfileArea::kSecure, "never-there"));

  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "abcd",
                               GFBuffer(QByteArray("x"))));
  EXPECT_TRUE(accessor_->Remove(ProfileArea::kSecure, "abcd"));
  EXPECT_FALSE(accessor_->Exists(ProfileArea::kSecure, "abcd"));
}

TEST_P(ProfileAccessorContractTest, ListingAndSizingRespectThePattern) {
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "a.key",
                               GFBuffer(QByteArray("1234567890"))));
  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "b.key",
                               GFBuffer(QByteArray("12345"))));
  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "notes.txt",
                               GFBuffer(QByteArray("ignored"))));

  auto keys = accessor_->List(ProfileArea::kSecure, "*.key");
  keys.sort();
  EXPECT_EQ(keys, QStringList({"a.key", "b.key"}));

  EXPECT_EQ(accessor_->TotalSize(ProfileArea::kSecure, "*.key"), 15);
  EXPECT_EQ(accessor_->List(ProfileArea::kSecure, "*").size(), 3);
}

TEST_P(ProfileAccessorContractTest, APatternMatchesWholeNamesNotSubstrings) {
  // The memory driver matched unanchored, so "*.key" caught anything with
  // ".key" anywhere in it while the filesystem driver caught only names ending
  // in it. An imported package chooses the names in the secure area, and this
  // listing is what the trial-decrypt loops walk.
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "real.key",
                               GFBuffer(QByteArray("1234"))));
  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "decoy.key.bak",
                               GFBuffer(QByteArray("5678"))));
  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "a.keyring",
                               GFBuffer(QByteArray("90"))));

  EXPECT_EQ(accessor_->List(ProfileArea::kSecure, "*.key"),
            QStringList({"real.key"}));
  EXPECT_EQ(accessor_->TotalSize(ProfileArea::kSecure, "*.key"), 4);
}

TEST_P(ProfileAccessorContractTest, AnEmptyPatternMeansEverything) {
  // QDir reads an empty name filter as "nothing matches" and the memory driver
  // read it as "everything", so the same call answered differently depending on
  // which driver was underneath.
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "a.key",
                               GFBuffer(QByteArray("1234"))));
  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "b.txt",
                               GFBuffer(QByteArray("56"))));

  EXPECT_EQ(accessor_->List(ProfileArea::kSecure, {}).size(), 2);
  EXPECT_EQ(accessor_->TotalSize(ProfileArea::kSecure, {}), 6);
}

TEST_P(ProfileAccessorContractTest, AnEmptyNameIsNotAnObject) {
  // An empty name addresses the area's own directory on a filesystem driver and
  // nothing at all on a memory one, so Exists() said true on one and false on
  // the other, and Remove() could have been read as "remove the area".
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  EXPECT_FALSE(accessor_->Exists(ProfileArea::kSecure, {}));
  EXPECT_FALSE(accessor_->Write(ProfileArea::kSecure, {},
                                GFBuffer(QByteArray("nowhere"))));
  EXPECT_FALSE(accessor_->Remove(ProfileArea::kSecure, {}));
}

TEST_P(ProfileAccessorContractTest, AreasDoNotSeeEachOther) {
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kDataObjects));
  ASSERT_TRUE(accessor_->Ensure(ProfileArea::kSecure));

  ASSERT_TRUE(accessor_->Write(ProfileArea::kSecure, "app.key",
                               GFBuffer(QByteArray("key material"))));

  // The closed set of areas is what stops one part of the profile reaching
  // into another by assembling a path. For the memory driver this is also the
  // check that a resident area and a delegated one stay separate.
  EXPECT_FALSE(accessor_->Exists(ProfileArea::kDataObjects, "app.key"));
  EXPECT_TRUE(accessor_->List(ProfileArea::kDataObjects, "*").isEmpty());
}

TEST(ProfileAccessorTest, TheFilesystemDriverPutsBytesWhereItSaysItDoes) {
  // Split out of the contract above: only a driver with a path can promise
  // this, and it is the promise everything that needs a real path depends on.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kDataObjects));
  ASSERT_TRUE(accessor.Write(ProfileArea::kDataObjects, "abcd",
                             GFBuffer(QByteArray("some sealed bytes"))));

  QFile file(accessor.PathOf(ProfileArea::kDataObjects, "abcd"));
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(file.readAll(), QByteArray("some sealed bytes"));
}

TEST(ProfileAccessorTest, SettingsFollowTheFileItWasGiven) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto ini = dir.path() + "/config/config.ini";
  FsProfileAccessor rooted(dir.path(), ini);
  EXPECT_EQ(rooted.Settings().fileName(), ini);

  // An empty settings file means "use the platform's native store", which is
  // what the installed root does on POSIX.
  FsProfileAccessor native(dir.path(), {});
  EXPECT_NE(native.Settings().fileName(), ini);
}

TEST(ProfileAccessorTest, TheLiveSessionUsesTheFilesystemDriver) {
  // Not a tautology: it is the assertion that this build ships exactly one
  // driver and that the running process is on it.
  ASSERT_TRUE(ProfileSession::Loaded());
  EXPECT_EQ(ProfileSession::Instance().Accessor().Driver(), QString("fs"));
}

// The capability half of the contract. It exists so that a driver storing the
// profile somewhere the platform does not leave readable can say so, and so
// that the one which cannot say so is obliged to admit it.

TEST(ProfileAccessorTest, TheFilesystemDriverClaimsNoProtection) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), dir.path() + "/config/config.ini");

  EXPECT_EQ(accessor.Driver(), "fs");
  EXPECT_FALSE(accessor.IsVolatile());

  // Whole-disk encryption may well be on underneath. This driver did not
  // arrange it and cannot tell whether it outlives Release(), and claiming a
  // protection we did not provide is worse than admitting we provide none.
  EXPECT_FALSE(accessor.IsEncryptedAtRest());

  EXPECT_FALSE(accessor.Label().isEmpty());
}

TEST(ProfileAccessorTest, TheFilesystemDriverReportsFreeSpace) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  EXPECT_GT(accessor.FreeBytes(), 0);
}

TEST(ProfileAccessorTest, ReleasingTheFilesystemDriverKeepsTheProfile) {
  // An installed or named profile is where the user keeps their keys. A window
  // closing is not a reason to delete it, whatever mode the caller passes.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kSecure));
  ASSERT_TRUE(accessor.Write(ProfileArea::kSecure, "app.key",
                             GFBuffer(QByteArray("key material"))));

  for (const auto mode :
       {ProfileStorageRelease::kKEEP, ProfileStorageRelease::kSCRUB,
        ProfileStorageRelease::kFAST}) {
    accessor.Release(mode);
    EXPECT_TRUE(QDir(dir.path()).exists());
    EXPECT_TRUE(accessor.Exists(ProfileArea::kSecure, "app.key"));
  }
}

TEST(ProfileAccessorTest, ScratchIsHiddenSoItIsNeverPacked) {
  // The scratch area is a directory like any other, but its name has to stay
  // dot-prefixed: that is the whole reason the area table already
  // skips it, and staging must never travel inside a package.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});

  const auto path = accessor.PathOf(ProfileArea::kScratch);
  EXPECT_TRUE(path.startsWith(dir.path() + "/"));
  EXPECT_TRUE(QFileInfo(path).fileName().startsWith('.'));

  ASSERT_TRUE(accessor.Ensure(ProfileArea::kScratch));
  EXPECT_TRUE(QDir(path).exists());
}

}  // namespace GpgFrontend::Test
