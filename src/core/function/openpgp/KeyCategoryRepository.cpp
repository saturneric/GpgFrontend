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

#include "KeyCategoryRepository.h"

#include "core/function/CacheManager.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/helper/KeyDatabaseScopedCache.h"
#include "core/struct/cache_object/KeyCategoriesCO.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

KeyCategoryRepository::KeyCategoryRepository(int channel)
    : SingletonFunctionObject<KeyCategoryRepository>(channel) {
  fetch_categories();
}

auto KeyCategoryRepository::Fetch() -> QList<KeyCategoryCO> {
  return categories_;
}

auto KeyCategoryRepository::find_category(const QString& id) -> KeyCategoryCO* {
  for (auto& c : categories_) {
    if (c.id == id) return &c;
  }
  return nullptr;
}

void KeyCategoryRepository::fetch_categories() {
  categories_.clear();
  tab_colors_.clear();
  tab_orders_.clear();

  auto json = LoadKeyDatabaseScopedCache(cm_, "kcats", ctx_.KeyDBId(),
                                         ctx_.KeyDBName());
  if (!json) return;

  auto co = KeyCategoriesCO(*json);
  categories_ = co.categories;
  tab_colors_ = co.tab_colors;
  tab_orders_ = co.tab_orders;
}

void KeyCategoryRepository::persist_categories() {
  // A retired channel's copy was loaded before the Runtime that replaced it,
  // which has its own copy of the same database's entry by now. Work still
  // finishing here must not write the older one over it.
  if (IsRetiredChannelHandle(GetChannel())) {
    LOG_W() << "not persisting from a retired channel:" << GetChannel();
    return;
  }

  KeyCategoriesCO co;
  co.key_db_name = ctx_.KeyDBName();
  co.categories = categories_;
  co.tab_colors = tab_colors_;
  co.tab_orders = tab_orders_;

  SaveKeyDatabaseScopedCache(cm_, "kcats", ctx_.KeyDBId(), ctx_.KeyDBName(),
                             co.ToJson());
}

auto KeyCategoryRepository::AddCategory(const QString& name,
                                        const QString& color) -> QString {
  KeyCategoryCO c;
  c.id = "cat:" + QUuid::createUuid().toRfc4122().toHex().left(14).toUpper();
  c.name = name;
  c.color = color;
  c.builtin = false;
  c.creation_time = CurrentDateTime();

  categories_.push_back(c);
  persist_categories();
  return c.id;
}

auto KeyCategoryRepository::Remove(const QString& id) -> bool {
  auto* c = find_category(id);
  if (c == nullptr || c->builtin) return false;

  for (auto it = categories_.begin(); it != categories_.end(); ++it) {
    if (it->id == id) {
      categories_.erase(it);
      break;
    }
  }
  persist_categories();
  return true;
}

auto KeyCategoryRepository::Rename(const QString& id, const QString& name)
    -> bool {
  auto* c = find_category(id);
  if (c == nullptr || c->builtin) return false;

  c->name = name;
  persist_categories();
  return true;
}

auto KeyCategoryRepository::AddKey2Category(const QString& id,
                                            const QString& key_id) -> bool {
  auto* c = find_category(id);
  if (c == nullptr || key_id.isEmpty() || c->key_ids.contains(key_id)) {
    return false;
  }

  c->key_ids.append(key_id);
  persist_categories();
  return true;
}

auto KeyCategoryRepository::RemoveKeyFromCategory(const QString& id,
                                                  const QString& key_id)
    -> bool {
  auto* c = find_category(id);
  if (c == nullptr || !c->key_ids.contains(key_id)) return false;

  c->key_ids.removeAll(key_id);
  persist_categories();
  return true;
}

auto KeyCategoryRepository::Contains(const QString& id, const QString& key_id)
    -> bool {
  auto* c = find_category(id);
  return c != nullptr && c->key_ids.contains(key_id);
}

auto KeyCategoryRepository::KeyIdsOf(const QString& id) -> QStringList {
  auto* c = find_category(id);
  return c != nullptr ? c->key_ids : QStringList{};
}

auto KeyCategoryRepository::GetTabColor(const QString& id) -> QString {
  if (const auto it = tab_colors_.constFind(id); it != tab_colors_.constEnd()) {
    return it.value();
  }
  // Fall back to a custom category's own colour, if any.
  if (auto* c = find_category(id); c != nullptr) return c->color;
  return {};
}

void KeyCategoryRepository::SetTabColor(const QString& id,
                                        const QString& color) {
  if (color.isEmpty()) {
    tab_colors_.remove(id);
  } else {
    tab_colors_.insert(id, color);
  }
  persist_categories();
}

auto KeyCategoryRepository::GetTabOrder(const QString& scope) -> QStringList {
  return tab_orders_.value(scope);
}

void KeyCategoryRepository::SetTabOrder(const QString& scope,
                                        const QStringList& order) {
  if (order.isEmpty()) {
    tab_orders_.remove(scope);
  } else {
    tab_orders_.insert(scope, order);
  }
  persist_categories();
}

auto KeyCategoryRepository::FlushCache() -> bool {
  fetch_categories();
  return true;
}

}  // namespace GpgFrontend
