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
          ProfileArea::kWorkspace};
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

TEST(ProfileAccessorTest, WhatIsWrittenIsWhatIsRead) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kDataObjects));

  const GFBuffer value(QByteArray("some sealed bytes"));
  ASSERT_TRUE(accessor.Write(ProfileArea::kDataObjects, "abcd", value));

  EXPECT_TRUE(accessor.Exists(ProfileArea::kDataObjects, "abcd"));

  const auto read = accessor.Read(ProfileArea::kDataObjects, "abcd");
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(*read, value);

  // And PathOf agrees with where the bytes actually went, which is the promise
  // everything that needs a real path depends on.
  QFile file(accessor.PathOf(ProfileArea::kDataObjects, "abcd"));
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(file.readAll(), QByteArray("some sealed bytes"));
}

TEST(ProfileAccessorTest, AWriteReplacesRatherThanAppends) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kDataObjects));

  ASSERT_TRUE(accessor.Write(ProfileArea::kDataObjects, "abcd",
                             GFBuffer(QByteArray("first, and longer"))));
  ASSERT_TRUE(accessor.Write(ProfileArea::kDataObjects, "abcd",
                             GFBuffer(QByteArray("second"))));

  const auto read = accessor.Read(ProfileArea::kDataObjects, "abcd");
  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->ConvertToQByteArray(), QByteArray("second"));
}

TEST(ProfileAccessorTest, ReadingSomethingAbsentIsEmptyNotAnError) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});

  EXPECT_FALSE(accessor.Exists(ProfileArea::kDataObjects, "nothing"));
  EXPECT_FALSE(accessor.Read(ProfileArea::kDataObjects, "nothing").has_value());

  // Listing an area that was never made is empty rather than a failure: a
  // profile on its first start has none of them yet.
  EXPECT_TRUE(accessor.List(ProfileArea::kDataObjects, "*").isEmpty());
  EXPECT_EQ(accessor.TotalSize(ProfileArea::kDataObjects, "*"), 0);
}

TEST(ProfileAccessorTest, RemovingSomethingAbsentSucceeds) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kDataObjects));

  // A clear-out that stops on the first missing file would leave the rest
  // behind, so absence is success.
  EXPECT_TRUE(accessor.Remove(ProfileArea::kDataObjects, "never-there"));

  ASSERT_TRUE(accessor.Write(ProfileArea::kDataObjects, "abcd",
                             GFBuffer(QByteArray("x"))));
  EXPECT_TRUE(accessor.Remove(ProfileArea::kDataObjects, "abcd"));
  EXPECT_FALSE(accessor.Exists(ProfileArea::kDataObjects, "abcd"));
}

TEST(ProfileAccessorTest, ListingAndSizingRespectThePattern) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kLogs));

  ASSERT_TRUE(accessor.Write(ProfileArea::kLogs, "a.log",
                             GFBuffer(QByteArray("1234567890"))));
  ASSERT_TRUE(accessor.Write(ProfileArea::kLogs, "b.log",
                             GFBuffer(QByteArray("12345"))));
  ASSERT_TRUE(accessor.Write(ProfileArea::kLogs, "notes.txt",
                             GFBuffer(QByteArray("ignored"))));

  auto logs = accessor.List(ProfileArea::kLogs, "*.log");
  logs.sort();
  EXPECT_EQ(logs, QStringList({"a.log", "b.log"}));

  EXPECT_EQ(accessor.TotalSize(ProfileArea::kLogs, "*.log"), 15);
  EXPECT_EQ(accessor.List(ProfileArea::kLogs, "*").size(), 3);
}

TEST(ProfileAccessorTest, AreasDoNotSeeEachOther) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  FsProfileAccessor accessor(dir.path(), {});
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kDataObjects));
  ASSERT_TRUE(accessor.Ensure(ProfileArea::kSecure));

  ASSERT_TRUE(accessor.Write(ProfileArea::kSecure, "app.key",
                             GFBuffer(QByteArray("key material"))));

  // The closed set of areas is what stops one part of the profile reaching
  // into another by assembling a path.
  EXPECT_FALSE(accessor.Exists(ProfileArea::kDataObjects, "app.key"));
  EXPECT_TRUE(accessor.List(ProfileArea::kDataObjects, "*").isEmpty());
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

}  // namespace GpgFrontend::Test
