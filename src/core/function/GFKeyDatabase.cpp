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

#include "GFKeyDatabase.h"

#include <QSqlError>
#include <QSqlQuery>

namespace GpgFrontend {

namespace {

auto PrimaryKeyAsSubKey(const GFKeyMetadata& meta) -> GFSubKeyMetadata {
  GFSubKeyMetadata sub;
  sub.marked = false;
  sub.fpr = meta.fpr;
  sub.key_id = meta.key_id;
  sub.algo = meta.algo;
  sub.created_at = meta.created_at;
  sub.has_secret = meta.has_secret;
  sub.can_sign = meta.can_sign;
  sub.can_encrypt = meta.can_encrypt;
  sub.can_auth = meta.can_auth;
  sub.can_certify = meta.can_certify;
  sub.key_length = meta.key_length;
  sub.is_revoked = false;  // primary key itself is not revoked, even if all
                           // its user IDs are revoked
  return sub;
}
}  // namespace

auto GFKeyDatabase::connect_db(const QString& path) -> bool {
  db_ = QSqlDatabase::addDatabase("QSQLITE");

  db_.setDatabaseName(path);

  if (!db_.open()) {
    LOG_E() << "cannot open key database at path: " << path;
    return false;
  }

  return true;
}

auto GFKeyDatabase::GetMetadataList() -> QContainer<GFKeyMetadata> {
  QContainer<GFKeyMetadata> list;
  QSqlQuery query(db_);

  if (query.exec(R"(
        SELECT fpr, key_id, algo, created_at, has_secret, is_revoked, key_length,
               can_sign, can_encrypt, can_auth, can_certify, update_time
        FROM key_metadata
      )")) {
    while (query.next()) {
      GFKeyMetadata meta;
      meta.fpr = query.value(0).toString();
      meta.key_id = query.value(1).toString();
      meta.algo = query.value(2).toInt();
      meta.created_at = query.value(3).toLongLong();
      meta.has_secret = query.value(4).toBool();
      meta.is_revoked = query.value(5).toBool();
      meta.key_length = static_cast<unsigned int>(query.value(6).toUInt());
      meta.can_sign = query.value(7).toBool();
      meta.can_encrypt = query.value(8).toBool();
      meta.can_auth = query.value(9).toBool();
      meta.can_certify = query.value(10).toBool();
      meta.update_time =
          QDateTime::fromString(query.value(11).toString(), Qt::ISODate)
              .toSecsSinceEpoch();

      // Load user IDs for this primary key
      meta.user_ids = load_user_ids_for_parent(meta.fpr);

      // Load subkeys for this primary key
      meta.subkeys = load_subkeys_for_parent(meta.fpr);

      // In gnupg keyring, the primary key is also represented as the first
      // subkey, so we need to add it to the subkeys list when returning the
      // metadata
      meta.subkeys.prepend(PrimaryKeyAsSubKey(meta));

      list.append(meta);
    }
  } else {
    LOG_E() << "failed to execute GetMetadataList query: "
            << query.lastError().text();
  }

  return list;
}

auto GFKeyDatabase::GetKeyBlocks(const QString& identifier)
    -> std::optional<GFKeyBlocks> {
  QString real_primary_fpr = ResolvePrimaryFpr(identifier);

  if (real_primary_fpr.isEmpty()) {
    return std::nullopt;
  }

  QSqlQuery query(db_);
  query.prepare(
      "SELECT public_key, secret_key FROM key_blocks WHERE fpr = :fpr");
  query.bindValue(":fpr", real_primary_fpr);

  if (query.exec() && query.next()) {
    GFKeyBlocks blocks;
    blocks.public_key = query.value(0).toString();
    blocks.secret_key = query.value(1).toString();
    return blocks;
  }

  return std::nullopt;
}

auto GFKeyDatabase::GetKeyByIdentifier(const QString& identifier)
    -> std::optional<GFKey> {
  QString real_primary_fpr = ResolvePrimaryFpr(identifier);
  if (real_primary_fpr.isEmpty()) {
    return std::nullopt;
  }

  auto meta = GetKeyMetadata(real_primary_fpr);
  if (!meta) {
    return std::nullopt;
  }

  auto blocks = GetKeyBlocks(real_primary_fpr);
  if (!blocks) {
    return std::nullopt;
  }

  return GFKey{.metadata = *meta, .blocks = *blocks};
}

auto GFKeyDatabase::Init(const QString& path) -> bool {
  QFileInfo db_home_info(path);
  if (!db_home_info.exists()) {
    LOG_E() << "key database home does not exist at path: " << path;
    return false;
  }

  // create a directory for the database if it does not exist
  QDir dir = db_home_info.absoluteDir();
  if (!dir.exists()) {
    if (!dir.mkpath(".")) {
      LOG_E() << "failed to create directory for key database at path: "
              << path;
      return false;
    }
  }

  LOG_I() << "created directory for key database at path: " << path;

  // target database file is under the created directory
  QFileInfo key_db_file_info(path + "/keys.db");

  return connect_db(key_db_file_info.absoluteFilePath()) && create_table();
}

auto GFKeyDatabase::SaveKey(const GFKeyMetadata& meta,
                            const GFKeyBlocks& blocks) -> bool {
  if (!db_.transaction()) {
    return false;
  }

  QSqlQuery query(db_);

  // 1. Save Primary Key Metadata
  query.prepare(R"(
    INSERT OR REPLACE INTO key_metadata 
    (fpr, key_id, algo, created_at, has_secret, is_revoked, key_length, can_sign, can_encrypt, can_auth, can_certify, update_time)
    VALUES (:fpr, :key_id, :algo, :created_at, :has_secret, :is_revoked, :key_length, :can_sign, :can_encrypt, :can_auth, :can_certify, :update_time)
  )");
  query.bindValue(":fpr", meta.fpr.toUpper());
  query.bindValue(":key_id", meta.key_id.toUpper());
  query.bindValue(":algo", meta.algo);
  query.bindValue(":created_at", meta.created_at);
  query.bindValue(":has_secret", meta.has_secret ? 1 : 0);
  query.bindValue(":is_revoked", meta.is_revoked ? 1 : 0);
  query.bindValue(":key_length", meta.key_length);
  query.bindValue(":can_sign", meta.can_sign ? 1 : 0);
  query.bindValue(":can_encrypt", meta.can_encrypt ? 1 : 0);
  query.bindValue(":can_auth", meta.can_auth ? 1 : 0);
  query.bindValue(":can_certify", meta.can_certify ? 1 : 0);
  query.bindValue(":update_time",
                  QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

  if (!query.exec()) {
    LOG_E() << "SaveKey meta error: " << query.lastError().text();
    db_.rollback();
    return false;
  }

  // 2. Save User IDs
  QSqlQuery uid_del_query(db_);
  uid_del_query.prepare("DELETE FROM user_ids WHERE fpr = :fpr");
  uid_del_query.bindValue(":fpr", meta.fpr.toUpper());
  uid_del_query.exec();

  QSqlQuery uid_insert_query(db_);
  uid_insert_query.prepare(R"(
    INSERT INTO user_ids (fpr, user_id, is_primary, is_revoked) VALUES (:fpr, :user_id, :is_primary, :is_revoked)
  )");

  for (const auto& uid : meta.user_ids) {
    uid_insert_query.bindValue(":fpr", meta.fpr.toUpper());
    uid_insert_query.bindValue(":user_id", uid.ToString());
    uid_insert_query.bindValue(":is_primary", uid.is_primary ? 1 : 0);
    uid_insert_query.bindValue(":is_revoked", uid.is_revoked ? 1 : 0);
    if (!uid_insert_query.exec()) {
      LOG_E() << "SaveKey user_id error: "
              << uid_insert_query.lastError().text();
      db_.rollback();
      return false;
    }
  }

  // 2. Save Subkeys
  QSqlQuery s_key_del_query(db_);
  s_key_del_query.prepare(
      "DELETE FROM subkey_metadata WHERE parent_fpr = :parent_fpr");
  s_key_del_query.bindValue(":parent_fpr", meta.fpr.toUpper());
  s_key_del_query.exec();

  query.prepare(R"(
    INSERT INTO subkey_metadata 
    (fpr, parent_fpr, key_id, algo, created_at, has_secret, key_length, can_sign, can_encrypt, can_auth, is_revoked)
    VALUES (:fpr, :parent_fpr, :key_id, :algo, :created_at, :has_secret, :key_length, :can_sign, :can_encrypt, :can_auth, :is_revoked)
  )");

  for (const auto& sub : meta.subkeys) {
    if (sub.fpr == meta.fpr) {
      LOG_W() << "subkey fpr is same as primary key fpr: " << sub.fpr
              << ", skipping to save this subkey";
      continue;
    }

    query.bindValue(":fpr", sub.fpr.toUpper());
    query.bindValue(":parent_fpr", meta.fpr.toUpper());
    query.bindValue(":key_id", sub.key_id.toUpper());
    query.bindValue(":algo", sub.algo);
    query.bindValue(":created_at", sub.created_at);
    query.bindValue(":has_secret", sub.has_secret ? 1 : 0);
    query.bindValue(":can_sign", sub.can_sign ? 1 : 0);
    query.bindValue(":can_encrypt", sub.can_encrypt ? 1 : 0);
    query.bindValue(":can_auth", sub.can_auth ? 1 : 0);
    query.bindValue(":can_certify", sub.can_certify ? 1 : 0);
    query.bindValue(":key_length", sub.key_length);
    query.bindValue(":is_revoked", sub.is_revoked ? 1 : 0);

    if (!query.exec()) {
      LOG_E() << "SaveKey subkey error: " << query.lastError().text();
      db_.rollback();
      return false;
    }
  }

  // 3. Save Key Blocks
  query.prepare(R"(
    INSERT OR REPLACE INTO key_blocks (fpr, public_key, secret_key)
    VALUES (:fpr, :public_key, :secret_key)
  )");
  query.bindValue(":fpr", meta.fpr.toUpper());
  query.bindValue(":public_key", blocks.public_key);
  query.bindValue(":secret_key",
                  blocks.secret_key.isEmpty() ? QString{} : blocks.secret_key);

  if (!query.exec()) {
    LOG_E() << "SaveKey blocks error: " << query.lastError().text();
    db_.rollback();
    return false;
  }

  return db_.commit();
}

auto GFKeyDatabase::GetKeyMetadata(const QString& identifier)
    -> std::optional<GFKeyMetadata> {
  // 1. Use the magic resolver to find the true primary fingerprint
  QString real_primary_fpr = ResolvePrimaryFpr(identifier);

  if (real_primary_fpr.isEmpty()) {
    // Key doesn't exist in our database at all
    return std::nullopt;
  }

  // 2. Now securely query the primary key table using the guaranteed primary
  // FPR
  QSqlQuery query(db_);
  query.prepare(R"(
    SELECT fpr, key_id, algo, created_at, has_secret, is_revoked, key_length,
           can_sign, can_encrypt, can_auth, can_certify, update_time
    FROM key_metadata WHERE fpr = :fpr
  )");
  // Bind the resolved fingerprint, not the original identifier!
  query.bindValue(":fpr", real_primary_fpr);

  if (query.exec() && query.next()) {
    GFKeyMetadata meta;
    meta.fpr = query.value(0).toString();
    meta.key_id = query.value(1).toString();
    meta.algo = query.value(2).toInt();
    meta.created_at = query.value(3).toLongLong();
    meta.has_secret = query.value(4).toBool();
    meta.is_revoked = query.value(5).toBool();
    meta.key_length = static_cast<unsigned int>(query.value(6).toUInt());
    meta.can_sign = query.value(7).toBool();
    meta.can_encrypt = query.value(8).toBool();
    meta.can_auth = query.value(9).toBool();
    meta.can_certify = query.value(10).toBool();
    meta.update_time =
        QDateTime::fromString(query.value(11).toString(), Qt::ISODate)
            .toSecsSinceEpoch();

    // Load user IDs
    meta.user_ids = load_user_ids_for_parent(meta.fpr);

    // Load subkeys
    meta.subkeys = load_subkeys_for_parent(meta.fpr);

    // In gnupg keyring, the primary key is also represented as the first
    // subkey, so we need to add it to the subkeys list when returning the
    // metadata
    meta.subkeys.prepend(PrimaryKeyAsSubKey(meta));

    return meta;
  }
  return {};
}

auto GFKeyDatabase::create_table() -> bool {
  QSqlQuery query(db_);

  // 1. Primary key metadata table with new capability fields
  const QString create_metadata_table = R"(
    CREATE TABLE IF NOT EXISTS key_metadata (
      fpr TEXT PRIMARY KEY COLLATE NOCASE,
      key_id TEXT NOT NULL COLLATE NOCASE,
      algo INTEGER,
      key_length INTEGER DEFAULT 0,
      created_at INTEGER,
      has_secret INTEGER DEFAULT 0,
      is_revoked INTEGER DEFAULT 0,
      can_sign INTEGER DEFAULT 0,
      can_encrypt INTEGER DEFAULT 0,
      can_auth INTEGER DEFAULT 0,
      can_certify INTEGER DEFAULT 0,
      update_time DATETIME DEFAULT(datetime('subsec'))
    )
  )";

  // 2. Subkey metadata table (Linked to primary key via parent_fpr)
  const QString create_subkey_table = R"(
    CREATE TABLE IF NOT EXISTS subkey_metadata (
      fpr TEXT PRIMARY KEY COLLATE NOCASE,
      parent_fpr TEXT NOT NULL COLLATE NOCASE,
      key_id TEXT NOT NULL COLLATE NOCASE,
      algo INTEGER,
      created_at INTEGER,
      key_length INTEGER DEFAULT 0,
      has_secret INTEGER DEFAULT 0,
      is_revoked INTEGER DEFAULT 0,
      can_sign INTEGER DEFAULT 0,
      can_encrypt INTEGER DEFAULT 0,
      can_auth INTEGER DEFAULT 0,
      can_certify INTEGER DEFAULT 0,
      FOREIGN KEY(parent_fpr) REFERENCES key_metadata(fpr) ON DELETE CASCADE
    )
  )";

  // 3. Key blocks table
  const QString create_blocks_table = R"(
    CREATE TABLE IF NOT EXISTS key_blocks (
      fpr TEXT PRIMARY KEY COLLATE NOCASE,
      public_key TEXT NOT NULL,
      secret_key TEXT,
      FOREIGN KEY(fpr) REFERENCES key_metadata(fpr) ON DELETE CASCADE
    )
  )";

  // 4. User IDs table (Linked to primary key via fpr)
  const QString create_user_ids_table = R"(
    CREATE TABLE IF NOT EXISTS user_ids (
      fpr TEXT NOT NULL COLLATE NOCASE,
      user_id TEXT NOT NULL,
      is_primary INTEGER DEFAULT 0,
      is_revoked INTEGER DEFAULT 0,
      FOREIGN KEY(fpr) REFERENCES key_metadata(fpr) ON DELETE CASCADE,
      UNIQUE(fpr, user_id)
    )
  )";

  if (!query.exec(create_metadata_table)) {
    LOG_E() << "Failed to create key_metadata table: "
            << query.lastError().text();
    return false;
  }

  if (!query.exec(create_subkey_table)) {
    LOG_E() << "Failed to create subkey_metadata table: "
            << query.lastError().text();
    return false;
  }

  if (!query.exec(create_blocks_table)) {
    LOG_E() << "Failed to create key_blocks table: "
            << query.lastError().text();
    return false;
  }

  if (!query.exec(create_user_ids_table)) {
    LOG_E() << "Failed to create user_ids table: " << query.lastError().text();
    return false;
  }

  return true;
}

auto GFKeyDatabase::load_subkeys_for_parent(const QString& parent_fpr)
    -> QContainer<GFSubKeyMetadata> {
  QContainer<GFSubKeyMetadata> subkeys;
  QSqlQuery query(db_);
  query.prepare(R"(
    SELECT fpr, key_id, algo, created_at, has_secret, is_revoked, key_length, can_sign, can_encrypt, can_auth, can_certify
    FROM subkey_metadata WHERE parent_fpr = :parent_fpr
  )");
  query.bindValue(":parent_fpr", parent_fpr);

  if (query.exec()) {
    while (query.next()) {
      GFSubKeyMetadata sub;
      sub.marked = false;
      sub.fpr = query.value(0).toString();
      sub.key_id = query.value(1).toString();
      sub.algo = query.value(2).toInt();
      sub.created_at = query.value(3).toLongLong();
      sub.has_secret = query.value(4).toBool();
      sub.is_revoked = query.value(5).toBool();
      sub.key_length = static_cast<unsigned int>(query.value(6).toUInt());
      sub.can_sign = query.value(7).toBool();
      sub.can_encrypt = query.value(8).toBool();
      sub.can_auth = query.value(9).toBool();
      sub.can_certify = query.value(10).toBool();
      subkeys.append(sub);
    }
  }
  return subkeys;
}

auto GFKeyDatabase::DeleteKey(const QString& fpr) -> bool {
  auto primary_fpr = ResolvePrimaryFpr(fpr);
  if (primary_fpr.isEmpty()) {
    LOG_W() << "Attempted to delete non-existent key with identifier: " << fpr;
    return false;
  }

  QSqlQuery query(db_);
  query.prepare("DELETE FROM key_metadata WHERE fpr = :fpr");
  query.bindValue(":fpr", primary_fpr);

  return query.exec();
}

auto GFKeyDatabase::ResolvePrimaryFpr(const QString& identifier) -> QString {
  // 1. Clean up the identifier (remove spaces and force uppercase)
  QString clean_id = identifier.simplified();
  clean_id.remove(' ');
  clean_id = clean_id.toUpper();

  if (clean_id.isEmpty()) {
    return {};
  }

  QSqlQuery query(db_);

  // 2. First, check if it's already a Primary Key (FPR or Key ID)
  query.prepare("SELECT fpr FROM key_metadata WHERE fpr = :id OR key_id = :id");
  query.bindValue(":id", clean_id);
  if (query.exec() && query.next()) {
    return query.value(0).toString();  // Return itself
  }

  // 3. If not found, check if it's a Subkey. If yes, return its parent's FPR.
  query.prepare(
      "SELECT parent_fpr FROM subkey_metadata WHERE fpr = :id OR key_id = "
      ":id");
  query.bindValue(":id", clean_id);
  if (query.exec() && query.next()) {
    return query.value(0).toString();  // Return the parent primary fingerprint
  }

  // 4. Not found anywhere
  return {};
}

auto GFKeyDatabase::load_user_ids_for_parent(const QString& fpr)
    -> QContainer<GFUserId> {
  auto real_primary_fpr = ResolvePrimaryFpr(fpr);
  if (real_primary_fpr.isEmpty()) {
    LOG_W()
        << "Attempted to load user IDs for non-existent key with identifier: "
        << fpr;
    return {};
  }

  QContainer<GFUserId> uids;
  QSqlQuery query(db_);
  query.prepare(
      "SELECT user_id, is_primary, is_revoked FROM user_ids WHERE fpr = :fpr");
  query.bindValue(":fpr", real_primary_fpr);

  if (query.exec()) {
    while (query.next()) {
      auto uid_str = query.value(0).toString();
      auto is_primary = query.value(1).toBool();
      auto is_revoked = query.value(2).toBool();

      GFUserId uid(uid_str);
      uid.is_primary = is_primary;
      uid.is_revoked = is_revoked;

      if (is_primary) {
        // Primary UID is typically listed first, but we can also enforce it
        // here
        uids.prepend(uid);
      } else {
        uids.append(uid);
      }
    }
  }
  return uids;
}

GFKeyDatabase::~GFKeyDatabase() {
  if (db_.isOpen()) {
    db_.close();
  }
}
}  // namespace GpgFrontend
