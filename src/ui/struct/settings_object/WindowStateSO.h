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

namespace GpgFrontend::UI {

struct WindowStateSO {
  bool window_save = false;
  QString window_state_data;
  int x = 100;
  int y = 100;
  int width = 400;
  int height = 200;

  WindowStateSO() = default;

  explicit WindowStateSO(const QJsonObject &j) {
    if (const auto v = j["window_save"]; v.isBool()) window_save = v.toBool();
    if (const auto v = j["window_state_data"]; v.isString()) {
      window_state_data = v.toString();
    }
    if (const auto v = j["x"]; v.isDouble()) x = v.toInt();
    if (const auto v = j["y"]; v.isDouble()) y = v.toInt();
    if (const auto v = j["width"]; v.isDouble()) width = v.toInt();
    if (const auto v = j["height"]; v.isDouble()) height = v.toInt();
  }

  [[nodiscard]] auto Json() const -> QJsonObject {
    QJsonObject j;
    j["window_save"] = window_save;
    j["window_state_data"] = window_state_data;
    j["x"] = x;
    j["y"] = y;
    j["width"] = width;
    j["height"] = height;
    return j;
  }
};
}  // namespace GpgFrontend::UI