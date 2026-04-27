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
 * @brief File-system tree view used by the file operation page.
 *
 * FileTreeView is a QFileSystemModel-backed QTreeView for browsing local files
 * and folders. It provides common file-manager style operations such as opening
 * files, creating files or folders, renaming, moving items to Trash, copying
 * paths, refreshing, and drag-and-drop move/copy.
 *
 * The view keeps track of the current root directory and the currently selected
 * paths. It emits signals when the directory changes, when the selection
 * changes, or when a readable file should be opened by the main window.
 *
 * This widget performs local file-system UI operations only. OpenPGP-specific
 * file operations are triggered by higher-level UI components based on the
 * selected paths.
 */
class FileTreeView : public QTreeView {
  Q_OBJECT

 public:
  /**
   * @brief Constructs a file tree view.
   *
   * The view creates its own QFileSystemModel, initializes the model with the
   * user's home directory, installs the context menu, and configures selection,
   * sorting, drag-and-drop and visual behavior.
   *
   * @param parent Parent widget.
   */
  explicit FileTreeView(QWidget* parent);

  /**
   * @brief Initializes visual style and interaction behavior.
   *
   * This configures tree expansion behavior, row selection, edit triggers,
   * drag-and-drop support, scrolling, header behavior, column widths, and the
   * widget stylesheet.
   */
  void InitViewStyle();

  /**
   * @brief Sets the initial path shown by the view.
   *
   * If @p target_path is an existing file, its parent directory is opened and
   * the file is selected after the model has loaded. If it is an existing
   * directory, that directory is opened directly. If the path does not exist,
   * the current process directory is used as a fallback.
   *
   * @param target_path File or directory path to open.
   */
  void SetPath(const QString& target_path);

  /**
   * @brief Returns the current root directory shown by the view.
   *
   * @return Current absolute directory path.
   */
  auto GetCurrentPath() -> QString;

  /**
   * @brief Returns the currently selected file-system paths.
   *
   * The returned list is updated from the selection model whenever the
   * selection changes. In single-selection mode it contains at most one item;
   * in batch mode it may contain multiple files or directories.
   *
   * @return Selected absolute paths.
   */
  auto GetSelectedPaths() -> QStringList;

  /**
   * @brief Returns the file-system path at a viewport position.
   *
   * The point is interpreted in the view's viewport coordinate system. If the
   * point does not hit a valid model index, an empty string is returned.
   *
   * @param point Viewport-local position.
   * @return Absolute path under the point, or an empty string.
   */
  auto GetPathByClickPoint(const QPoint& point) -> QString;

  /**
   * @brief Converts a viewport-local position to a global screen position.
   *
   * This is mainly used when showing the custom context menu.
   *
   * @param point Viewport-local position.
   * @return Global screen position.
   */
  auto GetMousePointGlobal(const QPoint& point) -> QPoint;

 protected:
  /**
   * @brief Handles selection changes and updates the cached selected paths.
   *
   * The method collects selected rows from column 0, converts them to absolute
   * file-system paths, ignores invalid indexes and the current root directory,
   * then emits SignalSelectedChanged().
   *
   * @param selected Newly selected model ranges.
   * @param deselected Deselected model ranges.
   */
  void selectionChanged(const QItemSelection& selected,
                        const QItemSelection& deselected) override;

  /**
   * @brief Handles keyboard shortcuts for file navigation and item operations.
   *
   * Supported keys:
   *
   * - Backspace: go to the parent directory;
   * - F2: rename the selected item;
   * - Delete: move the selected item or items to Trash;
   * - Enter/Return: open the selected file or enter the selected directory.
   *
   * Other key events are delegated to QTreeView.
   *
   * @param event Key event.
   */
  void keyPressEvent(QKeyEvent* event) override;

  /**
   * @brief Handles mouse presses on empty space.
   *
   * A left click on empty space clears the current selection and current index.
   * Other behavior is delegated to QTreeView.
   *
   * @param event Mouse event.
   */
  void mousePressEvent(QMouseEvent* event) override;

  /**
   * @brief Accepts drag-enter events containing local URLs.
   *
   * Internal drags are treated as move operations, while drags from external
   * sources are treated as copy operations.
   *
   * @param event Drag-enter event.
   */
  void dragEnterEvent(QDragEnterEvent* event) override;

  /**
   * @brief Validates drag movement over writable drop targets.
   *
   * The event is accepted only if it contains URLs and the resolved target
   * directory is writable.
   *
   * @param event Drag-move event.
   */
  void dragMoveEvent(QDragMoveEvent* event) override;

  /**
   * @brief Handles file-system drag-and-drop copy or move operations.
   *
   * Internal drags move files or folders. External drags copy files or folders.
   * The method validates the target directory, prevents moving or copying a
   * directory into itself or one of its children, asks the user for
   * confirmation, performs the operation, refreshes the view, and selects the
   * resulting items.
   *
   * @param event Drop event.
   */
  void dropEvent(QDropEvent* event) override;

 signals:
  /**
   * @brief Emitted when the current root directory changes.
   *
   * @param path New current directory path.
   */
  void SignalPathChanged(const QString& path);

  /**
   * @brief Emitted when the selected file-system paths change.
   *
   * @param paths Current selected absolute paths.
   */
  void SignalSelectedChanged(const QStringList& paths);

  /**
   * @brief Emitted when a file should be opened by the application.
   *
   * This signal is emitted when the user double-clicks a readable file, presses
   * Enter on a readable file, or triggers the internal Open action.
   *
   * @param path File path to open.
   */
  void SignalOpenFile(const QString& path);

 public slots:
  /**
   * @brief Opens the given directory path as the current root directory.
   *
   * The path must exist, be a directory, be readable, and be executable. On
   * success, the model root is changed, the selection is cleared, columns are
   * adjusted, and SignalPathChanged() is emitted.
   *
   * Passing an empty path clears the current path and opens an empty model
   * root.
   *
   * @param target_path Directory path to open.
   */
  void SlotGoPath(const QString& target_path);

  /**
   * @brief Opens the parent directory of the current path.
   *
   * If the current path is empty, the request is ignored.
   */
  void SlotUpLevel();

  /**
   * @brief Shows or hides system entries in the file-system model.
   *
   * @param on true to include system entries, false to hide them.
   */
  void SlotShowSystemFile(bool on);

  /**
   * @brief Shows or hides hidden entries in the file-system model.
   *
   * @param on true to include hidden entries, false to hide them.
   */
  void SlotShowHiddenFile(bool on);

  /**
   * @brief Moves the selected items to Trash after user confirmation.
   *
   * On Qt 5.15 or newer, QFile::moveToTrash() is used. On older Qt versions,
   * files are removed and directories are removed recursively as a fallback.
   */
  void SlotDeleteSelectedItem();

  /**
   * @brief Creates a new folder in the current root directory.
   *
   * This clears the current selection, sets the root index as the current
   * target, and delegates to SlotMkdirBelowAtSelectedItem().
   */
  void SlotMkdir();

  /**
   * @brief Creates a new folder under the current target directory.
   *
   * The target directory is the selected directory, the parent of the selected
   * file, or the current root directory when nothing is selected.
   */
  void SlotMkdirBelowAtSelectedItem();

  /**
   * @brief Creates a new empty file in the current root directory.
   *
   * This clears the current selection, sets the root index as the current
   * target, and delegates to SlotTouchBelowAtSelectedItem().
   */
  void SlotTouch();

  /**
   * @brief Creates a new empty file under the current target directory.
   *
   * The target directory is the selected directory, the parent of the selected
   * file, or the current root directory when nothing is selected.
   */
  void SlotTouchBelowAtSelectedItem();

  /**
   * @brief Renames the single selected file or folder.
   *
   * The method asks the user for a new name, rejects empty names, unchanged
   * names, path separators and existing target names, then refreshes the view
   * and selects the renamed item.
   */
  void SlotRenameSelectedItem();

  /**
   * @brief Opens the single selected item with the system default application.
   *
   * This uses QDesktopServices::openUrl() and requires exactly one selected
   * existing path.
   */
  void SlotOpenSelectedItemBySystemApplication();

  /**
   * @brief Enables or disables batch-selection mode.
   *
   * Batch mode uses ExtendedSelection and allows Ctrl or Shift based multiple
   * selection. Disabling batch mode restores SingleSelection.
   *
   * @param batch true to enable batch mode, false to disable it.
   */
  void SlotSwitchBatchMode(bool batch);

  /**
   * @brief Copies selected absolute paths to the clipboard.
   *
   * Multiple paths are joined with line-feed characters.
   */
  void SlotCopyPath();

  /**
   * @brief Refreshes the current file list.
   *
   * If the current path is no longer accessible, the view falls back to the
   * user's home directory.
   */
  void SlotRefresh();

 private slots:
  /**
   * @brief Handles activation of an item by double click or Enter.
   *
   * Readable files are opened through SignalOpenFile(). Readable and executable
   * directories are entered through SlotGoPath(). Inaccessible items show a
   * warning dialog.
   *
   * @param index Activated model index.
   */
  void slot_file_tree_view_item_double_clicked(const QModelIndex& index);

  /**
   * @brief Calculates the hash of the selected file asynchronously.
   *
   * The result is displayed on the global information board.
   */
  void slot_calculate_hash();

  /**
   * @brief Shows the custom context menu for the clicked position.
   *
   * The method updates the selection according to the click target, enables or
   * disables actions based on item type and permissions, then shows the popup
   * menu.
   *
   * @param point Viewport-local position where the context menu was requested.
   */
  void slot_show_custom_context_menu(const QPoint& point);

  /**
   * @brief Creates the context menu and all file-operation actions.
   */
  void slot_create_popup_menu();

  /**
   * @brief Adjusts non-name columns to their contents.
   *
   * The first column is kept at a usable minimum width.
   */
  void slot_adjust_column_widths();

 private:
  QFileSystemModel* dir_model_;  ///< File-system model backing the tree view.
  QString current_path_;         ///< Current root directory path.
  QStringList selected_paths_;   ///< Cached selected absolute paths.

  QMenu* popup_menu_;            ///< Main context menu.
  QMenu* new_item_action_menu_;  ///< Submenu for creating new items.

  QAction* action_open_file_;    ///< Opens selected files inside GpgFrontend.
  QAction* action_rename_file_;  ///< Renames the selected item.
  QAction* action_delete_file_;  ///< Moves selected items to Trash.
  QAction* action_calculate_hash_;  ///< Calculates hash of the selected file.
  QAction* action_create_empty_file_;  ///< Creates a new empty file.
  QAction* action_make_directory_;     ///< Creates a new folder.
  QAction* action_compress_files_;     ///< Reserved compression action.
  QAction*
      action_open_with_system_default_application_;  ///< Opens item with system
                                                     ///< default app.
  QAction* action_copy_path_ =
      nullptr;                         ///< Copies selected paths to clipboard.
  QAction* action_refresh_ = nullptr;  ///< Refreshes the file list.

  /**
   * @brief Resolves the directory that should receive a drop operation.
   *
   * Dropping on a directory targets that directory. Dropping on a file targets
   * the file's parent directory. Dropping on empty space targets the current
   * root directory.
   *
   * @param pos Viewport-local drop position.
   * @return Target directory path, or an empty string if none can be resolved.
   */
  [[nodiscard]] auto get_drop_target_directory(const QPoint& pos) const
      -> QString;

  /**
   * @brief Moves a file or folder into a target directory.
   *
   * The method validates source and target paths, checks writability, prevents
   * moving a directory into itself or a child directory, rejects name
   * conflicts, and performs the move with QDir::rename().
   *
   * @param source_path Source file or folder path.
   * @param target_dir Target directory path.
   * @return true if the move succeeds or the source is already at the target
   *         path, otherwise false.
   */
  auto move_path_to_directory(const QString& source_path,
                              const QString& target_dir) -> bool;

  /**
   * @brief Checks whether a source directory would be moved or copied into
   * itself.
   *
   * For non-directory sources this only returns true when source and target are
   * the same path. For directory sources it also returns true when the target
   * is one of the source directory's child paths.
   *
   * @param source_path Source path.
   * @param target_dir Target directory path.
   * @return true if the operation would target the source itself or its child.
   */
  [[nodiscard]] auto is_move_into_itself_or_child(
      const QString& source_path, const QString& target_dir) const -> bool;

  /**
   * @brief Checks whether all source paths already belong to the target
   * directory.
   *
   * This is used to ignore no-op drag-and-drop operations within the same
   * folder.
   *
   * @param source_paths Source paths.
   * @param target_dir Target directory path.
   * @return true if every source path is already in @p target_dir.
   */
  [[nodiscard]] auto is_same_directory_operation(
      const QStringList& source_paths, const QString& target_dir) const -> bool;

  /**
   * @brief Copies a file or folder into a target directory.
   *
   * Files are copied with QFile::copy(). Directories are copied recursively.
   * Existing target names are rejected.
   *
   * @param source_path Source file or folder path.
   * @param target_dir Target directory path.
   * @return true if the copy succeeds, otherwise false.
   */
  auto copy_path_to_directory(const QString& source_path,
                              const QString& target_dir) -> bool;

  /**
   * @brief Recursively copies one directory to another location.
   *
   * Hidden and system entries are included. Unsupported file-system entries
   * such as symbolic links, sockets or special files are skipped.
   *
   * @param source_dir Source directory path.
   * @param target_dir Target directory path to create and fill.
   * @return true if the whole directory is copied successfully, otherwise
   * false.
   */
  auto copy_directory_recursive(const QString& source_dir,
                                const QString& target_dir) -> bool;

  /**
   * @brief Returns the path of the current target directory.
   *
   * If the current index is a directory, that directory is returned. If it is a
   * file, its parent directory is returned. If no current index is available,
   * the current root directory is returned.
   *
   * @return Target directory path.
   */
  [[nodiscard]] auto current_target_directory_path() const -> QString;
};

}  // namespace GpgFrontend::UI