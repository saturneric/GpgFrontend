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
 * @brief The chrome style sheet every GpgFrontend main window wears.
 *
 * Names only palette() roles, so the one string is correct on the light and on
 * the hand-rolled dark palette alike — this codebase has no theme engine, only
 * the palette swap in GpgFrontendUIInit.cpp, so a literal colour here would
 * silently break dark mode.
 *
 * Handed out as a string rather than applied directly so it stays verifiable
 * without a widget: the unit tests run off the GUI thread and cannot construct
 * one.
 */
[[nodiscard]] auto GF_UI_EXPORT MainWindowChromeStyleSheet() -> QString;

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

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto RestoreSettingsOnce() noexcept -> bool;

 protected:
  /**
   *
   * @param event
   */
  void closeEvent(QCloseEvent* event) override;

  /**
   * @brief
   *
   * @param event
   */
  void showEvent(QShowEvent* event) override;

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
   *
   */
  void restoreSettings() noexcept;

  /**
   * @brief Wear the shared chrome.
   *
   * Subclasses that need extra rules should append to
   * MainWindowChromeStyleSheet() themselves rather than growing the shared
   * string: a style sheet cascades down the parent chain, so a rule added here
   * for one window reaches every widget the others own too.
   */
  void initWindowStyle();

  /**
   * @brief Re-read the appearance preferences into icon_size_ / icon_style_ /
   * font_size_ and apply them to the window.
   *
   * Called once at construction and again whenever the settings change, so a
   * window that is already open does not keep serving stale metrics.
   */
  void reloadAppearanceSettings();

  /**
   * @brief
   *
   */
  auto restoreWindowState() noexcept -> bool;

  QSize icon_size_;                 ///<
  int font_size_{};                 ///<
  Qt::ToolButtonStyle icon_style_;  ///<

 private slots:

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
  bool settings_restored_ = false;
};
}  // namespace GpgFrontend::UI
