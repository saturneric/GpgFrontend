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

#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto GFKeyDatabase::connect_db(const QString& path) -> bool {
  db_ = QSqlDatabase::addDatabase("QSQLITE");

  db_.setDatabaseName(path);

  if (!db_.open()) {
    LOG_E() << "cannot open key database at path: " << path;
    return false;
  }

  return true;
}

auto GFKeyDatabase::GetMetadataList() -> QList<GFKeyMetadata> {
  QList<GFKeyMetadata> list;
  QSqlQuery query(db_);

  if (query.exec(R"(
        SELECT fpr, key_id, user_id, algo, created_at, has_secret, 
               can_sign, can_encrypt, can_auth, can_certify 
        FROM key_metadata
      )")) {
    while (query.next()) {
      GFKeyMetadata meta;
      meta.fpr = query.value(0).toString();
      meta.key_id = query.value(1).toString();
      meta.user_id = query.value(2).toString();
      meta.algo = query.value(3).toInt();
      meta.created_at = query.value(4).toLongLong();
      meta.has_secret = query.value(5).toBool();
      meta.can_sign = query.value(6).toBool();
      meta.can_encrypt = query.value(7).toBool();
      meta.can_auth = query.value(8).toBool();
      meta.can_certify = query.value(9).toBool();
      meta.user_ids.push_back(ParseUserId(meta.user_id));

      // Load subkeys for this primary key
      meta.subkeys = load_subkeys_for_parent(meta.fpr);

      // In gnupg keyring, the primary key is also represented as the first
      // subkey, so we need to add it to the subkeys list when returning the
      // metadata
      GFSubKeyMetadata primary_as_subkey;
      primary_as_subkey.marked = false;
      primary_as_subkey.fpr = meta.fpr;
      primary_as_subkey.key_id = meta.key_id;
      primary_as_subkey.algo = meta.algo;
      primary_as_subkey.created_at = meta.created_at;
      primary_as_subkey.has_secret = meta.has_secret;
      primary_as_subkey.can_sign = meta.can_sign;
      primary_as_subkey.can_encrypt = meta.can_encrypt;
      primary_as_subkey.can_auth = meta.can_auth;
      primary_as_subkey.can_certify = meta.can_certify;
      meta.subkeys.prepend(primary_as_subkey);

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
    (fpr, key_id, user_id, algo, created_at, has_secret, can_sign, can_encrypt, can_auth, can_certify)
    VALUES (:fpr, :key_id, :user_id, :algo, :created_at, :has_secret, :can_sign, :can_encrypt, :can_auth, :can_certify)
  )");
  query.bindValue(":fpr", meta.fpr.toUpper());
  query.bindValue(":key_id", meta.key_id.toUpper());
  query.bindValue(":user_id", meta.user_id);
  query.bindValue(":algo", meta.algo);
  query.bindValue(":created_at", meta.created_at);
  query.bindValue(":has_secret", meta.has_secret ? 1 : 0);
  query.bindValue(":can_sign", meta.can_sign ? 1 : 0);
  query.bindValue(":can_encrypt", meta.can_encrypt ? 1 : 0);
  query.bindValue(":can_auth", meta.can_auth ? 1 : 0);
  query.bindValue(":can_certify", meta.can_certify ? 1 : 0);

  if (!query.exec()) {
    LOG_E() << "SaveKey meta error: " << query.lastError().text();
    db_.rollback();
    return false;
  }

  // 2. Save Subkeys
  query.prepare(R"(
    INSERT OR REPLACE INTO subkey_metadata 
    (fpr, parent_fpr, key_id, algo, created_at, has_secret, can_sign, can_encrypt, can_auth)
    VALUES (:fpr, :parent_fpr, :key_id, :algo, :created_at, :has_secret, :can_sign, :can_encrypt, :can_auth)
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
    SELECT fpr, key_id, user_id, algo, created_at, has_secret, 
           can_sign, can_encrypt, can_auth, can_certify 
    FROM key_metadata WHERE fpr = :fpr
  )");
  // Bind the resolved fingerprint, not the original identifier!
  query.bindValue(":fpr", real_primary_fpr);

  if (query.exec() && query.next()) {
    GFKeyMetadata meta;
    meta.fpr = query.value(0).toString();
    meta.key_id = query.value(1).toString();
    meta.user_id = query.value(2).toString();
    meta.algo = query.value(3).toInt();
    meta.created_at = query.value(4).toLongLong();
    meta.has_secret = query.value(5).toBool();
    meta.can_sign = query.value(6).toBool();
    meta.can_encrypt = query.value(7).toBool();
    meta.can_auth = query.value(8).toBool();
    meta.can_certify = query.value(9).toBool();
    meta.user_ids.push_back(ParseUserId(meta.user_id));

    // Load subkeys
    meta.subkeys = load_subkeys_for_parent(meta.fpr);

    // In gnupg keyring, the primary key is also represented as the first
    // subkey, so we need to add it to the subkeys list when returning the
    // metadata
    GFSubKeyMetadata primary_as_subkey;
    primary_as_subkey.marked = false;
    primary_as_subkey.fpr = meta.fpr;
    primary_as_subkey.key_id = meta.key_id;
    primary_as_subkey.algo = meta.algo;
    primary_as_subkey.created_at = meta.created_at;
    primary_as_subkey.has_secret = meta.has_secret;
    primary_as_subkey.can_sign = meta.can_sign;
    primary_as_subkey.can_encrypt = meta.can_encrypt;
    primary_as_subkey.can_auth = meta.can_auth;
    primary_as_subkey.can_certify = meta.can_certify;
    meta.subkeys.prepend(primary_as_subkey);

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
      user_id TEXT,
      algo INTEGER,
      created_at INTEGER,
      has_secret INTEGER DEFAULT 0,
      can_sign INTEGER DEFAULT 0,
      can_encrypt INTEGER DEFAULT 0,
      can_auth INTEGER DEFAULT 0,
      can_certify INTEGER DEFAULT 0
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
      has_secret INTEGER DEFAULT 0,
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

  return true;
}

auto GFKeyDatabase::load_subkeys_for_parent(const QString& parent_fpr)
    -> QList<GFSubKeyMetadata> {
  QList<GFSubKeyMetadata> subkeys;
  QSqlQuery query(db_);
  query.prepare(R"(
    SELECT fpr, key_id, algo, created_at, has_secret, can_sign, can_encrypt, can_auth, can_certify
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
      sub.can_sign = query.value(5).toBool();
      sub.can_encrypt = query.value(6).toBool();
      sub.can_auth = query.value(7).toBool();
      sub.can_certify = query.value(8).toBool();
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

GFKeyDatabase::~GFKeyDatabase() {
  if (db_.isOpen()) {
    db_.close();
  }
}
}  // namespace GpgFrontend
