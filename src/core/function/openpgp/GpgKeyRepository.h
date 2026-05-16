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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GpgKeyTableModel.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton repository for GPG keys on a given channel.
 *
 * Provides cached lookups by key ID or fingerprint for primary keys, public
 * keys, and subkeys. The cache can be flushed to force a reload from the
 * underlying key database.
 */
class GF_CORE_EXPORT GpgKeyRepository
    : public SingletonFunctionObject<GpgKeyRepository> {
 public:
  /**
   * @brief Construct the repository for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit GpgKeyRepository(int channel = kGpgFrontendDefaultChannel);

  ~GpgKeyRepository();

  /**
   * @brief Look up a GPG key by key ID or fingerprint.
   *
   * @param key_id key ID or fingerprint string
   * @param use_cache if true, serve from cache when available; if false, bypass
   * cache
   * @return GpgKey value, or an invalid GpgKey if not found
   */
  auto GetKey(const QString& key_id, bool use_cache = true) -> GpgKey;

  /**
   * @brief Look up a GPG key by key ID or fingerprint and return a shared
   * pointer.
   *
   * @param key_id key ID or fingerprint string
   * @param use_cache if true, serve from cache when available
   * @return shared pointer to the GpgKey, or nullptr if not found
   */
  auto GetKeyPtr(const QString& key_id, bool use_cache = true)
      -> QSharedPointer<GpgKey>;

  /**
   * @brief Look up a public key (no secret material) by key ID or fingerprint.
   *
   * @param key_id key ID or fingerprint string
   * @param use_cache if true, serve from cache when available
   * @return GpgKey value representing the public key, or invalid if not found
   */
  auto GetPubkey(const QString& key_id, bool use_cache = true) -> GpgKey;

  /**
   * @brief Look up a public key by key ID or fingerprint and return a shared
   * pointer.
   *
   * @param key_id key ID or fingerprint string
   * @param use_cache if true, serve from cache when available
   * @return shared pointer to the public GpgKey, or nullptr if not found
   */
  auto GetPubkeyPtr(const QString& key_id, bool use_cache = true) -> GpgKeyPtr;

  /**
   * @brief Look up a key or subkey by key ID, returning an abstract key
   * pointer.
   *
   * Returns a GpgKey if the ID matches a primary key, or a GpgSubKey pointer
   * if it matches a subkey.
   *
   * @param key_id key ID or fingerprint string (primary or subkey)
   * @return abstract key pointer, or nullptr if not found
   */
  auto GetKeyORSubkeyPtr(const QString& key_id) -> GpgAbstractKeyPtr;

  /**
   * @brief Return all GPG keys known to this channel.
   *
   * @return list of shared pointers to all GpgKey objects
   */
  auto Fetch() -> QContainer<QSharedPointer<GpgKey>>;

  /**
   * @brief Flush the key cache, forcing the next lookup to reload from the
   * database.
   *
   * @return true if the cache was flushed successfully
   */
  auto FlushKeyCache() -> bool;

  /**
   * @brief Look up multiple GPG keys by their key IDs.
   *
   * @param key_ids list of key ID strings to look up
   * @return list of GpgKey objects (invalid entries for missing keys)
   */
  auto GetKeys(const KeyIdArgsList& key_ids) -> GpgKeyList;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};
}  // namespace GpgFrontend
