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

#ifndef GPGFRONTEND_FILEPAGE_H
#define GPGFRONTEND_FILEPAGE_H

#include <boost/filesystem.hpp>

#include "ui/GpgFrontendUI.h"
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
  explicit FilePage(QWidget* parent = nullptr);

  /**
   * @brief Get the Selected object
   *
   * @return QString
   */
  [[nodiscard]] QString GetSelected() const;

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
  void SignalPathChanged(const QString& path);

  /**
   * @brief
   *
   * @param text
   * @param verify_label_status
   */
  void SignalRefreshInfoBoard(const QString& text,
                              InfoBoardStatus verify_label_status);

 private slots:

  /**
   * @brief
   *
   * @param index
   */
  void slot_file_tree_view_item_clicked(const QModelIndex& index);

  /**
   * @brief
   *
   * @param index
   */
  void slot_file_tree_view_item_double_clicked(const QModelIndex& index);

  /**
   * @brief
   *
   */
  void slot_up_level();

  /**
   * @brief
   *
   */
  void slot_open_item();

  /**
   * @brief
   *
   */
  void slot_rename_item();

  /**
   * @brief
   *
   */
  void slot_delete_item();

  /**
   * @brief
   *
   */
  void slot_encrypt_item();

  /**
   * @brief
   *
   */
  void slot_encrypt_sign_item();

  /**
   * @brief
   *
   */
  void slot_decrypt_item();

  /**
   * @brief
   *
   */
  void slot_sign_item();

  /**
   * @brief
   *
   */
  void slot_verify_item();

  /**
   * @brief
   *
   */
  void slot_calculate_hash();

  /**
   * @brief
   *
   */
  void slot_mkdir();

  /**
   * @brief
   *
   */
  void slot_create_empty_file();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void keyPressEvent(QKeyEvent* event) override;

  /**
   * @brief
   *
   * @param point
   */
  void onCustomContextMenu(const QPoint& point);

 private:
  /**
   * @brief Create a popup menu object
   *
   */
  void create_popup_menu();

  std::shared_ptr<Ui_FilePage> ui_;  ///<

  QFileSystemModel* dir_model_;            ///<
  QCompleter* path_edit_completer_;        ///<
  QStringListModel* path_complete_model_;  ///<

  boost::filesystem::path m_path_;         ///<
  boost::filesystem::path selected_path_;  ///<

  QMenu* popup_menu_{};         ///<
  QMenu* option_popup_menu_{};  ///<
  QWidget* first_parent_{};     ///<
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_FILEPAGE_H
