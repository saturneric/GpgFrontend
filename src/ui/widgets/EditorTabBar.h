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
 * @brief The editor's tab strip: one row that scrolls, never wraps.
 *
 * The wheel and the trackpad move the strip instead of switching documents;
 * each tab has its own close button, which shows a dot while the document has
 * unsaved changes, so the title never moves when the pointer reaches it.
 */
class GF_UI_EXPORT EditorTabBar : public QTabBar {
  Q_OBJECT
 public:
  explicit EditorTabBar(QWidget* parent = nullptr);

  /// Show (or stop showing) the unsaved-changes dot of tab @p index.
  void SetTabModified(int index, bool modified);

  /// Whether tab @p index shows the unsaved-changes dot.
  [[nodiscard]] auto TabModified(int index) const -> bool;

  /**
   * @brief Scroll the strip by @p steps tabs, without changing the current
   *        tab: positive towards the first tab, negative towards the last.
   */
  void ScrollBy(int steps);

 signals:
  /// A double click on the strip outside any tab.
  void SignalNewTabRequested();

 protected:
  void tabInserted(int index) override;
  [[nodiscard]] auto tabSizeHint(int index) const -> QSize override;
  [[nodiscard]] auto minimumTabSizeHint(int index) const -> QSize override;
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;

 private:
  /// The scroll arrow that reveals the tabs towards the first (@p back) or
  /// the last one; nullptr while the strip has nothing to scroll.
  [[nodiscard]] auto scroll_button(bool back) const -> QAbstractButton*;
  [[nodiscard]] auto close_button_side() const -> QTabBar::ButtonPosition;

  int wheel_rest_ = 0;  ///< wheel travel not yet worth a whole tab
  int middle_pressed_ = -1;
};

}  // namespace GpgFrontend::UI
