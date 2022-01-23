/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef FINDWIDGET_H
#define FINDWIDGET_H

#include "ui/GpgFrontendUI.h"
#include "ui/widgets/PlainTextEditorPage.h"

namespace GpgFrontend::UI {

/**
 * @brief Class for handling the find widget shown at buttom of a textedit-page
 */
class FindWidget : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief
   *
   * @param parent The parent widget
   */
  explicit FindWidget(QWidget* parent, PlainTextEditorPage* edit);

 protected:
  /**
   * @brief
   *
   * @param e
   */
  void keyPressEvent(QKeyEvent* e) override;

 private:
  /**
   * @details Set background of findEdit to red, if no match is found (Documents
   * textcursor position equals -1), otherwise set it to white.
   */
  void set_background();

  PlainTextEditorPage*
      m_text_page_;       ///< Textedit associated to the notification
  QLineEdit* find_edit_;  ///<  Label holding the text shown in infoBoard

 private slots:

  /**
   * @brief
   *
   */
  void slot_find_next();

  /**
   * @brief
   *
   */
  void slot_find_previous();

  /**
   * @brief
   *
   */
  void slot_find();

  /**
   * @brief
   *
   */
  void slot_close();
};

}  // namespace GpgFrontend::UI

#endif  // FINDWIDGET_H
