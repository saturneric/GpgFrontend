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

#include "core/function/CacheManager.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/struct/cache_object/KeyCategoryCO.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton repository for managing key categories on a given channel.
 *
 * A key category is a named, user-defined bucket that groups keys within one
 * key database for organisational purposes. Categories are membership-based
 * (a set of key IDs), scoped per key database, and persisted through the
 * durable cache under the key "kcats:<key_db_name>". The same cache also stores
 * per-tab colours and per-scope tab orders.
 */
class GF_CORE_EXPORT KeyCategoryRepository
    : public SingletonFunctionObject<KeyCategoryRepository> {
 public:
  /**
   * @brief Construct the repository for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit KeyCategoryRepository(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Return all categories known to this channel, built-ins first.
   *
   * @return ordered list of category cache objects
   */
  auto Fetch() -> QContainer<KeyCategoryCO>;

  /**
   * @brief Reload categories from persistent storage.
   *
   * @return true if the cache was reloaded successfully
   */
  auto FlushCache() -> bool;

  /**
   * @brief Create a new (non built-in) category.
   *
   * @param name display name
   * @param color optional colour hint (e.g. "#RRGGBB")
   * @return the identifier of the newly created category
   */
  auto AddCategory(const QString& name, const QString& color = {}) -> QString;

  /**
   * @brief Delete the category with the given identifier.
   *
   * Built-in categories cannot be removed.
   *
   * @param id category identifier
   * @return true if the category was removed
   */
  auto Remove(const QString& id) -> bool;

  /**
   * @brief Rename the category with the given identifier.
   *
   * Built-in categories cannot be renamed.
   *
   * @param id category identifier
   * @param name new display name
   * @return true if the category was renamed
   */
  auto Rename(const QString& id, const QString& name) -> bool;

  /**
   * @brief Add a key to a category.
   *
   * @param id category identifier
   * @param key_id key ID to add
   * @return true if the key was added, false if the category was not found or
   * the key was already a member
   */
  auto AddKey2Category(const QString& id, const QString& key_id) -> bool;

  /**
   * @brief Remove a key from a category.
   *
   * @param id category identifier
   * @param key_id key ID to remove
   * @return true if the key was removed
   */
  auto RemoveKeyFromCategory(const QString& id, const QString& key_id) -> bool;

  /**
   * @brief Return whether the given key is a member of the category.
   *
   * @param id category identifier
   * @param key_id key ID to test
   * @return true if the key belongs to the category
   */
  auto Contains(const QString& id, const QString& key_id) -> bool;

  /**
   * @brief Return the member key IDs of the category.
   *
   * @param id category identifier
   * @return list of key IDs, empty if the category does not exist
   */
  auto KeyIdsOf(const QString& id) -> QStringList;

  /**
   * @brief The user-chosen colour for a tab id, or empty if none.
   *
   * Works for both custom categories and built-in tab ids.
   *
   * @param id tab id
   * @return "#RRGGBB" colour, or an empty string
   */
  auto GetTabColor(const QString& id) -> QString;

  /**
   * @brief Persist a user-chosen colour for a tab id (empty clears it).
   *
   * @param id tab id
   * @param color "#RRGGBB" colour, or empty to reset
   */
  void SetTabColor(const QString& id, const QString& color);

  /**
   * @brief The persisted tab order for a scope (e.g. a window), or empty.
   *
   * @param scope order scope identifier
   * @return ordered tab ids
   */
  auto GetTabOrder(const QString& scope) -> QStringList;

  /**
   * @brief Persist the tab order for a scope (empty clears it).
   *
   * @param scope order scope identifier
   * @param order ordered tab ids, or empty to reset
   */
  void SetTabOrder(const QString& scope, const QStringList& order);

 private:
  // OpenPGP context, used to resolve the active key database name.
  OpenPGPContext& ctx_ =
      OpenPGPContext::GetInstance(SingletonFunctionObject::GetChannel());
  // Cache manager used to persist and restore category data.
  CacheManager& cm_ =
      CacheManager::GetInstance(SingletonFunctionObject::GetChannel());

  // All custom categories for the active key database.
  QContainer<KeyCategoryCO> categories_;
  // Per-tab colour overrides (tab id -> "#RRGGBB").
  QMap<QString, QString> tab_colors_;
  // Per-scope tab orders (scope -> ordered tab ids).
  QMap<QString, QStringList> tab_orders_;

  // Load categories from the durable cache into categories_.
  void fetch_categories();

  // Write categories_ back to the durable cache.
  void persist_categories();

  // Return a pointer to the category with the given id, or nullptr.
  auto find_category(const QString& id) -> KeyCategoryCO*;
};

}  // namespace GpgFrontend
