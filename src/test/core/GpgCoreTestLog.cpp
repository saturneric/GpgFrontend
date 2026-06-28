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

#include "core/GFCoreLog.h"

namespace GpgFrontend::Test {

namespace {

auto MakeEntry(const QString& message) -> GFLogEntry {
  GFLogEntry entry;
  entry.timestamp = QDateTime::currentDateTime();
  entry.type = QtInfoMsg;
  entry.formatted_message = message;
  entry.raw_message = message;
  return entry;
}

auto ReadAll(const QString& path) -> QString {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
  return QString::fromUtf8(f.readAll());
}

}  // namespace

TEST(GFLogFileTest, FlushesRingBacklogThenWritesNewEntries) {
  auto& mgr = GFLogManager::Instance();
  mgr.StopFileLogger();
  mgr.InitRingBuffer(1024);

  // pushed before file logging is enabled: only lands in the ring buffer
  mgr.Push(MakeEntry("BACKLOG_LINE"));

  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  mgr.InitFileLogger(tmp.path());

  const QString active = QDir(tmp.path()).absoluteFilePath("gpgfrontend.log");
  ASSERT_TRUE(QFile::exists(active));

  // the backlog must have been flushed into the freshly opened file
  EXPECT_TRUE(ReadAll(active).contains("BACKLOG_LINE"));

  // entries pushed after enabling are appended (and flushed immediately)
  mgr.Push(MakeEntry("LIVE_LINE"));
  EXPECT_TRUE(ReadAll(active).contains("LIVE_LINE"));

  mgr.StopFileLogger();
}

TEST(GFLogFileTest, RotatesWhenSizeExceedsThresholdAndCapsBackups) {
  auto& mgr = GFLogManager::Instance();
  mgr.StopFileLogger();
  mgr.InitRingBuffer(1024);

  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());

  // tiny threshold + few backups so a handful of writes forces rotation
  mgr.InitFileLogger(tmp.path(), /*max_file_bytes=*/256, /*max_files=*/2);

  const QDir dir(tmp.path());
  const QString line(120, QChar('x'));  // each write comfortably > threshold/2
  for (int i = 0; i < 50; ++i) {
    mgr.Push(MakeEntry(QString("ROT_%1_%2").arg(i).arg(line)));
  }

  // at least one rotated backup exists
  EXPECT_TRUE(QFile::exists(dir.absoluteFilePath("gpgfrontend.1.log")));
  // backups never exceed max_files (no .3 when max_files == 2)
  EXPECT_FALSE(QFile::exists(dir.absoluteFilePath("gpgfrontend.3.log")));
  // the active file is always present and below the rotation threshold
  EXPECT_TRUE(QFile::exists(dir.absoluteFilePath("gpgfrontend.log")));

  mgr.StopFileLogger();
}

TEST(GFLogFileTest, InitIsNoOpForEmptyDirAndWhileAlreadyOpen) {
  auto& mgr = GFLogManager::Instance();
  mgr.StopFileLogger();
  mgr.InitRingBuffer(1024);

  // empty directory: nothing is opened, push must not crash
  mgr.InitFileLogger(QString{});
  mgr.Push(MakeEntry("NO_FILE_LINE"));

  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  mgr.InitFileLogger(tmp.path());
  const QString active = QDir(tmp.path()).absoluteFilePath("gpgfrontend.log");
  ASSERT_TRUE(QFile::exists(active));

  // second init while already logging is a no-op (does not retarget)
  QTemporaryDir tmp2;
  ASSERT_TRUE(tmp2.isValid());
  mgr.InitFileLogger(tmp2.path());
  EXPECT_FALSE(
      QFile::exists(QDir(tmp2.path()).absoluteFilePath("gpgfrontend.log")));

  mgr.StopFileLogger();
}

}  // namespace GpgFrontend::Test
