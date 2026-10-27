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
 * @brief Splits a fingerprint into its spaced groups of five, on two balanced
 * lines when it is longer than a v4 one (a v6 fingerprint has 13 groups).
 *
 * The break is fixed rather than left to word wrap, so the text never wraps
 * differently at another width and the label holding it keeps its height.
 */
auto GF_UI_EXPORT FingerprintDisplayText(const QString& fingerprint) -> QString;

/**
 * @brief A value label that copies its value to the clipboard on a click.
 *
 * The confirmation is a tooltip under the label: it floats above the window,
 * so it never takes layout space or covers a notice row placed elsewhere.
 */
class GF_UI_EXPORT CopyableLabel : public QLabel {
  Q_OBJECT

 public:
  explicit CopyableLabel(QWidget* parent = nullptr);

  /**
   * @brief Shows text and copies copy_text, or the text itself when empty.
   */
  void SetValue(const QString& text, const QString& copy_text = {});

 signals:
  void SignalCopied(const QString& value);

 protected:
  void mouseReleaseEvent(QMouseEvent* event) override;

 private:
  QString copy_text_;
};

}  // namespace GpgFrontend::UI
