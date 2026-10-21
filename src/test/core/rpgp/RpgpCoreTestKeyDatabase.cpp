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
#include <QFileInfo>
#include <atomic>
#include <optional>
#include <thread>
#include <vector>

#include "GpgFrontendTest.h"
#include "core/function/GFKeyDatabase.h"

namespace GpgFrontend::Test {

// The key database stores key material (including secret keys) in plaintext, so
// Init() must lock down the storage directory and database file with OS
// permissions by default: directory owner-only (0700), file owner rw (0600).
TEST(RpgpKeyDatabaseTest, InitRestrictsStoragePermissions) {
  auto db_home = QDir(QDir::tempPath() + "/" + GenerateRandomString(12));
  if (db_home.exists()) db_home.removeRecursively();
  ASSERT_TRUE(db_home.mkpath("."));

  GFKeyDatabase db;
  ASSERT_TRUE(db.Init(db_home.canonicalPath()));

  const auto dir_perms = QFile(db_home.canonicalPath()).permissions();
  // Owner keeps full access.
  EXPECT_TRUE(dir_perms.testFlag(QFileDevice::ReadOwner));
  EXPECT_TRUE(dir_perms.testFlag(QFileDevice::WriteOwner));
  EXPECT_TRUE(dir_perms.testFlag(QFileDevice::ExeOwner));
  // Group and other are denied any access.
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ReadGroup));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::WriteGroup));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ExeGroup));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ReadOther));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::WriteOther));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ExeOther));

  const auto db_file = db_home.canonicalPath() + "/gf_keydb.sqlite";
  ASSERT_TRUE(QFileInfo::exists(db_file));

  const auto file_perms = QFile(db_file).permissions();
  // Owner keeps read/write, everyone else is denied.
  EXPECT_TRUE(file_perms.testFlag(QFileDevice::ReadOwner));
  EXPECT_TRUE(file_perms.testFlag(QFileDevice::WriteOwner));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::ReadGroup));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::WriteGroup));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::ReadOther));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::WriteOther));

  db_home.removeRecursively();
}

namespace {

auto FakeKey(const QString& fpr, const QString& block) -> GFKey {
  GFKey key;
  key.metadata.fpr = fpr;
  key.metadata.key_id = fpr.right(16);
  key.metadata.algo = 22;
  key.metadata.key_ver = 4;
  key.blocks.public_key = GFBuffer(block);
  return key;
}

/// A fresh database in a throwaway directory.
struct ScratchKeyDatabase {
  QDir home{QDir::tempPath() + "/" + GenerateRandomString(12)};
  std::optional<GFKeyDatabase> db;
  bool initialized = false;

  ScratchKeyDatabase() {
    home.mkpath(".");
    db.emplace();
    initialized = db->Init(home.canonicalPath());
  }
  ~ScratchKeyDatabase() {
    db.reset();  // closed before its files are removed
    home.removeRecursively();
  }
};

}  // namespace

// The database is shared by the GUI thread, the task runners and module
// threads. A QSqlDatabase connection may only be used by the thread that
// created it, and QSQLITE opens SQLite without its own mutex: every caller,
// from any thread, must still get correct answers.
TEST(RpgpKeyDatabaseTest, CallersOnManyThreadsAllGetCorrectAnswers) {
  ScratchKeyDatabase scratch;
  ASSERT_TRUE(scratch.initialized);

  constexpr int kThreads = 8;
  constexpr int kKeysPerThread = 25;
  std::atomic<int> failures{0};

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < kKeysPerThread; ++i) {
        const auto fpr =
            QString("%1").arg(t * 1000 + i, 40, 16, QChar('0')).toUpper();
        const auto block = QString("block-%1").arg(fpr);
        const auto key = FakeKey(fpr, block);
        if (!scratch.db->SaveKey(key.metadata, key.blocks)) ++failures;

        const auto read = scratch.db->GetKeyByIdentifier(fpr);
        if (!read || read->blocks.public_key != GFBuffer(block)) ++failures;

        scratch.db->GetMetadataList();
      }
    });
  }
  for (auto& thread : threads) thread.join();

  EXPECT_EQ(failures.load(), 0);
  EXPECT_EQ(scratch.db->GetMetadataList().size(), kThreads * kKeysPerThread);
}

// Read, compute and write as one unit: two threads updating the same key
// through RunExclusive() never lose each other's update.
TEST(RpgpKeyDatabaseTest, ExclusiveUnitsDoNotLoseUpdates) {
  ScratchKeyDatabase scratch;
  ASSERT_TRUE(scratch.initialized);

  const QString fpr(40, QChar('A'));
  const auto initial = FakeKey(fpr, "0");
  ASSERT_TRUE(scratch.db->SaveKey(initial.metadata, initial.blocks));

  constexpr int kThreads = 4;
  constexpr int kIncrements = 50;
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&]() {
      for (int i = 0; i < kIncrements; ++i) {
        scratch.db->RunExclusive([&]() {
          const auto current = scratch.db->GetKeyByIdentifier(fpr);
          if (!current) return false;
          const auto next =
              current->blocks.public_key.ConvertToQString().toInt() + 1;
          const auto key = FakeKey(fpr, QString::number(next));
          return scratch.db->SaveKey(key.metadata, key.blocks);
        });
      }
    });
  }
  for (auto& thread : threads) thread.join();

  const auto final_key = scratch.db->GetKeyByIdentifier(fpr);
  ASSERT_TRUE(final_key.has_value());
  EXPECT_EQ(final_key->blocks.public_key.ConvertToQString().toInt(),
            kThreads * kIncrements);
}

}  // namespace GpgFrontend::Test
