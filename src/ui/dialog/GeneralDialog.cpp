/*
 * Copyright (c) 2022. Saturneric
 *
 *  This file is part of GpgFrontend.
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
 */

#include "GeneralDialog.h"

#include "ui/struct/SettingsObject.h"

GpgFrontend::UI::GeneralDialog::GeneralDialog(std::string name, QWidget *parent)
    : name_(std::move(name)), QDialog(parent) {
  slot_restore_settings();
  connect(this, &QDialog::finished, this, &GeneralDialog::slot_save_settings);
}

GpgFrontend::UI::GeneralDialog::~GeneralDialog() = default;

void GpgFrontend::UI::GeneralDialog::slot_restore_settings() noexcept {
  try {
    LOG(INFO) << name_ << _("Called");

    SettingsObject general_windows_state(name_ + "_dialog_state");

    bool window_save = general_windows_state.Check("window_save", true);

    // Restore window size & location
    if (window_save) {
      int x = general_windows_state.Check("window_pos").Check("x", 100),
          y = general_windows_state.Check("window_pos").Check("y", 100);

      pos_ = {x, y};

      int width =
              general_windows_state.Check("window_size").Check("width", 400),
          height =
              general_windows_state.Check("window_size").Check("height", 247);

      size_ = {width, height};

      if (this->parent() != nullptr) {
        LOG(INFO) << "parent address" << this->parent();

        QPoint parent_pos = {0, 0};
        QSize parent_size = {0, 0};

        auto *parent_widget = qobject_cast<QWidget *>(this->parent());
        if (parent_widget != nullptr) {
          parent_pos = parent_widget->pos();
          parent_size = parent_widget->size();
        }

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

        LOG(INFO) << "parent pos x:" << parent_pos.x()
                  << "y:" << parent_pos.y();

        LOG(INFO) << "parent size width:" << parent_size.width()
                  << "height:" << parent_size.height();

        LOG(INFO) << "this dialog size width:" << size_.width()
                  << "height:" << size_.height();

        if (parent_pos != QPoint{0, 0}) {
          QPoint parent_center{parent_pos.x() + parent_size.width() / 2,
                               parent_pos.y() + parent_size.height() / 2};

          pos_ = {parent_center.x() - size_.width() / 2,
                  parent_center.y() - size_.height() / 2};

          // record parent_pos_
          this->parent_pos_ = parent_pos;
          this->parent_size_ = parent_size;
        }
      }

      this->move(pos_);
      this->resize(size_);
    }

  } catch (...) {
    LOG(ERROR) << name_ << "error";
  }
}

void GpgFrontend::UI::GeneralDialog::slot_save_settings() noexcept {
  try {
    LOG(INFO) << name_ << _("Called");

    SettingsObject general_windows_state(name_ + "_dialog_state");

    // window position and size
    general_windows_state["window_pos"]["x"] = pos().x();
    general_windows_state["window_pos"]["y"] = pos().y();

    // update size of current dialog
    size_ = this->size();

    general_windows_state["window_size"]["width"] = size_.width();
    general_windows_state["window_size"]["height"] = size_.height();
    general_windows_state["window_save"] = true;

  } catch (...) {
    LOG(ERROR) << name_ << "error";
  }
}

void GpgFrontend::UI::GeneralDialog::setPosCenterOfScreen() {
  auto *screen = QGuiApplication::primaryScreen();
  QRect geo = screen->availableGeometry();
  int screen_width = geo.width();
  int screen_height = geo.height();

  LOG(INFO) << "primary screen available geometry" << screen_width
            << screen_height;

  pos_ = QPoint((screen_width - QWidget::width()) / 2,
                (screen_height - QWidget::height()) / 2);
  this->move(pos_);
}

/**
 * @brief
 *
 */
void GpgFrontend::UI::GeneralDialog::movePosition2CenterOfParent() {
  LOG(INFO) << "parent pos x:" << parent_pos_.x() << "y:" << parent_pos_.y();

  LOG(INFO) << "parent size width:" << parent_size_.width()
            << "height:" << parent_size_.height();

  if (parent_pos_ != QPoint{0, 0} && parent_size_ != QSize{0, 0}) {
    LOG(INFO) << "update current dialog position now";
    QPoint parent_center{parent_pos_.x() + parent_size_.width() / 2,
                         parent_pos_.y() + parent_size_.height() / 2};

    // update size of current dialog
    size_ = this->size();

    pos_ = {parent_center.x() - size_.width() / 2,
            parent_center.y() - size_.height() / 2};
    this->move(pos_);
  }
}