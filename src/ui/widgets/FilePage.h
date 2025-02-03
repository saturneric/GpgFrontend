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

#include "ui/GpgFrontendUI.h"
#include "ui/widgets/FileTreeView.h"
#include "ui/widgets/InfoBoardWidget.h"

class Ui_FilePage;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class FilePage : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new File Page object
   *
   * @param parent
   */
  explicit FilePage(QWidget* parent, const QString&);

  /**
   * @brief Get the Selected object
   *
   * @return QString
   */
  [[nodiscard]] auto GetSelected() const -> QStringList;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsBatchMode() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsASCIIMode() const -> bool;

 public slots:
  /**
   * @brief
   *
   */
  void SlotGoPath();

 signals:

  /**
   * @brief
   *
   * @param path
   */
  void SignalPathChanged(const QString&);

  /**
   * @brief
   *
   * @param text
   * @param verify_label_status
   */
  void SignalRefreshInfoBoard(const QString&, InfoBoardStatus);

  /**
   * @brief
   *
   */
  void SignalCurrentTabChanged();

  /**
   * @brief
   *
   * @param int
   */
  void SignalMainWindowUpdateBasicOperaMenu(unsigned int);

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void keyPressEvent(QKeyEvent* event) override;

 private:
  std::shared_ptr<Ui_FilePage> ui_;  ///<

  QCompleter* path_edit_completer_;        ///<
  QStringListModel* path_complete_model_;  ///<

  QMenu* popup_menu_{};         ///<
  QMenu* option_popup_menu_{};  ///<
  FileTreeView* file_tree_view_;
  bool ascii_mode_;

 private slots:

  /**
   * @brief
   *
   */
  void update_main_basic_opera_menu(const QStringList&);
};

}  // namespace GpgFrontend::UI
