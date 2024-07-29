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

struct AppearanceSO {
  int text_editor_font_size = 12;
  int info_board_font_size = 12;
  int tool_bar_icon_width = 24;
  int tool_bar_icon_height = 24;
  Qt::ToolButtonStyle tool_bar_button_style = Qt::ToolButtonTextUnderIcon;

  bool save_window_state;

  explicit AppearanceSO(const QJsonObject& j) {
    if (const auto v = j["text_editor_font_size"]; v.isDouble()) {
      text_editor_font_size = v.toInt();
    }
    if (const auto v = j["info_board_font_size"]; v.isDouble()) {
      info_board_font_size = v.toInt();
    }
    if (const auto v = j["tool_bar_icon_width"]; v.isDouble()) {
      tool_bar_icon_width = v.toInt();
    }
    if (const auto v = j["tool_bar_icon_height"]; v.isDouble()) {
      tool_bar_icon_height = v.toInt();
    }
    if (const auto v = j["tool_bar_button_style"]; v.isDouble()) {
      tool_bar_button_style = static_cast<Qt::ToolButtonStyle>(v.toInt());
    }

    if (const auto v = j["save_window_state"]; v.isBool()) {
      save_window_state = v.toBool();
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    j["text_editor_font_size"] = text_editor_font_size;
    j["info_board_font_size"] = info_board_font_size;
    j["tool_bar_icon_width"] = tool_bar_icon_width;
    j["tool_bar_icon_height"] = tool_bar_icon_height;
    j["tool_bar_button_style"] = tool_bar_button_style;

    j["save_window_state"] = save_window_state;
    return j;
  }
};

}  // namespace GpgFrontend::UI