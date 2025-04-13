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

#include "GeneralMainWindow.h"

#include "core/model/SettingsObject.h"
#include "ui/UIModuleManager.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui/struct/settings_object/WindowStateSO.h"

namespace GpgFrontend::UI {

class GeneralWindowState {};

GpgFrontend::UI::GeneralMainWindow::GeneralMainWindow(QString id,
                                                      QWidget *parent)
    : QMainWindow(parent), id_(std::move(id)) {
  UIModuleManager::GetInstance().RegisterQObject(id_, this);
  slot_restore_settings();

  // should delete itself at closing by default
  setAttribute(Qt::WA_DeleteOnClose);
}

GpgFrontend::UI::GeneralMainWindow::~GeneralMainWindow() = default;

void GpgFrontend::UI::GeneralMainWindow::closeEvent(QCloseEvent *event) {
  slot_save_settings();
  QMainWindow::closeEvent(event);
}

void GpgFrontend::UI::GeneralMainWindow::slot_restore_settings() noexcept {
  try {
    WindowStateSO window_state(SettingsObject(id_ + "_state"));

    if (!window_state.window_state_data.isEmpty()) {
      // state sets pos & size of dock-widgets
      this->restoreState(
          QByteArray::fromBase64(window_state.window_state_data.toUtf8()));
    }

    this->setMinimumSize(800, 600);

    // restore window size & location
    if (window_state.window_save) {
      pos_ = {window_state.x, window_state.y};
      size_ = {window_state.width, window_state.height};

      if (this->parent() != nullptr) {
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

        if (parent_pos != QPoint{0, 0}) {
          QPoint const parent_center{parent_pos.x() + parent_size.width() / 2,
                                     parent_pos.y() + parent_size.height() / 2};

          pos_ = {parent_center.x() - size_.width() / 2,
                  parent_center.y() - size_.height() / 2};
        }
      }

      if (size_.width() < 800) size_.setWidth(800);
      if (size_.height() < 600) size_.setHeight(600);

      this->move(pos_);
      this->resize(size_);
    } else {
      movePosition2CenterOfParent();
    }

    // appearance
    AppearanceSO const appearance(SettingsObject("general_settings_state"));

    icon_size_ = {appearance.tool_bar_icon_width,
                  appearance.tool_bar_icon_height};
    font_size_ = appearance.info_board_font_size;

    this->setIconSize(icon_size_);
    this->setToolButtonStyle(appearance.tool_bar_button_style);
    icon_style_ = toolButtonStyle();

  } catch (...) {
    LOG_W() << "general main window: " << id_ << ", caught exception";
  }
}

void GpgFrontend::UI::GeneralMainWindow::slot_save_settings() noexcept {
  try {
    SettingsObject general_windows_state(id_ + "_state");

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
    LOG_W() << "general main window: " << id_ << ", caught exception";
  }
}

void GeneralMainWindow::setPosCenterOfScreen() {
  // update cache
  update_rect_cache();

  // update rect of current dialog
  rect_ = this->geometry();
  this->move(screen_rect_.center() -
             QPoint(rect_.width() / 2, rect_.height() / 2));
}

void GeneralMainWindow::movePosition2CenterOfParent() {
  // update cache
  update_rect_cache();

  if (parent_rect_.topLeft() != QPoint{0, 0} &&
      parent_rect_.size() != QSize{0, 0}) {
    if (rect_.width() <= 0) rect_.setWidth(100);
    if (rect_.height() <= 0) rect_.setHeight(100);

    QPoint target_position =
        parent_rect_.center() - QPoint(rect_.width() / 2, rect_.height() / 2);

    this->move(target_position);
  } else {
    setPosCenterOfScreen();
  }
}

void GeneralMainWindow::update_rect_cache() {
  // update size of current dialog
  rect_ = this->geometry();

  auto *screen = this->window()->screen();
  screen_rect_ = screen->availableGeometry();

  // read pos and size from parent
  if (this->parent() != nullptr) {
    QRect parent_rect;

    auto *parent_widget = qobject_cast<QWidget *>(this->parent());
    if (parent_widget != nullptr) {
      parent_rect = parent_widget->geometry();
    } else {
      auto *parent_dialog = qobject_cast<QDialog *>(this->parent());
      if (parent_dialog != nullptr) {
        parent_rect = parent_dialog->geometry();
      } else {
        auto *parent_window = qobject_cast<QMainWindow *>(this->parent());
        if (parent_window != nullptr) {
          parent_rect = parent_window->geometry();
        }
      }
    }
    parent_rect_ = parent_rect;
  } else {
    // reset parent's pos and size
    this->parent_rect_ = QRect{0, 0, 0, 0};
  }
}

auto GeneralMainWindow::GetId() const -> QString { return id_; }
}  // namespace GpgFrontend::UI