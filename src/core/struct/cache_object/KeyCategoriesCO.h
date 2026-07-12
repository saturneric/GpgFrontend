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

#include "core/struct/cache_object/KeyCategoryCO.h"

namespace GpgFrontend {

/**
 * @brief On-disk representation of all key categories belonging to one key
 * database.
 *
 * Persisted through the durable cache under the key "kcats:<key_db_name>". The
 * embedded key_db_name is verified on load and the payload is discarded if it
 * does not match the active database, mirroring KeyGroupsCO.
 *
 * legacy_favorites_migrated records whether the built-in "favourite" category
 * has already absorbed the pre-existing favourite key list, so the one-time
 * migration is not repeated.
 */
struct KeyCategoriesCO {
  QContainer<KeyCategoryCO> categories;
  QString key_db_name;
  bool legacy_favorites_migrated = false;

  KeyCategoriesCO() = default;

  explicit KeyCategoriesCO(const QJsonObject& j) {
    if (const auto v = j["key_db_name"]; v.isString()) {
      key_db_name = v.toString();
    }
    if (const auto v = j["legacy_favorites_migrated"]; v.isBool()) {
      legacy_favorites_migrated = v.toBool();
    }

    if (!j.contains("categories") || !j["categories"].isArray()) return;
    for (const auto& i : j["categories"].toArray()) {
      if (!i.isObject()) continue;
      categories.push_back(KeyCategoryCO(i.toObject()));
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    j["key_db_name"] = key_db_name;
    j["legacy_favorites_migrated"] = legacy_favorites_migrated;

    auto a = QJsonArray();
    for (const auto& c : categories) {
      a.push_back(c.ToJson());
    }
    j["categories"] = a;
    return j;
  }
};

}  // namespace GpgFrontend
