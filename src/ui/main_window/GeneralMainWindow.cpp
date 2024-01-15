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

#include "ui/struct/SettingsObject.h"
#include "ui/struct/settings/AppearanceSO.h"
#include "ui/struct/settings/WindowStateSO.h"

namespace GpgFrontend::UI {

class GeneralWindowState {};

GpgFrontend::UI::GeneralMainWindow::GeneralMainWindow(QString name,
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
    WindowStateSO window_state(SettingsObject(name_ + "_state"));

    GF_UI_LOG_DEBUG("restore main window state: {}",
                    window_state.window_state_data);

    // state sets pos & size of dock-widgets
    this->restoreState(
        QByteArray::fromBase64(window_state.window_state_data.toUtf8()));

    // restore window size & location
    if (window_state.window_save) {
      pos_ = {window_state.x, window_state.y};
      size_ = {window_state.width, window_state.height};

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
    AppearanceSO appearance(SettingsObject("general_settings_state"));

    icon_size_ = {appearance.tool_bar_icon_width,
                  appearance.tool_bar_icon_height};
    font_size_ = appearance.info_board_font_size;

    this->setIconSize(icon_size_);
    this->setToolButtonStyle(appearance.tool_bar_button_style);
    icon_style_ = toolButtonStyle();

  } catch (...) {
    GF_UI_LOG_ERROR("gernal main window: {}, caught exception", name_);
  }
}

void GpgFrontend::UI::GeneralMainWindow::slot_save_settings() noexcept {
  try {
    GF_UI_LOG_DEBUG("save main window state, name: {}", name_);
    SettingsObject general_windows_state(name_ + "_state");

    // update geo of current dialog
    size_ = this->size();
    pos_ = this->pos();

    WindowStateSO window_state;
    window_state.x = pos_.x();
    window_state.y = pos_.y();
    window_state.width = size_.width();
    window_state.height = size_.height();
    window_state.window_save = true;
    window_state.window_state_data = this->saveState().toBase64();

    general_windows_state.Store(window_state.Json());
  } catch (...) {
    GF_UI_LOG_ERROR("gernal main window: {}, caught exception", name_);
  }
}

}  // namespace GpgFrontend::UI