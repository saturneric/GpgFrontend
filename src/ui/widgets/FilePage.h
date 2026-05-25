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

#include "ui/widgets/InfoBoardWidget.h"

class Ui_FilePage;

namespace GpgFrontend::UI {

/**
 * @brief File browser page used by the main window.
 *
 * FilePage provides a file-system based operation page for GpgFrontend. It
 * wraps a FileTreeView together with a path editor, mounted-volume selector,
 * view options and batch-selection controls.
 *
 * The widget is responsible for:
 *
 * - displaying and changing the current directory;
 * - supporting user-entered paths, including "~" and relative paths;
 * - offering path completion for readable directories;
 * - tracking selected files and directories;
 * - updating the main window operation menu according to the current selection;
 * - exposing file-operation options such as batch mode and ASCII armor mode;
 * - maintaining the mounted-volume popup menu.
 *
 * The actual file operation execution is not performed by this widget. Instead,
 * FilePage emits signals and updates the main-window operation state according
 * to the selected paths.
 */
class FilePage : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Constructs a file browser page.
   *
   * @param parent Parent widget.
   * @param target_path Initial directory path shown by the file tree view.
   */
  explicit FilePage(QWidget* parent, const QString& target_path);

  /**
   * @brief Returns the currently selected file-system paths.
   *
   * In normal mode this usually contains at most one path. In batch mode it may
   * contain multiple selected files or directories.
   *
   * @return List of selected absolute paths.
   */
  [[nodiscard]] auto GetSelected() const -> QStringList;

  /**
   * @brief Returns whether batch-selection mode is enabled.
   *
   * Batch mode allows the file tree view to select multiple items for one
   * operation.
   *
   * @return true if batch mode is enabled, otherwise false.
   */
  [[nodiscard]] auto IsBatchMode() const -> bool;

  /**
   * @brief Returns whether ASCII armored output is enabled for file operations.
   *
   * When enabled, file encryption/signing operations should prefer ASCII
   * armored output where supported.
   *
   * @return true if ASCII armor mode is enabled, otherwise false.
   */
  [[nodiscard]] auto IsASCIIMode() const -> bool;

  /**
   * @brief Returns the current directory shown by the file tree view.
   *
   * @return Current absolute directory path.
   */
  [[nodiscard]] auto GetCurrentPath() const -> QString;

 public slots:
  /**
   * @brief Opens the path currently entered in the path editor.
   *
   * The input path is normalized before use. The method supports home-directory
   * shortcuts such as "~" and "~/...", and resolves relative paths against the
   * current directory.
   *
   * If the target path does not exist, is not a directory, or cannot be read,
   * the path editor shows an error tooltip and the current directory is kept
   * unchanged.
   */
  void SlotGoPath();

  /**
   * @brief Refreshes the file page state.
   *
   * This updates the mounted-volume menu and reloads the current path.
   */
  void SlotRefreshState();

 signals:
  /**
   * @brief Emitted when the current directory path changes.
   *
   * @param path New current directory path.
   */
  void SignalPathChanged(const QString& path);

  /**
   * @brief Requests the global information board to be refreshed.
   *
   * @param text Text shown on the information board.
   * @param status Display status used by the information board.
   */
  void SignalRefreshInfoBoard(const QString& text, InfoBoardStatus status);

  /**
   * @brief Emitted when the current tab becomes active.
   *
   * This signal is used to refresh the main-window operation menu based on the
   * current file selection.
   */
  void SignalCurrentTabChanged();

  /**
   * @brief Requests the main window to update its basic operation menu.
   *
   * The value is a bit mask of MainWindow::OperationMenu::OperationType values.
   * It indicates which operations are currently available for the selected
   * paths, for example encryption, decryption, signing or verification.
   *
   * @param operation_type Available operation bit mask.
   */
  void SignalMainWindowUpdateBasicOperaMenu(unsigned int operation_type);

 private:
  QSharedPointer<Ui_FilePage> ui_;  ///< Generated UI object.

  QCompleter* path_edit_completer_ = nullptr;  ///< Completer for path input.
  QStringListModel* path_complete_model_ =
      nullptr;  ///< Model used by the path completer.

  QMenu* popup_menu_{};           ///< Reserved popup menu pointer.
  QMenu* option_popup_menu_{};    ///< File-view option popup menu.
  QMenu* harddisk_popup_menu_{};  ///< Mounted-volume popup menu.

  bool ascii_mode_;  ///< Whether ASCII armored output is enabled.

  QSet<QString> last_volume_keys_;  ///< Cached mounted-volume keys.

  /**
   * @brief Updates the available main-window file operations.
   *
   * The selected paths are inspected and converted into an operation bit mask.
   * For example:
   *
   * - regular non-OpenPGP files can be encrypted or signed;
   * - OpenPGP message files can be decrypted or verified;
   * - detached signature files can be verified;
   * - directories can be encrypted where supported.
   *
   * @param selected_paths Currently selected file-system paths.
   */
  void update_main_basic_opera_menu(const QStringList& selected_paths);

  /**
   * @brief Rebuilds the mounted-volume popup menu.
   *
   * The menu is updated only when the mounted-volume set has changed. Each menu
   * item stores the corresponding root path and navigates the file page to that
   * path when triggered.
   */
  void update_harddisk_menu();

  /**
   * @brief Periodically refreshes the mounted-volume menu.
   *
   * This method schedules itself again after a short delay, allowing removable
   * drives and other mounted volumes to appear or disappear in the UI.
   */
  void update_harddisk_menu_periodic();

  /**
   * @brief Initializes visual style and basic widget behavior.
   *
   * This configures tool buttons, path editor behavior, popup modes and tree
   * view display options.
   */
  void init_ui_style();

  /**
   * @brief Updates path-completion candidates for the path editor.
   *
   * The input is normalized against the current directory. Completion
   * candidates are generated from readable subdirectories of the resolved base
   * directory. Displayed candidates preserve the user's input style where
   * possible, such as "~" paths or relative paths.
   *
   * @param input Current text entered by the user.
   */
  void update_path_completion(const QString& input);
};

}  // namespace GpgFrontend::UI
