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

#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend::UI {

class FileTreeView : public QTreeView {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new File Tree View object
   *
   * @param parent
   * @param target_path
   */
  explicit FileTreeView(QWidget* parent, const QString& target_path);

  /**
   * @brief Get the Current Path object
   *
   * @return QString
   */
  auto GetCurrentPath() -> QString;

  /**
   * @brief Get the Selected Path object
   *
   * @return QString
   */
  auto GetSelectedPaths() -> QStringList;

  /**
   * @brief Get the Path By Click Point object
   *
   * @param point
   * @return QString
   */
  auto GetPathByClickPoint(const QPoint& point) -> QString;

  /**
   * @brief Get the Mouse Point Global object
   *
   * @param point
   * @return QPoint
   */
  auto GetMousePointGlobal(const QPoint& point) -> QPoint;

 protected:
  /**
   * @brief
   *
   * @param selected
   * @param deselected
   */
  void selectionChanged(const QItemSelection& selected,
                        const QItemSelection& deselected) override;

  /**
   * @brief
   *
   * @param event
   */
  void keyPressEvent(QKeyEvent* event) override;

  /**
   * @brief
   *
   */
  void paintEvent(QPaintEvent* event) override;

  /**
   * @brief
   *
   * @param event
   */
  void mousePressEvent(QMouseEvent* event) override;

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
   */
  void SignalSelectedChanged(const QStringList&);

  /**
   * @brief
   *
   */
  void SignalOpenFile(const QString&);

 public slots:

  /**
   * @brief
   *
   */
  void SlotGoPath(const QString&);

  /**
   * @brief
   *
   */
  void SlotUpLevel();

  /**
   * @brief
   *
   */
  void SlotShowSystemFile(bool);

  /**
   * @brief
   *
   * @param on
   */
  void SlotShowHiddenFile(bool);

  /**
   * @brief
   *
   */
  void SlotDeleteSelectedItem();

  /**
   * @brief
   *
   */
  void SlotMkdir();

  /**
   * @brief
   *
   */
  void SlotMkdirBelowAtSelectedItem();

  /**
   * @brief
   *
   */
  void SlotTouch();

  /**
   * @brief
   *
   */
  void SlotTouchBelowAtSelectedItem();

  /**
   * @brief
   *
   */
  void SlotRenameSelectedItem();

  /**
   * @brief
   *
   */
  void SlotOpenSelectedItemBySystemApplication();

  /**
   * @brief
   *
   */
  void SlotSwitchBatchMode(bool);

 private slots:

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
  void slot_calculate_hash();

  /**
   * @brief compress directory into gpg-zip
   *
   */
  void slot_compress_files();

  /**
   * @brief
   *
   * @param point
   */
  void slot_show_custom_context_menu(const QPoint& point);

  /**
   * @brief Create a popup menu object
   *
   */
  void slot_create_popup_menu();

  /**
   * @brief
   *
   */
  void slot_adjust_column_widths();

 private:
  QFileSystemModel* dir_model_;  ///<
  QString current_path_;         ///<
  QStringList selected_paths_;   ///<

  QMenu* popup_menu_;
  QMenu* new_item_action_menu_;
  QAction* action_open_file_;
  QAction* action_rename_file_;
  QAction* action_delete_file_;
  QAction* action_calculate_hash_;
  QAction* action_create_empty_file_;
  QAction* action_make_directory_;
  QAction* action_compress_files_;

  bool initial_resize_done_ = false;
};
}  // namespace GpgFrontend::UI