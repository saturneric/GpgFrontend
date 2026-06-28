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

#include "core/model/GFBuffer.h"
#include "ui/widgets/FilePage.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI {

class TextEditTabWidget;

/**
 * @brief Main tabbed workspace widget for text editing and file browsing.
 *
 * TextEdit owns the central TextEditTabWidget used by the main window. It
 * provides high-level operations for creating, opening, saving, closing and
 * switching tabs.
 *
 * A tab can be a plain-text editor page, a file browser page, or a custom tab
 * provided by modules. TextEdit dispatches common UI actions to the currently
 * active tab where possible, for example cut/copy/paste, undo/redo, printing,
 * saving and text insertion.
 *
 * TextEdit also handles unsaved-document checks before closing tabs or exiting
 * the application.
 */
class TextEdit : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Constructs the main text-edit workspace.
   *
   * The constructor creates the internal tab widget, installs it into the
   * layout, connects tab-close handling, disables drag-and-drop on this
   * wrapper, and opens the default workspace tab.
   *
   * @param parent Parent widget.
   */
  explicit TextEdit(QWidget* parent);

  /**
   * @brief Loads a file into the current plain-text editor page.
   *
   * The file is read into a GFBuffer and its text representation is inserted
   * into the current text page. The current page file path and tab title are
   * updated afterwards.
   *
   * @param fileName File path to load.
   */
  void LoadFile(const QString& fileName);

  /**
   * @brief Checks all tabs for unsaved documents before closing.
   *
   * If text-editor page restoration is enabled, unsaved text editors are cached
   * and the method returns true. Otherwise, the user is asked whether to save,
   * discard or cancel depending on the number of modified tabs.
   *
   * @return true if closing can continue, false if it should be aborted.
   */
  auto MaybeSaveAnyTab() -> bool;

  /**
   * @brief Returns the number of currently opened tabs.
   *
   * @return Tab count.
   */
  [[nodiscard]] auto TabCount() const -> int;

  /**
   * @brief Returns the current plain-text editor page.
   *
   * @return Current PlainTextEditorPage, or nullptr if the current tab is not a
   *         text editor page.
   */
  [[nodiscard]] auto CurTextPage() const -> PlainTextEditorPage*;

  /**
   * @brief Returns the current file browser page.
   *
   * @return Current FilePage, or nullptr if the current tab is not a file page.
   */
  [[nodiscard]] auto CurFilePage() const -> FilePage*;

  /**
   * @brief Returns all modified text-editor tabs.
   *
   * Only finished-loading PlainTextEditorPage tabs whose documents are modified
   * are included.
   *
   * @return Hash mapping tab indexes to display names of unsaved documents.
   */
  [[nodiscard]] auto UnsavedDocuments() const -> QHash<int, QString>;

  /**
   * @brief Returns the plain text of the current text editor page.
   *
   * @return Current plain text, or an empty string if the current tab is not a
   *         text editor page.
   */
  [[nodiscard]] auto CurPlainText() const -> QString;

  /**
   * @brief Returns the underlying tab widget.
   *
   * @return Pointer to the internal QTabWidget.
   */
  [[nodiscard]] auto TabWidget() const -> QTabWidget*;

  /**
   * @brief Returns the current plain-text editor page.
   *
   * This is an alias for the current text editor page accessor used by parts of
   * the UI code.
   *
   * @return Current PlainTextEditorPage, or nullptr.
   */
  [[nodiscard]] auto CurPageTextEdit() const -> PlainTextEditorPage*;

  /**
   * @brief Returns the current file browser page.
   *
   * This is an alias for the current file page accessor used by parts of the UI
   * code.
   *
   * @return Current FilePage, or nullptr.
   */
  [[nodiscard]] auto CurPageFileTreeView() const -> FilePage*;

  /**
   * @brief Returns the currently active tab page.
   *
   * @return Current tab widget page, or nullptr if no page is available.
   */
  [[nodiscard]] auto CurPage() -> QWidget*;

 public slots:
  /**
   * @brief Quotes every line in the current text editor.
   *
   * A "> " prefix is inserted at the beginning of each line. The whole
   * operation is grouped as one undo/redo edit block.
   */
  void SlotQuote() const;

  /**
   * @brief Replaces the current text editor content with text.
   *
   * Undo/redo is temporarily disabled while the text is replaced. The document
   * is marked as modified afterwards.
   *
   * @param text Text to insert.
   */
  void SlotFillTextEditWithText(const QString& text) const;

  /**
   * @brief Replaces the current text editor content with a GFBuffer.
   *
   * The buffer is converted to QString before being inserted.
   *
   * @param buffer Buffer whose text representation should be inserted.
   */
  void SlotFillTextEditWithText(const GFBuffer& buffer) const;

  /**
   * @brief Saves the current tab.
   *
   * For text-editor tabs, this saves to the existing file path or opens Save As
   * when no file path is assigned. File browser tabs are ignored. Custom tab
   * types may handle saving through the module event system.
   */
  void SlotSave();

  /**
   * @brief Opens a Save File dialog and saves the current text-editor page.
   *
   * @return true if saving succeeds or no text page is active, otherwise false.
   */
  bool SlotSaveAs();  // NOLINT

  /**
   * @brief Opens one or more files in new text-editor tabs.
   *
   * The method shows a file-open dialog and delegates each selected file to
   * SlotOpenFile().
   */
  void SlotOpen();

  /**
   * @brief Prints the current text editor document.
   *
   * A print dialog is shown before printing. If the current tab has no text
   * document, a warning is shown.
   */
  void SlotPrint();

  /**
   * @brief Creates a new plain-text editor tab.
   *
   * @return Newly created tab page.
   */
  QWidget* SlotNewTab();  // NOLINT

  /**
   * @brief Creates a new custom tab.
   *
   * Custom tabs are identified by a type string and may be handled by modules
   * or other higher-level UI logic.
   *
   * @param type Custom tab type.
   * @param title Tab title.
   * @param icon Tab icon.
   * @param icon_name Icon name.
   * @return Newly created tab page.
   */
  // NOLINTNEXTLINE
  QWidget* SlotNewCustomTab(const QString& type, const QString& title,
                            const QIcon& icon, const QString& icon_name);

  /**
   * @brief Creates a new text-editor tab with predefined content.
   *
   * @param title Tab title.
   * @param content Initial text content.
   */
  void SlotNewTabWithContent(QString title, const QString& content);

  /**
   * @brief Opens a file path in a text-editor tab after validation.
   *
   * Delegates to TextEditTabWidget::SlotOpenFile, which performs all validation
   * (readable regular file, file-size limit, binary detection) and reports any
   * problem to the user.
   *
   * @param path File path to open.
   */
  void SlotOpenFile(const QString& path);

  /**
   * @brief Opens the default workspace tab.
   *
   * The default workspace is selected from settings. In sandbox mode, a plain
   * text editor is used instead of the file panel to avoid sandbox-related file
   * browser issues. Cached text editor tabs are restored afterwards.
   */
  void SlotNewDefaultWorkspaceTab();

  /**
   * @brief Opens the default file browser tab.
   */
  void SlotOpenDefaultFileBrowserTab();

  /**
   * @brief Opens a file browser tab from a selected file path.
   *
   * The user selects a file through a file dialog. The containing path is then
   * opened by the tab widget.
   */
  void SlotNewFileBrowserTab();

  /**
   * @brief Opens a file browser tab from a selected directory.
   *
   * The user selects a directory through a directory dialog, and that directory
   * is opened by the tab widget.
   */
  void SlotNewFileBrowserTabWithDirectory();

  /**
   * @brief Closes the current tab.
   *
   * Unsaved text-editor content is checked before the tab is removed.
   */
  void SlotCloseTab();

  /**
   * @brief Switches to the next tab.
   */
  void SlotSwitchTabUp() const;

  /**
   * @brief Switches to the previous tab.
   */
  void SlotSwitchTabDown() const;

  /**
   * @brief Cuts selected text from the current text editor.
   */
  void SlotCut() const;

  /**
   * @brief Copies selected text from the current text editor.
   */
  void SlotCopy() const;

  /**
   * @brief Pastes clipboard text into the current text editor.
   */
  void SlotPaste() const;

  /**
   * @brief Undoes the last change in the current text editor.
   */
  void SlotUndo() const;

  /**
   * @brief Redoes the last undone change in the current text editor.
   */
  void SlotRedo() const;

  /**
   * @brief Zooms in the current text editor.
   */
  void SlotZoomIn() const;

  /**
   * @brief Zooms out the current text editor.
   */
  void SlotZoomOut() const;

  /**
   * @brief Selects all text in the current text editor.
   */
  void SlotSelectAll() const;

  /**
   * @brief Appends text to the current text editor without revealing it.
   *
   * If no text editor is active, a new text tab is created first.
   *
   * @param text Text to append.
   */
  void SlotAppendText2CurTextPage(const QString& text);

  /**
   * @brief Appends text to the current text editor.
   *
   * The method preserves the old cursor and scroll position unless @p reveal is
   * true. A newline is inserted before or after the appended text when needed.
   * The document is marked as modified.
   *
   * @param text Text to append.
   * @param reveal true to scroll to and reveal the appended text.
   */
  void SlotAppendText2CurTextPage(const QString& text, bool reveal);

  /**
   * @brief Appends text to the current text editor and reveals it.
   *
   * @param text Text to append.
   */
  void SlotAppendText2CurTextPageAndReveal(const QString& text);

  /**
   * @brief Replaces the current text editor content with a GFBuffer.
   *
   * If no text editor is active, a new text tab is created first.
   *
   * @param buffer Buffer to convert and insert.
   */
  void SlotSetGFBuffer2CurTextPage(const GFBuffer& buffer);

  /**
   * @brief Returns the underlying tab widget.
   *
   * Slot-style accessor kept for signal/slot based callers.
   *
   * @return Pointer to the internal QTabWidget.
   */
  QTabWidget* SlotGetTabWidget();  // NOLINT

 protected:
  /**
   * @brief Saves the current plain-text editor page to a file.
   *
   * On success, the tab title, page file path, document modified state and page
   * save notification state are updated.
   *
   * @param file_name Target file path.
   * @return true if the file is saved successfully, otherwise false.
   */
  auto saveFile(const QString& file_name) -> bool;

 private slots:
  /**
   * @brief Removes a tab after checking for unsaved changes.
   *
   * The target tab is made current before the save check. If closing is
   * allowed, the tab is removed and scheduled for deletion.
   *
   * @param index Tab index to remove.
   */
  void slot_remove_tab(int index);

 private:
  TextEditTabWidget* tab_widget_;  ///< Tab widget containing workspace pages.

  /**
   * @brief Extracts the base file name from a full path.
   *
   * @param full_file_name Full file path.
   * @return File name without parent directory path.
   */
  static auto stripped_name(const QString& full_file_name) -> QString;

  /**
   * @brief Checks whether the current text-editor tab should be saved.
   *
   * If the current page is not a text editor or is not modified, the method
   * returns true. If the document has unsaved changes, the user may be asked to
   * save, discard or cancel depending on @p askToSave.
   *
   * @param askToSave true to show a save/discard/cancel dialog, false to save
   *                  directly without asking.
   * @return true if closing or continuing is allowed, otherwise false.
   */
  auto maybe_save_current_tab(bool askToSave) -> bool;
};

}  // namespace GpgFrontend::UI