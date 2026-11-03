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

#include <QPointer>

#include "core/model/GFBuffer.h"
#include "ui/widgets/FilePage.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/TabClosePrompter.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI {

class TextEditTabWidget;
class EditorEmptyView;
class OpenTabsPopup;

/// What became of the tabs a close was asked for.
struct GF_UI_EXPORT TabCloseOutcome {
  QList<quint64> closed;   ///< removed (or, quitting, free to go)
  QList<quint64> kept;     ///< still open: a save failed, or the view objected
  bool cancelled = false;  ///< the user cancelled; nothing was closed
};

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
// Exported whole rather than member by member: CurrentEditorContent() casts a
// registered QObject back to this type, and a qobject_cast needs
// staticMetaObject visible, which a per-member export does not give.
class GF_UI_EXPORT TextEdit : public QWidget {
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
   * @brief Close the tabs @p ids, asking about unsaved changes once.
   *
   * The one path every close takes: a tab's button, middle click, the menus,
   * the open-tabs list. Nothing is closed or wiped until every question has
   * been answered and every save asked for has succeeded; Cancel leaves every
   * tab open, and a tab whose save failed stays open with its content. Ids of
   * tabs already gone are ignored.
   */
  auto CloseTabs(const QList<quint64>& ids) -> TabCloseOutcome;

  /// Replace the dialogs closing tabs asks with (tests).
  void SetTabClosePrompter(std::unique_ptr<TabClosePrompter> prompter);

  /**
   * @brief The ways to start a document the empty editor offers, asked each
   *        time it is shown, so module actions added later appear too. Open
   *        File is always offered after them.
   */
  void SetEmptyViewActions(std::function<QList<QAction*>()> provider);

  /**
   * @brief Wipes every open tab on the way out.
   *
   * Closing a tab wipes its page, but quitting did not: MaybeSaveAnyTab()
   * either caches or saves and never closes a page, so the content sat in the
   * heap until the process died. Only closes the window a little sooner than
   * the kernel would zero the pages anyway, so this narrows the swap and
   * core-dump window rather than the long exposure that tab close covers.
   */
  void WipeAllTabs();

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
   * @brief Returns the plain text of the current text editor page.
   *
   * @return Current plain text, or an empty string if the current tab is not a
   *         text editor page.
   */
  [[nodiscard]] auto CurPlainText() const -> QString;

  /**
   * @brief Asks the current tab's module view to write itself back first, then
   * returns the tab's text.
   *
   * A module-backed tab keeps its structured view and the text document in
   * step lazily, so reading the document straight after an edit can hand back
   * stale bytes. Every caller that is about to *act* on the content -- a save,
   * a crypto operation -- must come through here instead of CurPlainText().
   *
   * Identical to CurPlainText() on a tab with no module view.
   *
   * @return The tab's text, or an empty string when there is no text page.
   */
  [[nodiscard]] auto CurPlainTextForOperation() const -> QString;

  /**
   * @brief The same, as the document's own bytes.
   *
   * Flushes the module view like CurPlainTextForOperation(), then hands back
   * DocumentBytes() rather than text -- line endings included, exactly as the
   * document holds them.
   *
   * For the operations that JUDGE content rather than produce it. A signature
   * covers exact octets, so a verify has to be given the document as it is:
   * normalizing first would answer for a repaired copy and report a good
   * signature over a document that does not have one.
   *
   * @return The tab's bytes, or an empty array when there is no text page.
   */
  [[nodiscard]] auto CurDocumentBytesForOperation() const -> QByteArray;

  /**
   * @brief Hands a verification result to a page's mounted module view.
   *
   * Addressed to THAT page rather than to whichever is current when the answer
   * arrives, for the same reason SetGFBuffer2Page() is: an operation's result
   * comes back an unbounded time later, by which point the tab may have been
   * switched away from or closed.
   *
   * A view that does not declare ApplyVerificationResult simply does not take
   * part; the contract is additive.
   *
   * @return true when a view was given the result.
   */
  auto ApplyVerificationToPage(const QPointer<QWidget>& page,
                               const QByteArray& payload) -> bool;

  /**
   * @brief Returns the underlying tab widget.
   *
   * @return Pointer to the internal TextEditTabWidget.
   */
  [[nodiscard]] auto TabWidget() const -> TextEditTabWidget*;

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
   * @brief Whether the current tab is an ordinary plain-text document.
   *
   * A module-backed tab (an e-mail, say) IS a PlainTextEditorPage, so
   * CurPageTextEdit() != nullptr is true for it and cannot answer this. Its
   * document is a structured format the module owns, and an operation that
   * rewrites the editor's text wholesale would corrupt it.
   */
  [[nodiscard]] auto CurPageIsPlainText() const -> bool;

  /// Which crypto operations the current tab's mounted view says apply now.
  /// @p has_opinion is false when no view answered.
  [[nodiscard]] auto CurPageCryptoOperations(bool& has_opinion) const
      -> QStringList;

  /**
   * @brief Offers an exported public key to the current tab's mounted view.
   *
   * @return 0 when nothing handled it and the caller should fall back to
   * appending armor, 1 when the view took it, 2 when the view refused
   */
  auto AttachPublicKeyToCurPage(const QByteArray& key, const QString& name)
      -> int;

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

  /**
   * @brief Why an operation result may or may not be written to a page.
   */
  enum class ResultTarget {
    kDeliver,        ///< alive, still one of our tabs, and can hold text
    kPageDestroyed,  ///< closed and deleted while the operation was running
    kPageDetached,   ///< alive, but no longer one of our tabs
    kNotATextPage,   ///< ours, but not a document that can take the bytes
  };

  /**
   * @brief Decides whether a result may be delivered to its originating page.
   *
   * Split out as a pure function so the policy can be exercised without a
   * widget -- the unit tests run off the GUI thread and cannot build one.
   *
   * Only kDeliver may write. Every other answer discards the result: there is
   * deliberately no fallback to the current tab and no "open a new tab for
   * it", because either would put the output of one document's operation --
   * a decrypted message, say -- into a different document.
   */
  [[nodiscard]] static auto ClassifyResultTarget(bool page_alive, int tab_index,
                                                 bool is_text_page)
      -> ResultTarget;

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
   * @brief Open a document of @p type holding @p content, in a new tab.
   *
   * What a module used to do by finding this widget and invoking
   * SlotNewCustomTab, SetContentFromBytes and SetFilePath on it by name. The
   * type decides which view the tab gets, exactly as for a tab the user
   * opened.
   *
   * @param saved whether @p content is exactly what @p path holds on disk
   * @return the new document's id
   */
  auto OpenDocument(const QString& type, const QString& title,
                    const QString& path, const GFBuffer& content, bool saved,
                    bool modified = false) -> qint64;

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

  /// Closes every tab, asking once about the unsaved ones.
  void SlotCloseAllTabs();

  /**
   * @brief Switches to the next tab in visual order.
   */
  void SlotSwitchTabUp() const;

  /**
   * @brief Switches to the previous tab in visual order.
   */
  void SlotSwitchTabDown() const;

  /// Ctrl+Tab: towards the tabs used longer ago.
  void SlotSwitchTabRecent();

  /// Ctrl+Shift+Tab: back towards the tabs used most recently.
  void SlotSwitchTabRecentBack();

  /// Shows the searchable list of open tabs.
  void SlotShowOpenTabs();

  /// Asks for a file and opens it in a tab.
  void SlotOpenFileDialog();

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
   * @brief Replaces the current text editor content with a GFBuffer.
   *
   * If no text editor is active, a new text tab is created first.
   *
   * @param buffer Buffer to convert and insert.
   */
  void SlotSetGFBuffer2CurTextPage(const GFBuffer& buffer);

  /**
   * @brief Writes operation output back to the page that ASKED for it.
   *
   * A crypto operation runs on another thread and its callback arrives an
   * unbounded time later. By then the user may have switched tabs, or closed
   * the one the operation started from. Writing to whatever tab happens to be
   * current at that moment puts a decrypted message into an unrelated
   * document -- and because SlotSetGFBuffer2CurTextPage() opens a new tab when
   * the current widget is not a text page, it could even mint a fresh tab and
   * put the plaintext there.
   *
   * So the result is addressed to a specific page, and delivered only if that
   * exact page is still one of this widget's tabs. @p page is taken as a
   * QPointer so a destroyed page reads as null rather than as a stale
   * address.
   *
   * @param page the page that initiated the operation
   * @param buffer the bytes to write
   * @return true if the bytes were written; false if the page is gone or is
   *         no longer a text page, in which case nothing is written anywhere
   */
  auto SetGFBuffer2Page(const QPointer<QWidget>& page, const GFBuffer& buffer)
      -> bool;

  /**
   * @brief Returns the underlying tab widget.
   *
   * Slot-style accessor kept for signal/slot based callers.
   *
   * @return Pointer to the internal TextEditTabWidget.
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
   * @brief Closes the tab at @p index through CloseTabs().
   *
   * @param index Tab index to remove.
   */
  void slot_remove_tab(int index);

 private:
  TextEditTabWidget* tab_widget_;  ///< Tab widget containing workspace pages.
  QStackedLayout* stack_ = nullptr;
  EditorEmptyView* empty_view_ = nullptr;
  QPointer<OpenTabsPopup> open_tabs_popup_;
  std::unique_ptr<TabClosePrompter> prompter_;

  /// Write @p page to @p file_name; false when nothing was written.
  auto save_page(PlainTextEditorPage* page, const QString& file_name) -> bool;

  /// Ask where to save @p page, then save it; false when not saved.
  auto save_page_as(PlainTextEditorPage* page) -> bool;

  /**
   * @brief The close path: assess, ask, save, then (with @p teardown) remove.
   *
   * @param cache_plain unsaved plain-text tabs go to the recovery cache rather
   *        than being asked about (quitting with restore enabled)
   */
  auto run_close(const QList<quint64>& ids, bool teardown, bool cache_plain)
      -> TabCloseOutcome;

  /// Show the tabs, or the empty view when there are none.
  void update_empty_view();

  /**
   * @brief Extracts the base file name from a full path.
   *
   * @param full_file_name Full file path.
   * @return File name without parent directory path.
   */
  static auto stripped_name(const QString& full_file_name) -> QString;
};

}  // namespace GpgFrontend::UI