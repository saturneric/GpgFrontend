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

#include "KeyGroupCO.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

struct KeyGroupsCO {
  QContainer<KeyGroupCO> key_groups;
  QString key_db_name;

  KeyGroupsCO() = default;

  explicit KeyGroupsCO(const QJsonObject& j) {
    if (const auto v = j["key_db_name"]; v.isString()) {
      key_db_name = v.toString();
    }
    if (!j.contains("key_groups") || !j["key_groups"].isArray()) return;
    for (const auto& i : j["key_groups"].toArray()) {
      if (!i.isObject()) continue;

      key_groups.append(KeyGroupCO{i.toObject()});
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;

    j["key_db_name"] = key_db_name;

    auto a = QJsonArray();
    for (const auto& k : key_groups) {
      a.push_back(k.ToJson());
    }
    j["key_groups"] = a;
    return j;
  }
};

}  // namespace GpgFrontend