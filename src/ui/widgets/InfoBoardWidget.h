/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "PlainTextEditorPage.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "ui/dialog/details/VerifyDetailsDialog.h"

class Ui_InfoBoard;

namespace GpgFrontend::UI {

/**
 * @details Enumeration for the status of Verify label
 */
typedef enum {
  INFO_ERROR_OK = 0,
  INFO_ERROR_WARN = 1,
  INFO_ERROR_CRITICAL = 2,
  INFO_ERROR_NEUTRAL = 3,
} InfoBoardStatus;

/**
 * @brief Class for handling the verify label shown at bottom of a textedit-page
 */
class InfoBoardWidget : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief
   *
   * @param ctx The GPGme-Context
   * @param parent The parent widget
   */
  explicit InfoBoardWidget(QWidget* parent);

  /**
   * @brief
   *
   * @param edit
   */
  void AssociateTextEdit(QTextEdit* edit);

  /**
   * @brief
   *
   * @param tab
   */
  void AssociateTabWidget(QTabWidget* tab);

  /**
   * @brief
   *
   * @param name
   * @param action
   */
  void AddOptionalAction(const QString& name,
                         const std::function<void()>& action);

  /**
   * @brief
   *
   */
  void ResetOptionActionsMenu();

  /**
   * @details Set the text and background-color of verify notification.
   *
   * @param text The text to be set.
   * @param verify_label_status The status of label to set the specified color.
   */
  void SetInfoBoard(const QString& text,
                    GpgFrontend::UI::InfoBoardStatus verify_label_status);

 public slots:

  /**
   * @brief
   *
   */
  void SlotReset();

  /**
   * @details Refresh the contents of dialog.
   */
  void SlotRefresh(const QString& text,
                   GpgFrontend::UI::InfoBoardStatus status);

 private slots:

  /**
   * @brief
   *
   */
  void slot_copy();

  /**
   * @brief
   *
   */
  void slot_save();

 private:
  std::shared_ptr<Ui_InfoBoard> ui_;  ///<

  QTextEdit* m_text_page_{
      nullptr};  ///< TextEdit associated to the notification
  QTabWidget* m_tab_widget_{nullptr};  ///<

  /**
   * @brief
   *
   * @param layout
   * @param start_index
   */
  void delete_widgets_in_layout(QLayout* layout, int start_index = 0);
};

}  // namespace GpgFrontend::UI
