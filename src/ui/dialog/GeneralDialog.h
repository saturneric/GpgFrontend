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

class GeneralDialog : public QDialog {
 public:
  /**
   *
   * @param name
   */
  explicit GeneralDialog(QString name, QWidget *parent = nullptr);

  /**
   *
   */
  ~GeneralDialog() override;

  /**
   * @brief Prepare the dialog's geometry before the native window is mapped.
   *
   * On the first show we polish the widget, activate its layout and resolve the
   * final size/position up front. Otherwise platforms that present eagerly
   * (Windows) briefly map the window at its unpolished size hint and paint the
   * unfilled backing store, so the user sees a small white frame that is then
   * laid out again into the real dialog.
   *
   * @param visible
   */
  void setVisible(bool visible) override;

 protected:
  /**
   *
   */
  void setPosCenterOfScreen();

  /**
   * @brief
   *
   */
  void movePosition2CenterOfParent();

  /**
   * @brief
   *
   */
  [[nodiscard]] auto isRectRestored() const -> bool;

  /**
   * @brief
   *
   * @param event
   */
  void closeEvent(QCloseEvent *event) override;

  /**
   * @brief
   *
   * @param event
   */
  void showEvent(QShowEvent *event) override;

 private slots:
  /**
   *
   */
  void slot_restore_settings() noexcept;

  /**
   *
   */
  void slot_save_settings() noexcept;

  /**
   * @brief Pull the window frame back onto the available screen area.
   *
   * Runs deferred after the native window is mapped, once the window manager
   * has added the title bar and the frame margins become known. The pre-map
   * clamping only constrains the client rect, so without this correction a
   * dialog centered against a full-screen parent can end up with its title bar
   * (and close button) above the top of the screen.
   */
  void slot_ensure_frame_on_screen();

 private:
  /**
   *
   */
  void update_rect_cache();

  /**
   * @brief Polish, size and restore the dialog's geometry. Runs exactly once,
   * before the native window is first mapped.
   */
  void prepare_first_show();

  QString name_;  ///<
  QRect rect_;
  QRect parent_rect_;
  QRect screen_rect_;
  bool rect_restored_ = false;
  bool first_show_handled_ = false;
};
}  // namespace GpgFrontend::UI
