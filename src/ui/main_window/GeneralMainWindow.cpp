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
#include "ui/UserInterfaceUtils.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui/struct/settings_object/WindowStateSO.h"


namespace GpgFrontend::UI {

GeneralMainWindow::GeneralMainWindow(QString id, QWidget *parent)
    : QMainWindow(parent), id_(std::move(id)) {
  // restore appearance settings
  AppearanceSO appearance(SettingsObject("general_settings_state"));

  icon_size_ = {appearance.tool_bar_icon_width,
                appearance.tool_bar_icon_height};
  font_size_ = appearance.info_board_font_size;
  icon_style_ = appearance.tool_bar_button_style;

  this->setIconSize(icon_size_);
  this->setToolButtonStyle(icon_style_);

  // should delete itself at closing by default
  setAttribute(Qt::WA_DeleteOnClose);
}

GeneralMainWindow::~GeneralMainWindow() = default;

void GeneralMainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);
  RestoreSettingsOnce();
}

auto GeneralMainWindow::RestoreSettingsOnce() noexcept -> bool {
  if (settings_restored_) return false;

  settings_restored_ = true;
  restoreSettings();
  return true;
}

void GeneralMainWindow::closeEvent(QCloseEvent *event) {
  // save size and position
  slot_save_settings();

  QMainWindow::closeEvent(event);
}

void GeneralMainWindow::restoreSettings() noexcept {
  try {
    WindowStateSO window_state(SettingsObject(id_ + "_state"));

    update_rect_cache();

    const int min_width =
        std::min(800, static_cast<int>(screen_rect_.width() * 0.95));
    const int min_height =
        std::min(600, static_cast<int>(screen_rect_.height() * 0.95));

    setMinimumSize(min_width, min_height);

    // restore window size & location
    if (window_state.window_save) {
      pos_ = {window_state.x, window_state.y};
      size_ = {window_state.width, window_state.height};

      if (this->parent() != nullptr) {
        auto *parent_widget = qobject_cast<QWidget *>(parent());
        if (parent_widget != nullptr) {
          const auto parent_rect = parent_widget->geometry();

          if (parent_rect.isValid() && !parent_rect.isNull()) {
            pos_ = parent_rect.center() -
                   QPoint(size_.width() / 2, size_.height() / 2);
          }

          // add a small offset to avoid completely overlapping with parent if
          // the window is opened multiple times
          pos_ += QPoint(24, 24);
        }
      }

      const int min_width =
          std::min(800, static_cast<int>(screen_rect_.width() * 0.95));
      const int min_height =
          std::min(600, static_cast<int>(screen_rect_.height() * 0.95));

      setMinimumSize(min_width, min_height);

      update_rect_cache();

      QRect target_rect{pos_, size_};
      target_rect = ClampRectToAvailableGeometry(target_rect, screen_rect_);

      setGeometry(target_rect);
    } else {
      LOG_D() << "general main window: " << id_
              << ", no window geometry data to restore, move to center of "
                 "parent or screen";
      if (this->parent() != nullptr) {
        movePosition2CenterOfParent();
      } else {
        setPosCenterOfScreen();
      }
    }

  } catch (...) {
    LOG_W() << "general main window: " << id_ << ", caught exception";
  }
}

void GeneralMainWindow::slot_save_settings() noexcept {
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
  update_rect_cache();

  QRect target_rect = rect_;

  if (target_rect.width() <= 0) {
    target_rect.setWidth(std::max(width(), 100));
  }

  if (target_rect.height() <= 0) {
    target_rect.setHeight(std::max(height(), 100));
  }

  target_rect.moveCenter(screen_rect_.center());
  target_rect = ClampRectToAvailableGeometry(target_rect, screen_rect_);

  setGeometry(target_rect);
}

void GeneralMainWindow::movePosition2CenterOfParent() {
  update_rect_cache();

  QRect target_rect = rect_;

  if (target_rect.width() <= 0) {
    target_rect.setWidth(std::max(width(), 100));
  }

  if (target_rect.height() <= 0) {
    target_rect.setHeight(std::max(height(), 100));
  }

  if (!parent_rect_.isNull() && parent_rect_.isValid()) {
    target_rect.moveCenter(parent_rect_.center());
  } else {
    target_rect.moveCenter(screen_rect_.center());
  }

  // add a small offset to avoid completely overlapping with parent if the
  // window is opened multiple times
  target_rect.translate(24, 24);
  target_rect = ClampRectToAvailableGeometry(target_rect, screen_rect_);
  setGeometry(target_rect);
}

void GeneralMainWindow::update_rect_cache() {
  rect_ = geometry();

  auto *parent_widget = qobject_cast<QWidget *>(parent());

  if (parent_widget != nullptr) {
    parent_rect_ = parent_widget->geometry();

    auto *screen = parent_widget->screen();

    if (screen == nullptr && parent_widget->windowHandle() != nullptr) {
      screen = parent_widget->windowHandle()->screen();
    }

    if (screen == nullptr) {
      screen = QGuiApplication::primaryScreen();
    }

    screen_rect_ = screen != nullptr ? screen->availableGeometry()
                                     : QRect(0, 0, 1024, 768);
    return;
  }

  parent_rect_ = QRect{0, 0, 0, 0};

  auto *screen = this->screen();

  if (screen == nullptr && this->windowHandle() != nullptr) {
    screen = this->windowHandle()->screen();
  }

  if (screen == nullptr) {
    screen = QGuiApplication::primaryScreen();
  }

  screen_rect_ =
      screen != nullptr ? screen->availableGeometry() : QRect(0, 0, 1024, 768);
}

auto GeneralMainWindow::GetId() const -> QString { return id_; }

auto GeneralMainWindow::restoreWindowState() noexcept -> bool {
  try {
    WindowStateSO window_state(SettingsObject(id_ + "_state"));

    if (window_state.window_state_data.isEmpty()) {
      LOG_D() << "general main window: " << id_
              << ", no window state data to restore, skip restoring state";
      return false;
    }

    return restoreState(
        QByteArray::fromBase64(window_state.window_state_data.toUtf8()));
  } catch (...) {
    LOG_W() << "general main window: " << id_
            << ", caught exception while restoring main window state";
    return false;
  }
}
}  // namespace GpgFrontend::UI
