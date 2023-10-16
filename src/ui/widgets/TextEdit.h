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

#ifndef __TEXTEDIT_H__
#define __TEXTEDIT_H__

#include "ui/dialog/QuitDialog.h"
#include "ui/widgets/FilePage.h"
#include "ui/widgets/HelpPage.h"
#include "ui/widgets/PlainTextEditorPage.h"

namespace GpgFrontend::UI {
/**
 * @brief TextEdit class
 */
class TextEdit : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief
   */
  TextEdit(QWidget* parent);

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
  bool MaybeSaveAnyTab();

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] int TabCount() const;

  /**
   * @details textpage of the currently activated tab
   * @return \li reference to QTextEdit if tab has one
   *         \li 0 otherwise (e.g. if helppage)
   */
  [[nodiscard]] PlainTextEditorPage* CurTextPage() const;

  /**
   * @brief
   *
   * @return FilePage*
   */
  [[nodiscard]] FilePage* CurFilePage() const;

  /**
   * @details  List of currently unsaved tabs.
   * @returns QHash<int, QString> Hash of tab indexes and title of unsaved tabs.
   */
  [[nodiscard]] QHash<int, QString> UnsavedDocuments() const;

  QTabWidget* tab_widget_; /** Widget containing the tabs of the editor */

 public slots:

  /**
   * @details Return pointer to the currently activated text edit tab page.
   *
   */
  PlainTextEditorPage* SlotCurPageTextEdit() const;

  /**
   * @details Return pointer to the currently activated file tree view tab page.
   *
   */
  FilePage* SlotCurPageFileTreeView() const;

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
   * @details
   *
   */
  void SlotNewTabWithContent(std::string title, const std::string& content);

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
  void slotNewHelpTab(const QString& title, const QString& path) const;

  /**
   * New File Tab to do file operation
   */
  void SlotNewFileTab() const;

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

 private:
  uint text_page_data_modified_count_ = 0;  ///<

  /**
   * @details return just a filename stripped of a whole path
   *
   * @param a filename path
   * @return QString containing the filename
   */
  static QString stripped_name(const QString& full_file_name);

  /**
   * @brief
   *
   * @param askToSave
   */
  bool maybe_save_current_tab(bool askToSave);

  int count_page_;  ///< int containing the number of added tabs

 private slots:

  void slot_file_page_path_changed(const QString& path) const;

  /**
   * @details Remove the tab with given index
   *
   * @param index Tab-number to remove
   */
  void slot_remove_tab(int index);

  /**
   * @brief
   *
   * @param index
   */
  void slot_save_status_to_cache_for_revovery();

 public slots:

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

 protected:
  /**
   * @brief Saves the content of currentTab to the file filename
   *
   * @param fileName
   */
  bool save_file(const QString& fileName);
};

}  // namespace GpgFrontend::UI

#endif  // __TEXTEDIT_H__
