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

#include "GeneralMainWindow.h"

#include <utility>

#include "ui/struct/SettingsObject.h"

GpgFrontend::UI::GeneralMainWindow::GeneralMainWindow(std::string name,
                                                      QWidget *parent)
    : QMainWindow(parent), name_(std::move(name)) {
  slot_restore_settings();
}

GpgFrontend::UI::GeneralMainWindow::~GeneralMainWindow() = default;

void GpgFrontend::UI::GeneralMainWindow::closeEvent(QCloseEvent *event) {
  GF_UI_LOG_DEBUG("main window close event caught, event type: {}",
                  event->type());
  slot_save_settings();

  QMainWindow::closeEvent(event);
}

void GpgFrontend::UI::GeneralMainWindow::slot_restore_settings() noexcept {
  try {
    SettingsObject general_windows_state(name_ + "_state");

    std::string window_state = general_windows_state.Check(
        "window_state", saveState().toBase64().toStdString());
    GF_UI_LOG_DEBUG("restore main window state: {}", window_state);

    // state sets pos & size of dock-widgets
    this->restoreState(
        QByteArray::fromBase64(QByteArray::fromStdString(window_state)));

    bool window_save = general_windows_state.Check("window_save", true);

    // Restore window size & location
    if (window_save) {
      int x = general_windows_state.Check("window_pos").Check("x", 100),
          y = general_windows_state.Check("window_pos").Check("y", 100);

      pos_ = {x, y};

      int width =
              general_windows_state.Check("window_size").Check("width", 800),
          height =
              general_windows_state.Check("window_size").Check("height", 450);

      size_ = {width, height};

      if (this->parent() != nullptr) {
        GF_UI_LOG_DEBUG("parent address: {}",
                        static_cast<void *>(this->parent()));

        QPoint parent_pos = {0, 0};
        QSize parent_size = {0, 0};

        auto *parent_dialog = qobject_cast<QDialog *>(this->parent());
        if (parent_dialog != nullptr) {
          parent_pos = parent_dialog->pos();
          parent_size = parent_dialog->size();
        }

        auto *parent_window = qobject_cast<QMainWindow *>(this->parent());
        if (parent_window != nullptr) {
          parent_pos = parent_window->pos();
          parent_size = parent_window->size();
        }

        GF_UI_LOG_DEBUG("parent pos x: {} y: {}", parent_pos.x(),
                        parent_pos.y());

        GF_UI_LOG_DEBUG("parent size width: {} height: {}", parent_size.width(),
                        parent_size.height());

        if (parent_pos != QPoint{0, 0}) {
          QPoint parent_center{parent_pos.x() + parent_size.width() / 2,
                               parent_pos.y() + parent_size.height() / 2};

          pos_ = {parent_center.x() - size_.width() / 2,
                  parent_center.y() - size_.height() / 2};
        }
      }

      this->move(pos_);
      this->resize(size_);
    }

    // appearance
    SettingsObject general_settings_state("general_settings_state");

    int width = general_settings_state.Check("icon_size").Check("width", 24),
        height = general_settings_state.Check("icon_size").Check("height", 24);
    GF_UI_LOG_DEBUG("icon size: {} {}", width, height);

    icon_size_ = {width, height};
    font_size_ = general_settings_state.Check("font_size", 10);

    this->setIconSize(icon_size_);

    // icon_style
    int s_icon_style =
        general_settings_state.Check("icon_style", Qt::ToolButtonTextUnderIcon);
    this->setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(s_icon_style));
    icon_style_ = toolButtonStyle();

  } catch (...) {
    GF_UI_LOG_ERROR(name_, "error");
  }
}

void GpgFrontend::UI::GeneralMainWindow::slot_save_settings() noexcept {
  try {
    GF_UI_LOG_DEBUG("save main window state, name: {}", name_);
    SettingsObject general_windows_state(name_ + "_state");

    // window position and size
    general_windows_state["window_state"] =
        saveState().toBase64().toStdString();
    general_windows_state["window_pos"]["x"] = pos().x();
    general_windows_state["window_pos"]["y"] = pos().y();

    // update size of current dialog
    size_ = this->size();

    general_windows_state["window_size"]["width"] = size_.width();
    general_windows_state["window_size"]["height"] = size_.height();
    general_windows_state["window_save"] = true;

    SettingsObject general_settings_state("general_settings_state");

    // icon size
    general_settings_state["icon_size"]["width"] = icon_size_.width();
    general_settings_state["icon_size"]["height"] = icon_size_.height();

    // font size
    general_settings_state["font_size"] = font_size_;

    // tool button style
    general_settings_state["icon_style"] = this->toolButtonStyle();

  } catch (...) {
    GF_UI_LOG_ERROR(name_, "error");
  }
}
