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
 * @brief One answer to a choice, as a tile the whole of which is clickable.
 *
 * A radio button with a grey paragraph under it asks the reader to aim at a
 * 14px circle and leaves them unsure which paragraph belongs to which answer.
 * A tile makes the answer and its explanation one object, and the selected one
 * says so with its border rather than with a dot.
 *
 * Tiles that answer one question share a QButtonGroup: two radios in two tiles
 * are two exclusive groups unless told otherwise.
 *
 * Not a Q_OBJECT: it declares no signals of its own; listen to Radio().
 */
class GF_UI_EXPORT ChoiceTile : public QFrame {
 public:
  /**
   * @param title the answer, in bold
   * @param detail what choosing it means; may be empty
   * @param chip a short tag beside the title ("Recommended"); may be empty
   * @param parent parent widget
   */
  ChoiceTile(const QString& title, const QString& detail, const QString& chip,
             QWidget* parent);

  [[nodiscard]] auto Radio() const -> QRadioButton*;

  /// @brief Change the badge beside the title; empty removes it.
  void SetChip(const QString& chip);

  /// @brief A quiet last line under the detail, such as what the answer is
  /// made of ("Ed25519 · X25519"); empty removes it.
  void SetSecondary(const QString& secondary);

 protected:
  void mousePressEvent(QMouseEvent* event) override;

  void changeEvent(QEvent* event) override;

 private:
  QRadioButton* radio_;
  QLabel* chip_label_;
  QLabel* secondary_label_;
  QString sheet_;  ///< the sheet last set, so an unchanged one is not set again
  bool restyling_ = false;

  void restyle();
};

}  // namespace GpgFrontend::UI
