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

#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

struct KeyDatabaseListSO {
  QContainer<KeyDatabaseItemSO> key_databases;

  KeyDatabaseListSO() = default;

  explicit KeyDatabaseListSO(const QJsonObject& j) {
    if (const auto v = j["key_databases"]; v.isArray()) {
      const QJsonArray j_array = v.toArray();
      for (const auto& key_database : j_array) {
        if (key_database.isObject()) {
          key_databases.append(KeyDatabaseItemSO(key_database.toObject()));
        }
      }
    }
  }

  auto ToJson() -> QJsonObject {
    QJsonObject j;
    auto j_array = QJsonArray();
    for (const auto& s : key_databases) {
      j_array.push_back(s.ToJson());
    }
    j["key_databases"] = j_array;
    return j;
  }
};

}  // namespace GpgFrontend