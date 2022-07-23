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

      this->move({x, y});
      pos_ = {x, y};

      int width =
              general_windows_state.Check("window_size").Check("width", 400),
          height =
              general_windows_state.Check("window_size").Check("height", 247);

      this->resize({width, height});
      size_ = {width, height};

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

    general_windows_state["window_size"]["width"] = size_.width();
    general_windows_state["window_size"]["height"] = size_.height();
    general_windows_state["window_save"] = true;

  } catch (...) {
    LOG(ERROR) << name_ << "error";
  }
}
