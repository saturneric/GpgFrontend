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

#include "ui/widgets/EMailEditorPage.h"
#include "ui/widgets/FilePage.h"
#include "ui/widgets/PlainTextEditorPage.h"

namespace GpgFrontend::UI {

class TextEditTabWidget;

class TextEdit : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief
   */
  explicit TextEdit(QWidget* parent);

  /**
   * @details Load the content of file into the current textpage
   *
   * @param fileName QString containing the filename to load
   * @return nothing
   */
  void LoadFile(const QString& fileName);

  /**
   * @details Checks if there are unsaved documents in any tab,
   *  which may need to be saved. Call this function before
   *  closing the programme or all tabs.
   * @return \li false, if the close event should be aborted.
   *         \li true, otherwise
   */
  auto MaybeSaveAnyTab() -> bool;

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] auto TabCount() const -> int;

  /**
   * @details textpage of the currently activated tab
   * @return \li reference to QTextEdit if tab has one
   *         \li 0 otherwise (e.g. if helppage)
   */
  [[nodiscard]] auto CurTextPage() const -> PlainTextEditorPage*;

  /**
   * @brief
   *
   * @return EMailEditorPage*
   */
  [[nodiscard]] auto CurEMailPage() const -> EMailEditorPage*;

  /**
   * @brief
   *
   * @return FilePage*
   */
  [[nodiscard]] auto CurFilePage() const -> FilePage*;

  /**
   * @details  List of currently unsaved tabs.
   * @returns QHash<int, QString> Hash of tab indexes and title of unsaved tabs.
   */
  [[nodiscard]] auto UnsavedDocuments() const -> QHash<int, QString>;

  /**
   * @details textpage of the currently activated tab
   * @return \li reference to QTextEdit if tab has one
   *         \li 0 otherwise (e.g. if helppage)
   */
  [[nodiscard]] auto CurPlainText() const -> QString;

  /**
   * @brief
   *
   * @return QTabWidget*
   */
  [[nodiscard]] auto TabWidget() const -> QTabWidget*;

  /**
   * @details Return pointer to the currently activated text edit tab page.
   *
   */
  [[nodiscard]] auto CurPageTextEdit() const -> PlainTextEditorPage*;

  /**
   * @details Return pointer to the currently activated file tree view tab page.
   *
   */
  [[nodiscard]] auto CurPageFileTreeView() const -> FilePage*;

 public slots:

  /**
   * @details Insert a ">" at the beginning of every line of current textedit.
   */
  void SlotQuote() const;

  /**
   * @details replace the text of currently active textedit with given text.
   * @param text to fill on.
   */
  void SlotFillTextEditWithText(const QString& text) const;

  /**
   * @details Saves the content of the current tab, if it has a filepath
   * otherwise it calls saveAs for the current tab
   */
  void SlotSave();

  /**
   * @details Opens a savefiledialog and calls save_file with the choosen
   * filename.
   *
   * @return Return the return value of the savefile method
   */
  bool SlotSaveAs();

  /**
   * @details Show an OpenFileDoalog and open the file in a new tab.
   * Shows an error dialog, if the open fails.
   * Set the focus to the tab of the opened file.
   */
  void SlotOpen();

  /**
   * @details Open a print-dialog for the current tab
   */
  void SlotPrint();

  /**
   * @details Adds a new tab with the title "untitled"+countpage+".txt"
   *                      Sets the focus to the new tab. Increase Tab-Count by
   * one
   */
  void SlotNewTab();

  /**
   * @details Adds a new tab with the title "untitled"+countpage+".eml"
   *          Sets the focus to the new tab. Increase Tab-Count by one
   */
  void SlotNewEMailTab();

  /**
   * @details
   *
   */
  void SlotNewTabWithContent(QString title, const QString& content);

  /**
   * @details Adds a new tab with opening file by path
   */
  void SlotOpenFile(const QString& path);

  /**
   * @details Adds a new tab with the given title and opens given html file.
   * Increase Tab-Count by one
   * @param title title for the tab
   * @param path  path for html file to show
   */
  void SlotNewHelpTab(const QString& title, const QString& path) const;

  /**
   * New File Tab to do file operation
   */
  void SlotNewFileBrowserTab();

  /**
   * @details put a * in front of current tabs title, if current textedit is
   * modified
   */
  void SlotShowModified(bool) const;

  /**
   * @details close the current tab and decrease TabWidget->count by \a 1
   *
   */
  void SlotCloseTab();

  /**
   * @details Switch to the next tab.
   *
   */
  void SlotSwitchTabUp() const;

  /**
   * @details Switch to the previous tab.
   *
   */
  void SlotSwitchTabDown() const;

  /**
   * @details Cut selected text in current text page.
   */
  void SlotCut() const;

  /**
   * @details Copy selected text of current text page to clipboard.
   */
  void SlotCopy() const;

  /**
   * @details Paste text from clipboard to current text page.
   */
  void SlotPaste() const;

  /**
   * @details Undo last change in current textpage.
   *
   */
  void SlotUndo() const;

  /**
   * @brief  redo last change in current text page
   *
   */
  void SlotRedo() const;

  /**
   * @brief
   *
   */
  void SlotZoomIn() const;

  /**
   * @brief
   *
   */
  void SlotZoomOut() const;

  /**
   * @brief select all in current text page
   *
   */
  void SlotSelectAll() const;

  /**
   * @brief
   *
   * @param text
   */
  void SlotAppendText2CurTextPage(const QString& text);

  /**
   * @brief
   *
   * @param text
   */
  void SlotSetText2CurEMailPage(const QString& text);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool SlotSaveAsEML();

 protected:
  /**
   * @brief Saves the content of currentTab to the file filename
   *
   * @param fileName
   */
  auto saveFile(const QString& file_name) -> bool;

  /**
   * @brief
   *
   * @return auto
   */
  auto saveEMLFile(const QString& file_name) -> bool;

 private slots:

  /**
   * @details Remove the tab with given index
   *
   * @param index Tab-number to remove
   */
  void slot_remove_tab(int index);

 private:
  /**
   * @details return just a filename stripped of a whole path
   *
   * @param a filename path
   * @return QString containing the filename
   */
  static auto stripped_name(const QString& full_file_name) -> QString;

  /**
   * @brief
   *
   * @param askToSave
   */
  auto maybe_save_current_tab(bool askToSave) -> bool;

  TextEditTabWidget* tab_widget_;  ///< widget containing the tabs of the editor
};

}  // namespace GpgFrontend::UI
