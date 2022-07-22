/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GeneralMainWindow.h"

#include <utility>

#include "ui/struct/SettingsObject.h"

GpgFrontend::UI::GeneralMainWindow::GeneralMainWindow(std::string name,
                                                      QWidget* parent)
    : name_(std::move(name)), QMainWindow(parent) {
  slot_restore_settings();
}

GpgFrontend::UI::GeneralMainWindow::~GeneralMainWindow() = default;

void GpgFrontend::UI::GeneralMainWindow::closeEvent(QCloseEvent* event) {
  slot_save_settings();
  QMainWindow::closeEvent(event);
}

void GpgFrontend::UI::GeneralMainWindow::slot_restore_settings() noexcept {
  try {
    LOG(INFO) << name_ << _("Called");

    SettingsObject general_windows_state(name_ + "_state");

    std::string window_state = general_windows_state.Check(
        "window_state", saveState().toBase64().toStdString());

    // state sets pos & size of dock-widgets
    this->restoreState(
        QByteArray::fromBase64(QByteArray::fromStdString(window_state)));

    bool window_save = general_windows_state.Check("window_save", true);

    // Restore window size & location
    if (window_save) {
      int x = general_windows_state.Check("window_pos").Check("x", 100),
          y = general_windows_state.Check("window_pos").Check("y", 100);

      this->move({x, y});
      pos_ = {x, y};

      int width =
              general_windows_state.Check("window_size").Check("width", 800),
          height =
              general_windows_state.Check("window_size").Check("height", 450);

      this->resize({width, height});
      size_ = {width, height};
    }

    int width = general_windows_state.Check("icon_size").Check("width", 24),
        height = general_windows_state.Check("icon_size").Check("height", 24);
    LOG(INFO) << "icon_size" << width << height;

    icon_size_ = {width, height};
    font_size_ = general_windows_state.Check("font_size", 10);

    this->setIconSize(icon_size_);

    // icon_style
    int s_icon_style =
        general_windows_state.Check("icon_style", Qt::ToolButtonTextUnderIcon);
    auto icon_style = static_cast<Qt::ToolButtonStyle>(s_icon_style);
    this->setToolButtonStyle(icon_style);

  } catch (...) {
    LOG(ERROR) << name_ << "error";
  }
}

void GpgFrontend::UI::GeneralMainWindow::slot_save_settings() noexcept {
  try {
    LOG(INFO) << name_ << _("Called");

    SettingsObject general_windows_state(name_ + "_state");

    // window position and size
    general_windows_state["window_state"] =
        saveState().toBase64().toStdString();
    general_windows_state["window_pos"]["x"] = pos().x();
    general_windows_state["window_pos"]["y"] = pos().y();

    general_windows_state["window_size"]["width"] = size_.width();
    general_windows_state["window_size"]["height"] = size_.height();
    general_windows_state["window_save"] = true;

    // icon size
    general_windows_state["icon_size"]["width"] = icon_size_.width();
    general_windows_state["icon_size"]["height"] = icon_size_.height();

    // font size
    general_windows_state["font_size"] = font_size_;

  } catch (...) {
    LOG(ERROR) << name_ << "error";
  }
}
