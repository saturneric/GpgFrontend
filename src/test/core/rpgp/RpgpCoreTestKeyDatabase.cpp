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
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <atomic>
#include <optional>
#include <thread>
#include <vector>

#include "GpgFrontendTest.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/model/GpgKey.h"

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

namespace {

auto WithSubkey(GFKey key, const QString& subkey_fpr) -> GFKey {
  GFSubKeyMetadata sub;
  sub.fpr = subkey_fpr;
  sub.key_id = subkey_fpr.right(16);
  key.metadata.subkeys.append(sub);
  return key;
}

const QString kSharedSubkey = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
const QString kSquatter = "1111111111111111111111111111111111111111";
const QString kRealKey = "2222222222222222222222222222222222222222";

}  // namespace

// A subkey's fingerprint was the whole key of its row, so whichever
// certificate claimed a subkey first owned it: the real certificate then
// failed to save, and a lookup by the subkey found the squatter.
TEST(RpgpKeyDatabaseTest, TwoCertificatesMayCarryOneSubkey) {
  ScratchKeyDatabase scratch;
  ASSERT_TRUE(scratch.initialized);

  auto squatter = WithSubkey(FakeKey(kSquatter, "squatter"), kSharedSubkey);
  auto real = WithSubkey(FakeKey(kRealKey, "real"), kSharedSubkey);
  real.metadata.has_secret = true;

  ASSERT_TRUE(scratch.db->SaveKey(squatter.metadata, squatter.blocks));
  ASSERT_TRUE(scratch.db->SaveKey(real.metadata, real.blocks))
      << "the real key could not be saved after a squatter";
  EXPECT_EQ(scratch.db->ResolvePrimaryFpr(kSharedSubkey), kRealKey);
}

// The key pairs carrying one key are all named, under its fingerprint and
// its key ID; a key only one key pair carries, and an ADSK, are not.
TEST(RpgpKeyDatabaseTest, KeysSharedByKeyPairsAreIndexed) {
  const QString other_subkey = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
  const QString adsk_holder = "3333333333333333333333333333333333333333";
  const QString primary_claimer = "4444444444444444444444444444444444444444";

  auto as_key = [](GFKey key) {
    // The primary is listed first, as the engines list it.
    GFSubKeyMetadata primary;
    primary.fpr = key.metadata.fpr;
    primary.key_id = key.metadata.key_id;
    key.metadata.subkeys.prepend(primary);
    return QSharedPointer<GpgKey>::create(key.metadata);
  };

  // Listed with the higher fingerprint first: the index sorts its owners.
  auto real = as_key(WithSubkey(FakeKey(kRealKey, "real"), kSharedSubkey));
  auto squatter = as_key(WithSubkey(
      WithSubkey(FakeKey(kSquatter, "squatter"), kSharedSubkey), kRealKey));
  auto lone = as_key(
      WithSubkey(FakeKey(kRealKey.left(39) + "3", "lone"), other_subkey));
  auto adsk = WithSubkey(FakeKey(adsk_holder, "adsk"), kSharedSubkey);
  adsk.metadata.subkeys.last().can_renc = true;
  auto claimer =
      as_key(WithSubkey(FakeKey(primary_claimer, "claimer"), kRealKey));

  const auto shared =
      IndexKeysSharedByKeyPairs({real, squatter, lone, as_key(adsk), claimer});

  auto fprs = [](const GpgKeyPtrList& keys) {
    QStringList ret;
    for (const auto& key : keys) ret << key->Fingerprint();
    return ret;
  };

  EXPECT_EQ(fprs(shared.value(kSharedSubkey)),
            QStringList({kSquatter, kRealKey}))
      << "an ADSK is another holder's key, not a shared one";
  EXPECT_EQ(fprs(shared.value(kSharedSubkey.right(16))),
            QStringList({kSquatter, kRealKey}));
  EXPECT_EQ(fprs(shared.value(kRealKey)),
            QStringList({kSquatter, kRealKey, primary_claimer}))
      << "a primary claimed as a subkey elsewhere is shared too";
  EXPECT_FALSE(shared.contains(other_subkey));
  EXPECT_FALSE(shared.contains(kSquatter));
}

// A database written by the previous schema is rebuilt with the composite key
// and keeps its rows.
TEST(RpgpKeyDatabaseTest, AVersion3DatabaseIsMigrated) {
  ScratchKeyDatabase scratch;
  ASSERT_TRUE(scratch.initialized);
  auto squatter = WithSubkey(FakeKey(kSquatter, "squatter"), kSharedSubkey);
  ASSERT_TRUE(scratch.db->SaveKey(squatter.metadata, squatter.blocks));
  scratch.db.reset();

  const auto file = scratch.home.canonicalPath() + "/gf_keydb.sqlite";
  {
    auto sql = QSqlDatabase::addDatabase("QSQLITE", "v3-downgrade");
    sql.setDatabaseName(file);
    ASSERT_TRUE(sql.open());
    QSqlQuery query(sql);
    for (const auto* step : {
             "CREATE TABLE subkey_v3 (fpr TEXT PRIMARY KEY COLLATE NOCASE, "
             "parent_fpr TEXT NOT NULL COLLATE NOCASE, key_id TEXT NOT NULL "
             "COLLATE NOCASE, algo INTEGER, key_ver INTEGER NOT NULL DEFAULT "
             "0, created_at INTEGER, expires_at INTEGER DEFAULT 0, key_length "
             "INTEGER DEFAULT 0, has_secret INTEGER DEFAULT 0, is_revoked "
             "INTEGER DEFAULT 0, can_sign INTEGER DEFAULT 0, can_encrypt "
             "INTEGER DEFAULT 0, can_auth INTEGER DEFAULT 0, can_certify "
             "INTEGER DEFAULT 0)",
             "INSERT INTO subkey_v3 SELECT fpr, parent_fpr, key_id, algo, "
             "key_ver, created_at, expires_at, key_length, has_secret, "
             "is_revoked, can_sign, can_encrypt, can_auth, can_certify FROM "
             "subkey_metadata",
             "DROP TABLE subkey_metadata",
             "ALTER TABLE subkey_v3 RENAME TO subkey_metadata",
             "PRAGMA user_version = 3",
         }) {
      ASSERT_TRUE(query.exec(step)) << query.lastError().text().toStdString();
    }
    query.finish();
    sql.close();
  }
  QSqlDatabase::removeDatabase("v3-downgrade");

  scratch.db.emplace();
  ASSERT_TRUE(scratch.db->Init(scratch.home.canonicalPath()));
  EXPECT_EQ(scratch.db->GetSchemaVersion(), 4);
  EXPECT_EQ(scratch.db->ResolvePrimaryFpr(kSharedSubkey), kSquatter);

  auto real = WithSubkey(FakeKey(kRealKey, "real"), kSharedSubkey);
  EXPECT_TRUE(scratch.db->SaveKey(real.metadata, real.blocks));
}

namespace {

/// Pretends a newer release wrote the database in @p home: its schema is
/// @p version and it names @p min_reader (none when empty) as the oldest
/// schema that may still read it.
void PretendNewerRelease(const QString& home, int version,
                         const QString& min_reader) {
  const auto connection = u"newer-release"_s;
  {
    auto sql = QSqlDatabase::addDatabase("QSQLITE", connection);
    sql.setDatabaseName(home + "/gf_keydb.sqlite");
    ASSERT_TRUE(sql.open());
    QSqlQuery query(sql);
    ASSERT_TRUE(query.exec(u"PRAGMA user_version = %1"_s.arg(version)));
    ASSERT_TRUE(query.exec("DELETE FROM gf_meta"));
    if (!min_reader.isEmpty()) {
      query.prepare("INSERT INTO gf_meta (name, value) VALUES (:n, :v)");
      query.bindValue(":n", u"min_reader_schema"_s);
      query.bindValue(":v", min_reader);
      ASSERT_TRUE(query.exec()) << query.lastError().text().toStdString();
    }
    query.finish();
    sql.close();
  }
  QSqlDatabase::removeDatabase(connection);
}

}  // namespace

// Every database this release writes says which schema may still read it, so
// an older release meeting a newer database can tell whether to open it.
TEST(RpgpKeyDatabaseTest, ADatabaseRecordsTheOldestSchemaThatMayReadIt) {
  ScratchKeyDatabase scratch;
  ASSERT_TRUE(scratch.initialized);
  EXPECT_EQ(scratch.db->GetMeta("min_reader_schema"), u"3"_s);
  EXPECT_FALSE(GFKeyDatabase::IsFromNewerRelease(scratch.home.canonicalPath()));
}

// v2.2.4 refused to start on a database a newer build had upgraded. A newer
// database that admits this schema opens, and is left as the newer release
// wrote it.
TEST(RpgpKeyDatabaseTest, ANewerDatabaseThatAdmitsUsOpensUnchanged) {
  ScratchKeyDatabase scratch;
  ASSERT_TRUE(scratch.initialized);
  auto squatter = WithSubkey(FakeKey(kSquatter, "squatter"), kSharedSubkey);
  ASSERT_TRUE(scratch.db->SaveKey(squatter.metadata, squatter.blocks));
  scratch.db.reset();

  const auto home = scratch.home.canonicalPath();
  PretendNewerRelease(home, 99, u"4"_s);
  EXPECT_FALSE(GFKeyDatabase::IsFromNewerRelease(home));

  scratch.db.emplace();
  ASSERT_TRUE(scratch.db->Init(home));
  EXPECT_EQ(scratch.db->ResolvePrimaryFpr(kSharedSubkey), kSquatter);
  auto real = WithSubkey(FakeKey(kRealKey, "real"), kSharedSubkey);
  EXPECT_TRUE(scratch.db->SaveKey(real.metadata, real.blocks));

  EXPECT_EQ(scratch.db->GetSchemaVersion(), 99) << "never lowered";
  EXPECT_EQ(scratch.db->GetMeta("min_reader_schema"), u"4"_s)
      << "the newer release's marker is its own";
}

// A newer database that does not admit this schema, or says nothing, stays
// closed, and the startup can tell why.
TEST(RpgpKeyDatabaseTest, ANewerDatabaseThatDoesNotAdmitUsStaysClosed) {
  for (const auto& min_reader : {QString(), u"99"_s, u"junk"_s}) {
    ScratchKeyDatabase scratch;
    ASSERT_TRUE(scratch.initialized);
    scratch.db.reset();

    const auto home = scratch.home.canonicalPath();
    PretendNewerRelease(home, 99, min_reader);
    EXPECT_TRUE(GFKeyDatabase::IsFromNewerRelease(home))
        << min_reader.toStdString();

    scratch.db.emplace();
    EXPECT_FALSE(scratch.db->Init(home)) << min_reader.toStdString();
  }
}

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

// What the database keeps about itself survives closing it.
TEST(RpgpKeyDatabaseTest, ItKeepsValuesAboutItself) {
  QDir home{QDir::tempPath() + "/" + GenerateRandomString(12)};
  home.mkpath(".");
  {
    GFKeyDatabase db;
    ASSERT_TRUE(db.Init(home.canonicalPath()));
    EXPECT_TRUE(db.GetMeta("anything").isEmpty());
    EXPECT_TRUE(db.SetMeta("anything", "one"));
    EXPECT_TRUE(db.SetMeta("anything", "two"));
    EXPECT_EQ(db.GetMeta("anything"), "two");
  }
  {
    GFKeyDatabase db;
    ASSERT_TRUE(db.Init(home.canonicalPath()));
    EXPECT_EQ(db.GetMeta("anything"), "two");
  }
  home.removeRecursively();
}

// Every key's metadata is derived again once per build, not on every start:
// a thousand keys made the phone wait a quarter of a minute each time.
TEST(RpgpKeyDatabaseTest, KeyMetadataIsRefreshedOncePerBuild) {
  ScratchKeyDatabase scratch;
  ASSERT_TRUE(scratch.initialized);
  const QString fpr(40, QChar('B'));
  const auto key = FakeKey(fpr, "not a key");
  ASSERT_TRUE(scratch.db->SaveKey(key.metadata, key.blocks));

  EXPECT_EQ(RefreshKeyMetadataIfStale(*scratch.db), 1);
  EXPECT_EQ(scratch.db->GetMeta(kKeyMetadataStampName), KeyMetadataStamp());

  // The same build again: nothing to do.
  EXPECT_EQ(RefreshKeyMetadataIfStale(*scratch.db), 0);

  // Another build was here since: done again.
  ASSERT_TRUE(scratch.db->SetMeta(kKeyMetadataStampName, "another build"));
  EXPECT_EQ(RefreshKeyMetadataIfStale(*scratch.db), 1);
  EXPECT_EQ(RefreshKeyMetadataIfStale(*scratch.db), 0);
}

}  // namespace GpgFrontend::Test
