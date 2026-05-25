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
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Singleton manager providing two-tier caching: runtime and durable.
 *
 * Runtime cache: volatile in-memory LRU store with optional TTL, holding
 * strings (SaveCache/LoadCache) or raw binary (SaveSecCache/LoadSecCache).
 *
 * Durable cache: encrypted on-disk store backed by DataObjectOperator,
 * holding JSON (SaveDurableCache/LoadDurableCache) or raw binary
 * (SaveSecDurableCache/LoadSecDurableCache). Dirty entries are flushed to
 * disk automatically every 15 seconds or on an explicit FlushCacheStorage().
 */
class GF_CORE_EXPORT CacheManager
    : public SingletonFunctionObject<CacheManager> {
 public:
  /**
   * @brief Construct the cache manager and load all durable cache entries from
   * disk.
   *
   * @param channel singleton channel identifier
   */
  explicit CacheManager(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Destroy the cache manager, releasing all runtime and durable cache
   * state.
   */
  ~CacheManager() override;

  /**
   * @brief Store a string value in the runtime in-memory cache.
   *
   * @param key cache key
   * @param value string value to store
   * @param ttl time-to-live in seconds from now, or -1 for no expiry
   */
  void SaveCache(const QString& key, QString value, qint64 ttl = -1);

  /**
   * @brief Store a raw binary value in the runtime in-memory cache.
   *
   * @param key cache key
   * @param value binary value to store
   * @param ttl time-to-live in seconds from now, or -1 for no expiry
   */
  void SaveSecCache(const QString& key, const GFBuffer& value, qint64 ttl = -1);

  /**
   * @brief Store a JSON document in the durable encrypted on-disk cache.
   *
   * @param key cache key
   * @param value JSON document to store
   * @param flush if true, write dirty entries to disk immediately
   */
  void SaveDurableCache(const QString& key, const QJsonDocument& value,
                        bool flush = false);

  /**
   * @brief Store a raw binary value in the durable encrypted on-disk cache.
   *
   * @param key cache key
   * @param value binary value to store
   * @param flush if true, write dirty entries to disk immediately
   */
  void SaveSecDurableCache(const QString& key, const GFBuffer& value,
                           bool flush = false);

  /**
   * @brief Retrieve a string value from the runtime cache.
   *
   * Returns an empty string if the key is absent or the entry has expired.
   *
   * @param key cache key
   * @return cached string, or empty if not found or expired
   */
  auto LoadCache(const QString& key) -> QString;

  /**
   * @brief Retrieve a raw binary value from the runtime cache.
   *
   * Returns an empty GFBuffer if the key is absent or the entry has expired.
   *
   * @param key cache key
   * @return cached binary value, or empty if not found or expired
   */
  auto LoadSecCache(const QString& key) -> GFBuffer;

  /**
   * @brief Retrieve a raw binary value from the durable cache, loading from
   * disk if needed.
   *
   * @param key cache key
   * @return cached binary value, or empty if not found
   */
  auto LoadSecDurableCache(const QString& key) -> GFBuffer;

  /**
   * @brief Retrieve a raw binary value from the durable cache, returning a
   * default if absent.
   *
   * @param key cache key
   * @param default_value value to return if the key is not found
   * @return cached binary value, or default_value if not found
   */
  auto LoadSecDurableCache(const QString& key, const GFBuffer& default_value)
      -> GFBuffer;

  /**
   * @brief Retrieve a JSON document from the durable cache, loading from disk
   * if needed.
   *
   * @param key cache key
   * @return cached JSON document, or an empty document if not found
   */
  auto LoadDurableCache(const QString& key) -> QJsonDocument;

  /**
   * @brief Retrieve a JSON document from the durable cache, returning a default
   * if absent.
   *
   * @param key cache key
   * @param default_value value to return if the key is not found
   * @return cached JSON document, or default_value if not found
   */
  auto LoadDurableCache(const QString& key, QJsonDocument default_value)
      -> QJsonDocument;

  /**
   * @brief Remove an entry from the runtime in-memory cache.
   *
   * @param key cache key to remove
   */
  void ResetCache(const QString& key);

  /**
   * @brief Remove an entry from the durable cache.
   *
   * @param key cache key to remove
   * @return true if the key existed and was removed, false if it was not found
   */
  auto ResetDurableCache(const QString& key) -> bool;

  /**
   * @brief Immediately flush all pending durable cache writes to disk.
   *
   * This is called automatically on a 15-second timer; use this to force
   * an early flush, for example before application shutdown.
   */
  void FlushCacheStorage();

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend
