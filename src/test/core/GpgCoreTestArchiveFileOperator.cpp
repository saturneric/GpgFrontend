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
#include <QSharedPointer>
#include <QTemporaryDir>

#include "core/function/ArchiveFileOperator.h"

namespace {

void CreateTestFile(const QString& dir, const QString& name,
                    const QByteArray& content) {
  QFile f(dir + "/" + name);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write(content);
  f.close();
}

}  // namespace

namespace GpgFrontend::Test {

TEST(ArchiveFileOperatorTest, ArchiveAndExtract) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  QString src_dir = temp_dir.path() + "/src";
  QDir().mkpath(src_dir);

  CreateTestFile(src_dir, "test1.txt", "hello world 1");
  CreateTestFile(src_dir, "test2.txt", "hello world 2");

  auto exchanger = CreateStandardGFDataExchanger();
  bool archive_finished = false;

  GpgFrontend::ArchiveFileOperator::NewArchive2DataExchanger(
      src_dir, exchanger, [&](GFError err, DataObjectPtr) {
        EXPECT_EQ(err, 0);
        archive_finished = true;
      });

  WAIT_FOR_TRUE(archive_finished, 3000);

  QTemporaryDir extract_dir;
  ASSERT_TRUE(extract_dir.isValid());
  bool extract_finished = false;
  GpgFrontend::ArchiveFileOperator::ExtractArchiveFromDataExchanger(
      exchanger, extract_dir.path(), [&](GFError err, DataObjectPtr) {
        EXPECT_EQ(err, 0);
        extract_finished = true;
      });
  WAIT_FOR_TRUE(extract_finished, 3000);

  QFile f1(extract_dir.path() + "/test1.txt");
  QFile f2(extract_dir.path() + "/test2.txt");
  ASSERT_TRUE(f1.open(QIODevice::ReadOnly));
  ASSERT_TRUE(f2.open(QIODevice::ReadOnly));
  EXPECT_EQ(f1.readAll(), QByteArray("hello world 1"));
  EXPECT_EQ(f2.readAll(), QByteArray("hello world 2"));
}

TEST(ArchiveFileOperatorTest, ListArchive) {
  GpgFrontend::ArchiveFileOperator::ListArchive("/tmp/archive.tar");
}

TEST(ArchiveFileOperatorTest, HandleInvalidInput) {
  auto exchanger = CreateStandardGFDataExchanger();
  bool archive_finished = false;
  GpgFrontend::ArchiveFileOperator::NewArchive2DataExchanger(
      "/not/exist", exchanger, [&](int err, DataObjectPtr) {
        EXPECT_NE(err, 0);
        archive_finished = true;
      });
  WAIT_FOR_TRUE(archive_finished, 3000);

  QTemporaryDir temp_dir;
  bool extract_finished = false;
  GpgFrontend::ArchiveFileOperator::ExtractArchiveFromDataExchanger(
      exchanger, temp_dir.path(), [&](int err, DataObjectPtr) {
        EXPECT_EQ(err, 0);
        extract_finished = true;
      });
  WAIT_FOR_TRUE(extract_finished, 3000);
}
}  // namespace GpgFrontend::Test
