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

#include "platform/common/PlatformFs.h"

namespace GpgFrontend::Test {

// The portable half of the platform layer, and the parts of the OS half whose
// answer this machine can be asked for without mounting or touching anything
// outside a temporary directory.

TEST(PlatformFsTest, NearestExistingAncestorOfAnExistingPathIsItself) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  EXPECT_EQ(Platform::NearestExistingAncestor(dir.path()),
            QDir::cleanPath(dir.path()));
}

TEST(PlatformFsTest, NearestExistingAncestorWalksUpPastMissingLevels) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  EXPECT_EQ(Platform::NearestExistingAncestor(dir.path() + "/not/yet/here"),
            QDir::cleanPath(dir.path()));
}

TEST(PlatformFsTest, NearestExistingAncestorOfEmptyIsEmpty) {
  EXPECT_TRUE(Platform::NearestExistingAncestor({}).isEmpty());
}

TEST(PlatformFsTest, StatRegularFileRefusesWhatIsNotARegularFile) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  EXPECT_EQ(Platform::StatRegularFile(dir.path()).size, -1);
  EXPECT_EQ(Platform::StatRegularFile(dir.path() + "/missing").size, -1);
}

TEST(PlatformFsTest, StatRegularFileReportsSizeAndStaysStable) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("file");
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("twelve bytes");
  file.close();

  const auto first = Platform::StatRegularFile(path);
  EXPECT_EQ(first.size, 12);
  EXPECT_GE(first.modified_ms, 0);

  const auto second = Platform::StatRegularFile(path);
  EXPECT_EQ(first.size, second.size);
  EXPECT_EQ(first.modified_ms, second.modified_ms);
  EXPECT_EQ(first.file_id, second.file_id);

#ifdef Q_OS_UNIX
  EXPECT_NE(first.file_id, 0U);
#endif
}

TEST(PlatformFsTest, LockedMemoryLimitIsALimitOrUnknown) {
  EXPECT_GE(Platform::LockedMemoryLimit(), -1);
}

TEST(PlatformFsTest, DuplicateFdGivesAnIndependentDescriptor) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QFile file(dir.filePath("file"));
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));

  const auto copy = Platform::DuplicateFd(file.handle());
  ASSERT_GE(copy, 0);
  EXPECT_NE(copy, file.handle());
  EXPECT_EQ(Platform::CloseFd(copy), 0);

  // The original is untouched by closing the copy.
  EXPECT_EQ(file.write("still open"), 10);
}

TEST(PlatformFsTest, ExcludeFromIndexAndBackupLeavesTheDirectoryUsable) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  Platform::ExcludeFromIndexAndBackup(dir.path());

  QFile file(dir.filePath("after"));
  EXPECT_TRUE(file.open(QIODevice::WriteOnly));
}

#ifdef Q_OS_UNIX

TEST(PlatformFsTest, ClaimPrivateDirectoryCreatesOwnerOnly) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("claimed");
  QString reason;
  ASSERT_TRUE(Platform::ClaimPrivateDirectory(path, reason))
      << reason.toStdString();

  EXPECT_EQ(QFile::permissions(path) &
                (QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
                 QFile::ReadOther | QFile::WriteOther | QFile::ExeOther),
            QFile::Permissions{});

  // Claimed again, it is already ours.
  EXPECT_TRUE(Platform::ClaimPrivateDirectory(path, reason))
      << reason.toStdString();
}

TEST(PlatformFsTest, ClaimPrivateDirectoryRefusesAnOpenDirectory) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto path = dir.filePath("open");
  ASSERT_TRUE(QDir().mkdir(path));
  ASSERT_TRUE(QFile::setPermissions(
      path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther |
                QFile::ExeOther));

  QString reason;
  EXPECT_FALSE(Platform::ClaimPrivateDirectory(path, reason));
  EXPECT_FALSE(reason.isEmpty());
}

TEST(PlatformFsTest, ClaimPrivateDirectoryRefusesASymlink) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  const auto target = dir.filePath("target");
  QString reason;
  ASSERT_TRUE(Platform::ClaimPrivateDirectory(target, reason));

  const auto link = dir.filePath("link");
  ASSERT_TRUE(QFile::link(target, link));

  EXPECT_FALSE(Platform::ClaimPrivateDirectory(link, reason));
}

#endif

}  // namespace GpgFrontend::Test
