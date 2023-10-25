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

#include "GeneralDialog.h"

#include "ui/struct/SettingsObject.h"

GpgFrontend::UI::GeneralDialog::GeneralDialog(std::string name, QWidget *parent)
    : QDialog(parent), name_(std::move(name)) {
  slot_restore_settings();
  connect(this, &QDialog::finished, this, &GeneralDialog::slot_save_settings);
}

GpgFrontend::UI::GeneralDialog::~GeneralDialog() = default;

void GpgFrontend::UI::GeneralDialog::slot_restore_settings() noexcept {
  try {
    update_rect_cache();

    SettingsObject general_windows_state(name_ + "_dialog_state");
    bool window_save = general_windows_state.Check("window_save", false);

    // Restore window size & location
    if (window_save) {
      int x = general_windows_state.Check("window_pos").Check("x", 0),
          y = general_windows_state.Check("window_pos").Check("y", 0);
      SPDLOG_DEBUG("stored dialog pos, x: {}, y: {}", x, y);

      QPoint relative_pos = {x, y};
      QPoint pos = parent_rect_.topLeft() + relative_pos;
      SPDLOG_DEBUG("relative dialog pos, x: {}, y: {}", relative_pos.x(),
                   relative_pos.y());

      int width = general_windows_state.Check("window_size").Check("width", 0),
          height =
              general_windows_state.Check("window_size").Check("height", 0);
      SPDLOG_DEBUG("stored dialog size, width: {}, height: {}", width, height);

      QRect target_rect_ = {pos.x(), pos.y(), width, height};
      SPDLOG_DEBUG("dialog stored target rect, width: {}, height: {}", width,
                   height);

      // check for valid
      if (width > 0 && height > 0 && screen_rect_.contains(target_rect_)) {
        this->setGeometry(target_rect_);
        this->rect_restored_ = true;
      }
    }

  } catch (...) {
    SPDLOG_ERROR("error at restoring settings");
  }
}

void GpgFrontend::UI::GeneralDialog::slot_save_settings() noexcept {
  try {
    SettingsObject general_windows_state(name_ + "_dialog_state");

    update_rect_cache();

    SPDLOG_DEBUG("dialog pos, x: {}, y: {}", rect_.x(), rect_.y());
    SPDLOG_DEBUG("dialog size, width: {}, height: {}", rect_.width(),
                 rect_.height());

    // window position relative to parent
    auto relative_pos = rect_.topLeft() - parent_rect_.topLeft();
    SPDLOG_DEBUG("store dialog pos, x: {}, y: {}", relative_pos.x(),
                 relative_pos.y());

    general_windows_state["window_pos"]["x"] = relative_pos.x();
    general_windows_state["window_pos"]["y"] = relative_pos.y();

    general_windows_state["window_size"]["width"] = rect_.width();
    general_windows_state["window_size"]["height"] = rect_.height();
    general_windows_state["window_save"] = true;

  } catch (...) {
    SPDLOG_ERROR(name_, "error");
  }
}

void GpgFrontend::UI::GeneralDialog::setPosCenterOfScreen() {
  update_rect_cache();

  int screen_width = screen_rect_.width();
  int screen_height = screen_rect_.height();
  SPDLOG_DEBUG("dialog current screen available geometry", screen_width,
               screen_height);

  // update rect of current dialog
  rect_ = this->geometry();
  this->move(screen_rect_.center() -
             QPoint(rect_.width() / 2, rect_.height() / 2));
}

/**
 * @brief
 *
 */
void GpgFrontend::UI::GeneralDialog::movePosition2CenterOfParent() {
  // update cache
  update_rect_cache();

  // log for debug
  SPDLOG_DEBUG("parent pos x: {} y: {}", parent_rect_.x(), parent_rect_.y());
  SPDLOG_DEBUG("parent size width: {}, height: {}", parent_rect_.width(),
               parent_rect_.height());
  SPDLOG_DEBUG("parent center pos x: {}, y: {}", parent_rect_.center().x(),
               parent_rect_.center().y());
  SPDLOG_DEBUG("dialog pos x: {} y: {}", rect_.x(), rect_.y());
  SPDLOG_DEBUG("dialog size width: {} height: {}", rect_.width(),
               rect_.height());

  if (parent_rect_.topLeft() != QPoint{0, 0} &&
      parent_rect_.size() != QSize{0, 0}) {
    if (rect_.width() <= 0) rect_.setWidth(100);
    if (rect_.height() <= 0) rect_.setHeight(100);

    QPoint target_position =
        parent_rect_.center() - QPoint(rect_.width() / 2, rect_.height() / 2);

    SPDLOG_DEBUG("update position to parent's center, target pos, x:{}, y: {}",
                 target_position.x(), target_position.y());

    this->move(target_position);
  } else {
    setPosCenterOfScreen();
  }
}

/**
 *
 */
void GpgFrontend::UI::GeneralDialog::update_rect_cache() {
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

/**
 * @brief
 *
 */
bool GpgFrontend::UI::GeneralDialog::isRectRestored() { return rect_restored_; }

/**
 * @brief
 *
 */
void GpgFrontend::UI::GeneralDialog::showEvent(QShowEvent *event) {
  SPDLOG_DEBUG("General Dialog named {} is about to show, caught show event",
               name_);

  // default position strategy
  if (!isRectRestored()) movePosition2CenterOfParent();

  QWidget::showEvent(event);
}