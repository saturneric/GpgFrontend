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

#pragma once

#include <QSqlDatabase>

#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

/**
 * @brief SQLite-backed store for OpenPGP key metadata, subkeys, user IDs, and
 * armored key blocks.
 *
 * Each instance owns a uniquely named QSqlDatabase connection. The connection
 * is closed and removed from the Qt registry on destruction. Call Init() before
 * any other method; schema creation and migration to the current version are
 * performed automatically at that point.
 */
class GF_CORE_EXPORT GFKeyDatabase {
 public:
  /**
   * @brief Default-construct a database handle. Call Init() before use.
   */
  GFKeyDatabase() = default;

  /**
   * @brief Close the database connection and remove it from the Qt registry.
   */
  ~GFKeyDatabase();

  /**
   * @brief Open or create the database at @p path and migrate the schema if
   * needed.
   *
   * Creates the directory and database file (gf_keydb.sqlite) if they do not
   * exist, enables WAL and foreign-key pragmas, and runs any pending
   * migrations.
   *
   * @param path directory in which to place the database file
   * @return true on success, false if the directory, connection, or schema
   * setup fails
   */
  auto Init(const QString& path) -> bool;

  /**
   * @brief Return all primary key metadata records with their user IDs and
   * subkeys.
   *
   * @return list of GFKeyMetadata, each including associated user IDs and
   * subkeys
   */
  auto GetMetadataList() -> QContainer<GFKeyMetadata>;

  /**
   * @brief Return the armored public and secret key blocks for a given key.
   *
   * @param identifier primary fingerprint or key ID (primary or subkey)
   * @return key blocks if found, or empty if the identifier cannot be resolved
   */
  auto GetKeyBlocks(const QString& identifier) -> std::optional<GFKeyBlocks>;

  /**
   * @brief Return the full key record (metadata and blocks) for a given
   * identifier.
   *
   * @param identifier fingerprint or key ID (primary or subkey)
   * @return complete GFKey on success, or empty if not found
   */
  auto GetKeyByIdentifier(const QString& identifier) -> std::optional<GFKey>;

  /**
   * @brief Insert or replace a key record in a single transaction.
   *
   * Saves primary key metadata, user IDs, subkeys, and key blocks together.
   * If @p update_ts is true the update_time column is set to the current UTC
   * time; otherwise the timestamp from @p metadata is preserved.
   *
   * @param metadata primary key metadata including user IDs and subkeys
   * @param blocks armored public and secret key data
   * @param update_ts if true, stamp update_time with the current time
   * @return true if the transaction committed successfully, false otherwise
   */
  auto SaveKey(const GFKeyMetadata& metadata, const GFKeyBlocks& blocks,
               bool update_ts = true) -> bool;

  /**
   * @brief Return metadata for the primary key identified by fingerprint or key
   * ID.
   *
   * @param identifier fingerprint or key ID (primary or subkey)
   * @return GFKeyMetadata including user IDs and subkeys, or empty if not found
   */
  auto GetKeyMetadata(const QString& identifier)
      -> std::optional<GFKeyMetadata>;

  /**
   * @brief Delete a key and its cascaded subkeys, user IDs, and blocks from the
   * database.
   *
   * @param fpr primary fingerprint of the key to delete
   * @return true if the key was found and deleted, false otherwise
   */
  auto DeleteKey(const QString& fpr) -> bool;

  /**
   * @brief Resolve a fingerprint or key ID to the corresponding primary key
   * fingerprint.
   *
   * Accepts fingerprints and key IDs for both primary keys and subkeys.
   *
   * @param identifier fingerprint or key ID to resolve
   * @return primary fingerprint string, or an empty string if not found
   */
  auto ResolvePrimaryFpr(const QString& identifier) -> QString;

  /**
   * @brief Return the current SQLite schema version (PRAGMA user_version).
   *
   * @return schema version integer, or -1 on error
   */
  auto GetSchemaVersion() -> int;

  /**
   * @brief Mark a key as disabled without removing it from the database.
   *
   * @param fpr primary fingerprint of the key to disable
   * @return true if the key was found and updated, false otherwise
   */
  auto DisableKey(const QString& fpr) -> bool;

 private:
  QSqlDatabase db_;          ///< SQLite database connection
  QString connection_name_;  ///< Unique connection name used to remove the
                             ///< connection on destruction

  /**
   * @brief Open a QSQLITE connection to the database file at @p path.
   *
   * Generates a unique connection name on first call. Returns false if the
   * database cannot be opened or a connection is already initialised.
   *
   * @param path absolute path to the SQLite database file
   * @return true if the connection is open, false on error
   */
  auto connect_db(const QString& path) -> bool;

  /**
   * @brief Create all tables at the current schema version.
   *
   * Creates key_metadata, subkey_metadata, key_blocks, and user_ids tables.
   *
   * @return true on success, false if any CREATE TABLE statement fails
   */
  auto create_schema_latest() -> bool;

  /**
   * @brief Load all subkeys belonging to the given primary key fingerprint.
   *
   * @param parent_fpr primary key fingerprint
   * @return list of GFSubKeyMetadata for that primary key
   */
  auto load_subkeys_for_parent(const QString& parent_fpr)
      -> QContainer<GFSubKeyMetadata>;

  /**
   * @brief Load all user IDs belonging to the primary key identified by @p fpr.
   *
   * @param fpr fingerprint or key ID; resolved to the primary fingerprint
   * internally
   * @return list of GFUserId, primary UID first
   */
  auto load_user_ids_for_parent(const QString& fpr) -> QContainer<GFUserId>;

  /**
   * @brief Enable recommended SQLite pragmas: foreign keys, WAL journal mode,
   * and synchronous=NORMAL.
   *
   * @return true if foreign_keys was enabled successfully (WAL and synchronous
   * failures are warned but not fatal)
   */
  auto enable_pragmas() -> bool;

  /**
   * @brief Read the schema version and run migrations until the database is
   * current.
   *
   * @return true if the schema is at the expected version, false on any
   * migration error
   */
  auto ensure_schema() -> bool;

  /**
   * @brief Write @p version to the SQLite user_version PRAGMA.
   *
   * @param version schema version to record
   * @return true on success, false if the PRAGMA statement fails
   */
  auto set_schema_version(int version) -> bool;

  /**
   * @brief Return whether the named table exists in the database.
   *
   * @param table_name table to check
   * @return true if the table exists, false otherwise
   */
  auto table_exists(const QString& table_name) -> bool;

  /**
   * @brief Return whether the named column exists in the given table.
   *
   * @param table_name table to inspect
   * @param column_name column to look for
   * @return true if the column exists, false otherwise
   */
  auto column_exists(const QString& table_name, const QString& column_name)
      -> bool;

  /**
   * @brief Return whether the database contains no user-created tables.
   *
   * @return true if no user tables exist, false otherwise
   */
  auto is_database_empty() -> bool;

  /**
   * @brief Migrate the schema from version 1 to version 2.
   *
   * Adds the key_ver column to key_metadata and subkey_metadata if not present.
   *
   * @return true on success, false if any ALTER TABLE statement fails
   */
  auto migrate_v1_to_v2() -> bool;

  /**
   * @brief Restrict the database directory and files to the current user via OS
   * permissions.
   *
   * Sets the storage directory to owner-only (0700) and the database file and
   * its WAL/SHM sidecars to owner read/write (0600), protecting the plaintext
   * key material from other local users. Missing sidecar files are skipped.
   *
   * @param db_dir storage directory holding the database file
   * @param db_file absolute path to the SQLite database file
   */
  static void secure_storage_permissions(const QString& db_dir,
                                         const QString& db_file);
};

}  // namespace GpgFrontend
