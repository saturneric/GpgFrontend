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

#include "ui/widgets/MetaListPanel.h"

class QDialogButtonBox;

namespace GpgFrontend::UI {

/**
 * @brief A question or a notice about a file, described in rows.
 *
 * What a QMessageBox is for, minus the thing that made every one of these
 * unreadable: informative text. A message box has one paragraph to say what a
 * file is, where it is, how big it is and what it claims about itself, and four
 * facts crammed into a paragraph read as none. Here they are a list, in the
 * same shape and the same order as everywhere else the same file appears.
 *
 * The buttons are the caller's, in the caller's words, because "Yes" answers a
 * question nobody re-reads by the time they reach it.
 */
class GF_UI_EXPORT MetaListDialog : public QDialog {
  Q_OBJECT

 public:
  /**
   * @brief Open a dialog with the shared badge, heading and rule.
   *
   * @param title the heading, and the window title
   * @param subtitle one wrapped line saying what this is about
   * @param parent parent widget
   */
  MetaListDialog(const QString& title, const QString& subtitle,
                 QWidget* parent = nullptr);

  /**
   * @brief Add a list, optionally inside a titled card.
   *
   * @param card_title the card's heading; empty puts the rows in the column
   * @param rows the rows
   */
  void AddSection(const QString& card_title, const QVector<MetaListRow>& rows);

  /**
   * @brief Add a sentence that is not about one row in particular.
   *
   * @param text the sentence
   * @param danger paint it as a cost rather than as an aside
   */
  void AddNote(const QString& text, bool danger = false);

  /**
   * @brief Add a button, named after what it does.
   *
   * @param text the button's text
   * @param role where the platform style seats it, and what it means
   * @return the index this button answers with
   */
  auto AddButton(const QString& text, QDialogButtonBox::ButtonRole role) -> int;

  /**
   * @brief Which button was pressed.
   *
   * @return its index, or -1 when the dialog was dismissed instead
   */
  [[nodiscard]] auto Choice() const -> int;

 private:
  QVBoxLayout* layout_{};
  QDialogButtonBox* buttons_{};
  int choice_ = -1;
};

}  // namespace GpgFrontend::UI
