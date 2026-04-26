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

class FileTreeView : public QTreeView {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new File Tree View object
   *
   * @param parent
   * @param target_path
   */
  explicit FileTreeView(QWidget* parent);

  void InitViewStyle();

  /**
   */
  void SetPath(const QString& target_path);

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

  /**
   * @brief
   *
   * @return QModelIndex
   */
  [[nodiscard]] auto CurrentTargetDirectoryIndex() const -> QModelIndex;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto CurrentTargetDirectoryPath() const -> QString;

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

  /**
   * @brief
   *
   * @param event
   */
  void dragEnterEvent(QDragEnterEvent* event) override;

  /**
   * @brief
   *
   * @param event
   */
  void dragMoveEvent(QDragMoveEvent* event) override;

  /**
   * @brief
   *
   * @param event
   */
  void dropEvent(QDropEvent* event) override;

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

  /**
   * @brief
   *
   */
  void SlotCopyPath();

  /**
   * @brief
   *
   */
  void SlotRefresh();

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
  QAction* action_open_with_system_default_application_;
  QAction* action_copy_path_ = nullptr;
  QAction* action_refresh_ = nullptr;

  bool initial_resize_done_ = false;

  /**
   * @brief Get the drop target directory object
   *
   * @param pos
   * @return QString
   */
  [[nodiscard]] auto get_drop_target_directory(const QPoint& pos) const
      -> QString;

  /**
   * @brief
   *
   * @param source_path
   * @param target_dir
   * @return true
   * @return false
   */
  auto move_path_to_directory(const QString& source_path,
                              const QString& target_dir) -> bool;

  /**
   * @brief
   *
   * @param source_path
   * @param target_dir
   * @return true
   * @return false
   */
  [[nodiscard]] auto is_move_into_itself_or_child(
      const QString& source_path, const QString& target_dir) const -> bool;

  /**
   * @brief
   *
   * @param source_paths
   * @param target_dir
   * @return true
   * @return false
   */
  [[nodiscard]] auto is_same_directory_operation(
      const QStringList& source_paths, const QString& target_dir) const -> bool;

  /**
   * @brief
   *
   * @param source_path
   * @param target_dir
   * @return true
   * @return false
   */
  auto copy_path_to_directory(const QString& source_path,
                              const QString& target_dir) -> bool;

  /**
   * @brief
   *
   * @param source_dir
   * @param target_dir
   * @return true
   * @return false
   */
  auto copy_directory_recursive(const QString& source_dir,
                                const QString& target_dir) -> bool;
};
}  // namespace GpgFrontend::UI