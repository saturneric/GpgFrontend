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
 * @brief An on/off switch: its label on the left, a sliding knob on the right.
 *
 * For a setting that changes what the window shows, such as Expert Mode,
 * where a checkbox reads like one more option among the others. A checkable
 * QAbstractButton, so toggled(), the keyboard and accessibility come from Qt;
 * the whole width, label included, takes the click.
 */
class GF_UI_EXPORT ToggleSwitch : public QAbstractButton {
  Q_OBJECT

 public:
  explicit ToggleSwitch(const QString& text, QWidget* parent = nullptr);

  [[nodiscard]] auto sizeHint() const -> QSize override;

 protected:
  void paintEvent(QPaintEvent* event) override;

  void enterEvent(QEnterEvent* event) override;

  void leaveEvent(QEvent* event) override;

 private:
  QVariantAnimation* slide_;
  qreal position_ = 0;  ///< 0 off, 1 on; animated between
  bool hovered_ = false;

  [[nodiscard]] auto track_rect() const -> QRectF;
};

}  // namespace GpgFrontend::UI
