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

/**
 *
 */
class GeneralMainWindow : public QMainWindow {
 public:
  /**
   *
   * @param name
   */
  explicit GeneralMainWindow(QString id, QWidget* parent = nullptr);

  /**
   *
   */
  ~GeneralMainWindow() override;

  /**
   * @brief Get the Id object
   *
   * @return QString
   */
  [[nodiscard]] auto GetId() const -> QString;

 protected:
  /**
   *
   * @param event
   */
  void closeEvent(QCloseEvent* event) override;

  /**
   *
   */
  void setPosCenterOfScreen();

  /**
   * @brief
   *
   */
  void movePosition2CenterOfParent();

  QSize icon_size_;                 ///<
  int font_size_{};                 ///<
  Qt::ToolButtonStyle icon_style_;  ///<

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
   * @brief
   *
   */
  void update_rect_cache();

 private:
  QString id_;  ///<
  QPoint pos_;  ///<
  QSize size_;  ///<
  QRect rect_;
  QRect screen_rect_;
  QRect parent_rect_;
};
}  // namespace GpgFrontend::UI
