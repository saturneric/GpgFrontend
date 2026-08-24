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

#include <QPlainTextEdit>
#include <QWidget>

namespace GpgFrontend::UI {

class PlainTextEditor;

class LineNumberArea : public QWidget {
 public:
  explicit LineNumberArea(PlainTextEditor* editor);

  [[nodiscard]] auto sizeHint() const -> QSize override;

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  PlainTextEditor* editor_;
};

class PlainTextEditor : public QPlainTextEdit {
 public:
  /**
   * @brief Construct a new Plain Text Editor object
   *
   * @param parent
   */
  explicit PlainTextEditor(QWidget* parent = nullptr);

  /**
   * @brief
   *
   * @param event
   */
  void LineNumberAreaPaintEvent(QPaintEvent* event);

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] auto LineNumberAreaWidth() const -> int;

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void resizeEvent(QResizeEvent* event) override;

  /**
   * @brief Keeps the line number gutter on the side the text starts at.
   *
   * The gutter is placed next to the viewport, so it has to move whenever the
   * editor is laid out the other way round. Without this it would only follow
   * on the next resize.
   *
   * @param event
   */
  void changeEvent(QEvent* event) override;

 private slots:
  /**
   * @brief
   *
   * @param new_block_count
   */
  void slot_update_line_number_area_width(int new_block_count);

  /**
   * @brief
   *
   * @param rect
   * @param dy
   */
  void slot_update_line_number_area(const QRect& rect, int dy);

  /**
   * @brief
   *
   */
  void slot_highlight_current_line();

 private:
  QWidget* line_number_area_ = nullptr;

  /**
   * @brief Places the line number gutter beside the viewport.
   *
   * Anchored to the viewport rather than to the editor's own contents rect, so
   * it follows the strip that slot_update_line_number_area_width() reserved,
   * whichever side that was, and stays clear of the vertical scroll bar, which
   * does swap sides under a right-to-left layout.
   */
  void update_line_number_area_geometry();
};

}  // namespace GpgFrontend::UI