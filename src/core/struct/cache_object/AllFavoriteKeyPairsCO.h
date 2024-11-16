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

#include "FavoriteKeyPairsByKeyDatabaseCO.h"

namespace GpgFrontend {

struct AllFavoriteKeyPairsCO {
  QMap<QString, FavoriteKeyPairsByKeyDatabaseCO> key_dbs;

  explicit AllFavoriteKeyPairsCO(const QJsonObject& j) {
    if (j.contains("key_dbs") && j["key_dbs"].isArray()) {
      for (const auto& o : j["key_dbs"].toArray()) {
        if (!o.isObject()) continue;
        auto mapping = o.toObject();

        if (!mapping.contains("key_db_name") || !mapping.contains("key_db")) {
          continue;
        }

        if (!mapping["key_db_name"].isString() ||
            !mapping["key_db"].isObject()) {
          continue;
        }

        key_dbs.insert(
            mapping["key_db_name"].toString(),
            FavoriteKeyPairsByKeyDatabaseCO(mapping["key_db"].toObject()));
      }
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    auto j_key_dbs = QJsonArray();
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    for (const auto& k : key_dbs.asKeyValueRange()) {
      QJsonObject o;
      o["key_db_name"] = k.first;
      o["key_db"] = k.second.ToJson();
      j_key_dbs.append(o);
    }
    j["key_dbs"] = j_key_dbs;
    return j;
  }
#else
    for (auto it = key_dbs.keyValueBegin(); it != key_dbs.keyValueEnd(); ++it) {
      QJsonObject o;
      o["key_db_name"] = it->first;
      o["key_db"] = it->second.ToJson();
      j_key_dbs.append(o);
    }
    j["key_dbs"] = j_key_dbs;
    return j;
  }
#endif
};

}  // namespace GpgFrontend