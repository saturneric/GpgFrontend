/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

namespace GpgFrontend::UI {

struct KeyServerSO {
  int default_server = 0;
  QStringList server_list;

  KeyServerSO() = default;

  explicit KeyServerSO(const QJsonObject& j) {
    if (const auto v = j["default_server"]; v.isDouble()) {
      default_server = v.toInt();
    }

    if (const auto v = j["server_list"]; v.isArray()) {
      const QJsonArray j_array = v.toArray();
      server_list.reserve(j_array.size());
      for (const auto& server : j_array) {
        server_list.append(server.toString());
      }
    }

    if (server_list.empty()) ResetDefaultServerList();
  }

  auto ToJson() -> QJsonObject {
    QJsonObject j;
    j["default_server"] = default_server;
    auto j_array = QJsonArray();

    for (const auto& s : server_list) {
      j_array.push_back(s);
    }
    j["server_list"] = j_array;
    return j;
  }

  auto GetTargetServer() -> QString {
    if (server_list.empty()) this->ResetDefaultServerList();
    if (default_server >= server_list.size()) default_server = 0;
    return server_list[default_server];
  }

  void ResetDefaultServerList() {
    server_list << "https://keyserver.ubuntu.com"
                << "https://keys.openpgp.org";
  }
};

}  // namespace GpgFrontend::UI