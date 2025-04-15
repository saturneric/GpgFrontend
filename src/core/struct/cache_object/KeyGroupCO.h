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

namespace GpgFrontend {

struct KeyGroupCO {
  QString id;
  QString name;
  QString email;
  QString comment;
  QDateTime creation_time;
  QStringList key_ids;

  KeyGroupCO() = default;

  explicit KeyGroupCO(const QJsonObject& j) {
    if (const auto v = j["id"]; v.isString()) {
      id = v.toString();
    }
    if (const auto v = j["name"]; v.isString()) {
      name = v.toString();
    }
    if (const auto v = j["email"]; v.isString()) {
      email = v.toString();
    }
    if (const auto v = j["comment"]; v.isString()) {
      comment = v.toString();
    }
    if (const auto v = j["creation_time"]; v.isDouble()) {
      creation_time = QDateTime::fromSecsSinceEpoch(v.toInt());
    }

    if (!j.contains("key_ids") || !j["key_ids"].isArray()) return;
    for (const auto& i : j["key_ids"].toArray()) {
      if (!i.isString()) continue;

      key_ids.append(i.toString());
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;

    j["id"] = id;
    j["name"] = name;
    j["email"] = email;
    j["comment"] = comment;
    j["creation_time"] = creation_time.toSecsSinceEpoch();

    auto a = QJsonArray();
    for (const auto& k : key_ids) {
      a.push_back(k);
    }
    j["key_ids"] = a;
    return j;
  }
};

}  // namespace GpgFrontend