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

#include "GeneralDialog.h"

#include "core/model/SettingsObject.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/struct/settings_object/WindowStateSO.h"

namespace GpgFrontend::UI {

namespace {

auto SafeAvailableGeometry(QWidget *widget) -> QRect {
  QScreen *screen = nullptr;

  if (widget != nullptr && widget->windowHandle() != nullptr) {
    screen = widget->windowHandle()->screen();
  }

  if (screen == nullptr && widget != nullptr && widget->screen() != nullptr) {
    screen = widget->screen();
  }

  if (screen == nullptr) {
    screen = QGuiApplication::primaryScreen();
  }

  if (screen == nullptr) {
    return {0, 0, 1024, 768};
  }

  return screen->availableGeometry();
}

}  // namespace

GeneralDialog::GeneralDialog(QString name, QWidget *parent)
    : QDialog(parent), name_(std::move(name)) {
  // should delete itself at closing by default
  setAttribute(Qt::WA_DeleteOnClose);
}

void GeneralDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);

  is_first_show_ = true;
  if (is_first_show_) {
    is_first_show_ = false;
    slot_restore_settings();
  }
}

GeneralDialog::~GeneralDialog() = default;

void GeneralDialog::slot_restore_settings() noexcept {
  try {
    update_rect_cache();

    SettingsObject general_windows_state(name_ + "_dialog_state");
    auto window_state = WindowStateSO(general_windows_state);

    if (!window_state.window_save) {
      return;
    }

    int width = window_state.width;
    int height = window_state.height;

    if (width <= 0 || height <= 0) return;

    QPoint target_pos{window_state.x, window_state.y};
    QRect target_rect{target_pos, QSize(width, height)};

    update_rect_cache();
    target_rect = ClampRectToAvailableGeometry(target_rect, screen_rect_);

    this->setGeometry(target_rect);
    rect_restored_ = true;

  } catch (...) {
    FLOG_W("error at restoring settings");
  }
}

void GeneralDialog::slot_save_settings() noexcept {
  try {
    SettingsObject general_windows_state(name_ + "_dialog_state");

    update_rect_cache();

    QRect current_geometry = this->frameGeometry();

    WindowStateSO window_state;
    window_state.x = current_geometry.x();
    window_state.y = current_geometry.y();

    window_state.width = this->geometry().width();
    window_state.height = this->geometry().height();
    window_state.window_save = true;

    general_windows_state.Store(window_state.Json());

  } catch (...) {
    LOG_W() << "general dialog: " << name_ << ", caught exception";
  }
}

void GeneralDialog::closeEvent(QCloseEvent *event) {
  slot_save_settings();
  QDialog::closeEvent(event);
}

void GeneralDialog::setPosCenterOfScreen() {
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

  this->setGeometry(target_rect);
}

void GeneralDialog::movePosition2CenterOfParent() {
  update_rect_cache();

  QRect target_rect = rect_;

  if (target_rect.width() <= 0) {
    target_rect.setWidth(std::max(width(), 100));
  }

  if (target_rect.height() <= 0) {
    target_rect.setHeight(std::max(height(), 100));
  }

  const bool has_parent = !parent_rect_.isNull() && parent_rect_.isValid();

  if (has_parent) {
    target_rect.moveCenter(parent_rect_.center());
    target_rect.translate(24, 24);
  } else {
    target_rect.moveCenter(screen_rect_.center());
  }

  target_rect = ClampRectToAvailableGeometry(target_rect, screen_rect_);
  setGeometry(target_rect);
}

void GeneralDialog::update_rect_cache() {
  rect_ = geometry();

  auto *parent_widget = qobject_cast<QWidget *>(parent());

  if (parent_widget != nullptr) {
    QWidget *anchor_widget = parent_widget->window();

    if (anchor_widget == nullptr) {
      anchor_widget = parent_widget;
    }

    parent_rect_ =
        QRect(anchor_widget->mapToGlobal(QPoint(0, 0)), anchor_widget->size());

    screen_rect_ = SafeAvailableGeometry(anchor_widget);
    return;
  }

  parent_rect_ = QRect{0, 0, 0, 0};
  screen_rect_ = SafeAvailableGeometry(this);
}

auto GeneralDialog::isRectRestored() const -> bool { return rect_restored_; }

}  // namespace GpgFrontend::UI