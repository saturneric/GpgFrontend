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

#include <optional>

#include "core/function/CacheManager.h"

namespace GpgFrontend {

/**
 * @brief The durable cache key a key database's own state is kept under.
 *
 * Keyed by the database's id, which survives a rename and a reorder. A context
 * built without one (a test, a placeholder) falls back to the name, which is
 * how every entry was keyed before the id existed.
 *
 * @param prefix what is kept, e.g. "kgs" for key groups
 * @param db_id the database id, possibly empty
 * @param db_name the database name
 * @return the cache key
 */
inline auto KeyDatabaseScopedCacheKey(const QString& prefix,
                                      const QString& db_id,
                                      const QString& db_name) -> QString {
  return QString("%1:%2").arg(prefix, db_id.isEmpty() ? db_name : db_id);
}

/**
 * @brief Load what is kept for one key database under @p prefix.
 *
 * An entry keyed by the id is accepted when it records that id. Failing that,
 * an entry written under the name by a build before the id existed is taken
 * over once: written back under the id and removed under the name, so the
 * next database to be given this name does not inherit it.
 *
 * The payload is expected to carry "key_db_name", and "key_db_id" once it has
 * been written by this build -- what KeyGroupsCO and KeyCategoriesCO write.
 *
 * @param cm the durable cache
 * @param prefix what is kept, e.g. "kgs" for key groups
 * @param db_id the database id, possibly empty
 * @param db_name the database name
 * @return the payload, or nothing when none belongs to this database
 */
inline auto LoadKeyDatabaseScopedCache(CacheManager& cm, const QString& prefix,
                                       const QString& db_id,
                                       const QString& db_name)
    -> std::optional<QJsonObject> {
  const auto key = KeyDatabaseScopedCacheKey(prefix, db_id, db_name);
  auto stored = cm.LoadDurableCache(key).object();

  if (db_id.isEmpty()) {
    if (stored["key_db_name"].toString() != db_name) return {};
    return stored;
  }

  if (stored["key_db_id"].toString() == db_id) return stored;

  // Not yet under the id: a database whose state an earlier build kept under
  // its name. Only an entry that records no id at all is taken, so one that
  // already moved to another database's id is never claimed twice.
  const auto legacy_key = KeyDatabaseScopedCacheKey(prefix, {}, db_name);
  auto legacy = cm.LoadDurableCache(legacy_key).object();
  if (legacy.isEmpty() || legacy.contains("key_db_id") ||
      legacy["key_db_name"].toString() != db_name) {
    return {};
  }

  legacy["key_db_id"] = db_id;
  cm.SaveDurableCache(key, QJsonDocument{legacy}, true);
  cm.ResetDurableCache(legacy_key);
  return legacy;
}

/**
 * @brief Keep @p payload for one key database under @p prefix.
 *
 * Stamps it with the database's id and name, which is what
 * LoadKeyDatabaseScopedCache() checks it against.
 *
 * @param cm the durable cache
 * @param prefix what is kept, e.g. "kgs" for key groups
 * @param db_id the database id, possibly empty
 * @param db_name the database name
 * @param payload what to keep
 */
inline void SaveKeyDatabaseScopedCache(CacheManager& cm, const QString& prefix,
                                       const QString& db_id,
                                       const QString& db_name,
                                       QJsonObject payload) {
  if (!db_id.isEmpty()) payload["key_db_id"] = db_id;
  payload["key_db_name"] = db_name;
  cm.SaveDurableCache(KeyDatabaseScopedCacheKey(prefix, db_id, db_name),
                      QJsonDocument{payload}, true);
}

}  // namespace GpgFrontend
