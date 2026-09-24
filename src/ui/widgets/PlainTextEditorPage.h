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

#include "ui/function/TextDirection.h"

class Ui_PlainTextEditor;
class QActionGroup;

class QLabel;
class QToolButton;

namespace GpgFrontend::UI {

/**
 * @brief Plain-text editor page used as one editor tab.
 *
 * PlainTextEditorPage represents one text editing page in the main window. It
 * owns a QPlainTextEdit-based editor, optional notification widgets, and a
 * small status area showing cursor position, character count, line-ending style
 * and encoding.
 *
 * If a file path is provided, the page can load the file asynchronously through
 * FileReadTask. File contents are inserted in chunks to keep the UI responsive.
 * During loading, the editor is disabled and the loading label shows progress.
 *
 * The page also applies editor appearance settings, uses a preferred monospace
 * font for each platform, and lightly formats OpenPGP cleartext signature
 * headers after loading.
 */
class PlainTextEditorPage : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Constructs a plain-text editor page.
   *
   * The page initializes the editor UI, status labels and appearance settings.
   * If @p file_path is empty, the page is immediately marked as ready. If a
   * file path is provided, the page starts in loading state until ReadFile()
   * finishes.
   *
   * @param file_path File path associated with this editor page. May be empty
   *                  for a new unsaved page.
   * @param parent Parent widget.
   */
  explicit PlainTextEditorPage(QString file_path = {},
                               QWidget* parent = nullptr);

  ~PlainTextEditorPage() override;

  /**
   * @brief Returns the full plain text currently shown in the editor.
   *
   * @return Editor content as plain text.
   */
  auto GetPlainText() -> QString;

  /**
   * @brief Shows an additional notification widget below the editor.
   *
   * The widget is assigned a dynamic property named by @p className, so it can
   * later be found and closed by CloseNoteByClass().
   *
   * @param widget Notification widget to add to the page layout.
   * @param className Dynamic property name used to identify the widget group.
   */
  void ShowNotificationWidget(QWidget* widget, const char* className);

  /**
   * @brief The document's text, in the line-ending convention it came with.
   *
   * GetPlainText() returns what QPlainTextEdit stores, which is always bare
   * LF. This restores CRLF for a document that was loaded as CRLF, so callers
   * that write the document out, or hand it to something that reads exact
   * bytes, see the file rather than the editor's internal form.
   */
  [[nodiscard]] auto DocumentBytes() const -> QByteArray;

  /**
   * @brief Mount a module's native document widget as the primary view.
   *
   * The Host talks to the view only
   * through its GFDocumentWidgetOps, keyed by @p instance, and never hands
   * it the page's own editor -- switching to the raw source is the page's
   * switcher, which the view may only ask for.
   *
   * @param widget_id the native widget registration to build one of
   */
  auto MountNativeView(const QString& widget_id) -> bool;

  /// The native instance mounted here, or 0.
  [[nodiscard]] auto NativeInstanceId() const -> quint64 {
    return native_instance_;
  }

  /**
   * @brief Let the mounted view check and normalise what is about to be
   *        written to a file. It may ask the user.
   *
   * @return the bytes to write, or nullopt when the user cancelled
   */
  auto PrimaryViewPrepareSave(const QByteArray& bytes)
      -> std::optional<QByteArray>;

  /// The default file suffix a mounted native view declared, or empty.
  [[nodiscard]] auto PrimaryViewDefaultSuffix() const -> QString;

  /**
   * @brief Offers an exported public key to the mounted view.
   *
   * A structured document has somewhere better to put a key than the middle
   * of its own bytes, so the view is asked first. Returns 0 (not handled) when
   * there is no view or it declares no such member, which is the caller's
   * signal to fall back to appending the armor as text.
   *
   * @return the view's add-content result: 0 not handled, 1 done, 2 refused
   */
  auto AttachPublicKeyToPrimaryView(const QByteArray& key, const QString& name)
      -> int;

  /**
   * @brief Offers text to the mounted view rather than to the document.
   *
   * Same contract as AttachPublicKeyToPrimaryView().
   */
  auto AppendTextToPrimaryView(const QString& text) -> int;

  /**
   * @brief Hands a verification result to the mounted view, if it takes one.
   *
   * Optional half of the page/view contract. The host performs the one
   * verification there is; a structured view is what renders it, and this is
   * how the answer reaches it.
   *
   * @return true when a view took the result.
   */
  auto ApplyVerificationToPrimaryView(const QByteArray& payload) -> bool;

  /**
   * @brief Which crypto operations the mounted view says apply right now.
   *
   * An empty list means the view has no opinion -- either there is none, or
   * it declares no such member -- and the caller should fall back to whatever
   * the tab type alone allows. @p has_opinion distinguishes that from a view
   * that answered "none of them".
   */
  auto PrimaryViewCryptoOperations(bool& has_opinion) const -> QStringList;

  /**
   * @brief The mounted primary view, or nullptr when the tab has none.
   */
  [[nodiscard]] auto PrimaryView() const -> QWidget*;

  /**
   * @brief Asks the primary view to reserialize itself into the document.
   *
   * A no-op when there is no primary view, or when it does not declare
   * FlushToSource(). Callers use this before reading the document for a save
   * or a crypto operation, so the bytes they read are never stale.
   */
  void FlushPrimaryView();

  /**
   * @brief A name the mounted view suggests for saving this tab, if any.
   *
   * Only the view knows what its content is called -- a message has a subject,
   * a tab title does not. Empty when there is no view, when it does not offer
   * one, or when it has nothing to suggest; the caller then keeps its own
   * guess. The name is already safe to use as a single path component.
   */
  [[nodiscard]] auto PrimaryViewSuggestedFileName() const -> QString;

  /// A QFileDialog filter for this tab's content type, or empty for the
  /// host's default.
  [[nodiscard]] auto PrimaryViewFileTypeFilter() const -> QString;

  /**
   * @brief Whether the primary view holds edits it has not written back yet.
   *
   * Only meaningful between an edit and the flush that follows it; the
   * document's own modified flag is what the rest of the application consults,
   * and the view marks that eagerly.
   *
   * @return false when there is no primary view or it declares no IsDirty().
   */
  [[nodiscard]] auto PrimaryViewIsDirty() const -> bool;

  /**
   * @brief Hands the document's current bytes to the primary view.
   *
   * Used when content arrives from outside the view -- a file being opened, or
   * the result of a crypto operation. A no-op without a primary view.
   */
  void ReloadPrimaryView();

  /**
   * @brief Closes notification widgets with a matching dynamic property.
   *
   * Every child widget whose @p className property is true will be closed.
   *
   * @param className Dynamic property name used to identify widgets to close.
   */
  void CloseNoteByClass(const char* className);

  /**
   * @brief Starts asynchronous loading of the associated file.
   *
   * The editor is cleared, disabled, and filled incrementally by FileReadTask.
   * Undo/redo and document signals are temporarily blocked during loading. When
   * reading finishes, the editor is re-enabled, marked as unmodified, the
   * status bar is refreshed, and OpenPGP cleartext signature headers are
   * formatted.
   */
  void ReadFile();

  /**
   * @brief Returns whether file loading has completed.
   *
   * New pages without an associated file path are considered ready immediately.
   *
   * @return true if the editor is ready for normal editing, otherwise false.
   */
  [[nodiscard]] auto ReadDone() const -> bool;

  /**
   * @brief Clears editor content and resets editor state.
   *
   * Undo and redo history is discarded first, so the content cannot be
   * restored, and the document is marked unmodified. See WipeTextDocument()
   * for what clearing a QTextDocument does and does not achieve.
   */
  void Clear();

  /**
   * @brief Wipes the content this page is holding.
   *
   * Called explicitly when the tab is closed and when the window is closing,
   * rather than relying on closeEvent reaching an already removed, hidden and
   * unparented widget. Idempotent, so the belt-and-braces call from closeEvent
   * costs nothing.
   */
  void WipeContent();

  /**
   * @brief Reapplies editor appearance settings.
   *
   * This reloads the preferred monospace font and configured editor font size,
   * then updates the tab stop width.
   */
  void ApplyAppearanceSettings();

  /**
   * @brief Overrides the direction this page lays its text out in.
   *
   * The mode is not stored: it is a view choice for this tab and this session.
   * Every tab opens in kTEXT_DIRECTION_AUTO, which is the mode that lets each
   * paragraph follow its own content; the explicit modes are the override.
   *
   * Also moves the check mark in the menu returned by
   * TextDirectionMenuAction(), so a change made anywhere shows up everywhere
   * that menu is shown.
   *
   * @param mode Direction mode to apply.
   */
  void SetTextDirectionMode(TextDirectionMode mode);

  /**
   * @brief Returns the direction mode configured for this page.
   *
   * @return The mode, which may be kTEXT_DIRECTION_AUTO.
   */
  [[nodiscard]] auto GetTextDirectionMode() const -> TextDirectionMode;

  /**
   * @brief Returns the action opening this page's text direction submenu.
   *
   * The page owns the three mode actions, and this is how anything else shows
   * them. The main window puts these very actions in its View menu rather than
   * building a second set, so the two menus cannot disagree and each label is
   * translated once.
   *
   * @return Menu action owned by this page; never null.
   */
  [[nodiscard]] auto TextDirectionMenuAction() const -> QAction*;

 public slots:
  /**
   * @brief Returns the file path associated with this editor page.
   *
   * @return Full file path, or an empty string for an unsaved page.
   */
  QString GetFilePath();  // NOLINT

  /**
   * @brief Marks the document as saved and refreshes the editor status.
   *
   * This clears the modified flag and updates the status label tooltip and
   * modified marker.
   */
  void NotifyFileSaved();

  /**
   * @brief Returns the underlying text editor widget.
   *
   * @return Pointer to the QPlainTextEdit used by this page.
   */
  QPlainTextEdit* GetTextPage();  // NOLINT

  /**
   * @brief Updates the file path associated with this editor page.
   *
   * @param filePath New full file path.
   */
  void SetFilePath(const QString& filePath);

  /**
   * @brief Fills the document from raw file bytes, keeping their line endings.
   *
   * QPlainTextEdit always stores line breaks as bare LF, so the convention the
   * bytes arrived with is recorded here rather than lost. DocumentBytes() and
   * saving then reproduce it.
   *
   * Use this instead of setting the text directly whenever the exact bytes
   * matter: a signed message verifies over the octets it arrived as, and a
   * caller that normalises line endings on the way in has already broken it.
   */
  void SetContentFromBytes(const QByteArray& bytes);

  /**
   * @brief Fills the document with the bytes an in-app operation produced.
   *
   * The other half of SetContentFromBytes(): same byte-exact handling of line
   * endings, different history. These bytes are a decryption, a signature, a
   * ciphertext -- they exist nowhere but in this document, so the page comes
   * out MODIFIED rather than matching a file on disk.
   *
   * Named rather than a flag on SetContentFromBytes() because the two differ
   * in what the document then is, not in a setting: one has a file behind it
   * and one does not.
   */
  void SetOperationResultBytes(const QByteArray& bytes);

 signals:
  /**
   * @brief Emitted after a chunk of file bytes has been displayed in the
   * editor.
   *
   * FileReadTask uses this signal as a back-pressure mechanism. After the UI
   * has inserted one chunk, this signal requests the next chunk.
   */
  void SignalUIBytesDisplayed();

  /**
   * @brief Emitted when the direction mode chosen for this page changes.
   *
   * Fires on an actual change of mode, not on every edit: under
   * kTEXT_DIRECTION_AUTO the paragraphs re-resolve themselves during layout,
   * with nothing for anyone to be told about.
   */
  void SignalTextDirectionModeChanged();

  /**
   * @brief Emitted when the mounted view asks for a crypto operation.
   *
   * The view says which of the operations the host already routes per tab
   * type the user asked for -- "encrypt", "decrypt", "sign", "verify",
   * "encrypt_sign" or "decrypt_verify" -- and nothing more. Performing it
   * stays entirely with the host, so an operation reached from inside a
   * message and one reached from the menu bar are the same code path.
   */
  void SignalCryptoOperationRequested(const QString& operation);

  /// Emitted when the mounted view's answer about applicable operations
  /// changed, so whoever drives a menu from it can ask again.
  void SignalCryptoOperationsChanged();

 protected:
  QSharedPointer<Ui_PlainTextEditor> ui_;  ///< Generated editor page UI object.

  /**
   * @brief Clears editor content before the page is closed.
   *
   * @param event Close event.
   */
  void closeEvent(QCloseEvent* event) override;

 private slots:
  /**
   * @brief Marks the document modified because the primary view was edited.
   *
   * Deliberately does not reserialize: the flag has to be correct immediately
   * so that closing the tab prompts to save, while writing the document back
   * is deferred until something actually needs to read it.
   */
  void slot_primary_view_modified();

  /**
   * @brief Applies a subdued text style to OpenPGP cleartext signature
   * metadata.
   *
   * The OpenPGP signed message header and signature block are formatted with a
   * smaller gray font. The method runs only once per loaded content.
   */
  void slot_format_gpg_header();

  /**
   * @brief Computes SHA-256 of the current editor content and updates the
   * footer label. The abbreviated hash is shown as text; the full hash is in
   * the tooltip.
   */
  void slot_update_sha256();

  /**
   * @brief Inserts one chunk of file bytes into the editor.
   *
   * The byte chunk is decoded as UTF-8, inserted into the editor, and counted
   * as loaded progress. CRLF line endings are detected across chunk boundaries.
   * After insertion, the next read chunk is requested shortly afterwards.
   *
   * @param bytes_data File bytes read by FileReadTask.
   */
  void slot_insert_text(QByteArray bytes_data);

 private:
  friend class NativeDocumentPort;

  quint64 native_instance_ = 0;  ///< the native view's instance, or 0
  std::unique_ptr<class NativeDocumentPort> native_port_;

  /// For a native view: the raw source is read-only until the user unlocks
  /// it, and unlocking asks the view first -- signed bytes, say, must not be
  /// edited by accident. The row lives in the page's own switcher.
  QLabel* source_notice_ = nullptr;
  QToolButton* source_unlock_ = nullptr;
  bool source_unlocked_ = false;
  void refresh_source_lock();

  /**
   * @brief Mounts a module-supplied widget as this page's primary view.
   *
   * The page keeps owning the text document, which stays the canonical content
   * of the tab: saving, crash recovery, the unsaved-changes prompt and
   * CurPlainText() all keep reading it. The mounted widget is shown above the
   * editor and a switcher lets the user move between it ("Message") and the
   * raw document ("Raw Source").
   *
   * Ownership of @p view passes to the page. Mounting a second view is
   * rejected; a page has one primary view for its whole life. Reached only
   * through MountNativeView().
   *
   * @param view Fresh, unparented widget. Must not be nullptr.
   * @return true when the view was mounted.
   */
  auto mount_primary_view(QWidget* view) -> bool;
  void set_source_unlocked(bool on);

  /// The native view's module was withdrawn: drop its view, show the source.
  void withdraw_native_view();

  QString full_file_path_;  ///< File path associated with this editor page.
  bool sign_marked_{};  ///< Whether OpenPGP signature metadata was formatted.
  bool read_done_ = false;  ///< Whether asynchronous file loading has finished.
  size_t read_bytes_ = 0;   ///< Number of file bytes inserted into the editor.
  bool is_crlf_ = false;    ///< Whether CRLF line endings were detected.
  bool last_insert_has_partial_cr_ =
      false;                        ///< Whether previous chunk ended with '\r'.
  QTimer* sha256_timer_ = nullptr;  ///< Debounce timer for SHA-256 updates.
  TextDirectionMode text_direction_mode_ =
      kTEXT_DIRECTION_AUTO;               ///< Configured direction mode.
  QMenu* text_direction_menu_ = nullptr;  ///< Submenu holding the mode actions.
  QPointer<QWidget> primary_view_;        ///< Module-supplied view, or null.
  QWidget* view_switcher_ = nullptr;      ///< Message / Raw Source selector.
  /// Set while content is being moved between the view and the document, in
  /// either direction. Both handlers bail out on it, which is what stops a
  /// write in one direction bouncing straight back as a write in the other.
  bool primary_view_syncing_ = false;
  /// Document revision as of the last sync. A contentsChanged carrying this
  /// revision is the echo of our own write, not an external edit -- the check
  /// that catches what the flag above misses when the signal is queued.
  int source_generation_ = -1;
  QActionGroup* text_direction_group_ =
      nullptr;  ///< Makes the three mode actions exclusive.

  /**
   * @brief Builds the Message / Raw Source selector shown above both views.
   */
  void build_view_switcher();

  /**
   * @brief Shows either the mounted view or the raw document.
   *
   * Switching away from the mounted view flushes it first, so what the user
   * reads as "Raw Source" is never behind the structured view.
   *
   * @param primary true for the mounted view, false for the editor.
   */
  void show_primary_view(bool primary);

  /**
   * @brief Applies the configured mode to the editor.
   */
  void apply_text_direction();

  /**
   * @brief Builds the text direction submenu and attaches it to the editor.
   *
   * The actions are added to the editor widget as well, which is what puts them
   * in its context menu: PlainTextEditor appends whatever actions it carries to
   * the standard menu.
   */
  void build_text_direction_menu();

  /**
   * @brief Initializes editor styling, status labels and page stylesheet.
   *
   * This configures the QPlainTextEdit, chooses a platform-preferred monospace
   * font, applies the configured editor font size, initializes status labels,
   * and installs the page stylesheet.
   */
  void init_editor_style();

  /**
   * @brief Updates cursor position, character count, line ending and encoding.
   *
   * The status label also shows a modified marker when the document has unsaved
   * changes.
   */
  void update_status_bar();

  /**
   * @brief Enables or disables loading mode.
   *
   * In loading mode, the loading label is shown and the editor is disabled and
   * read-only. In normal mode, the loading label is hidden and editing is
   * enabled.
   *
   * @param loading true to enter loading mode, false to leave it.
   * @param message Optional loading message.
   */
  void set_loading_state(bool loading, const QString& message = {});

  /**
   * @brief Updates the editor modified-state hint shown to the user.
   *
   * @param modified true if the document has unsaved changes, otherwise false.
   */
  void set_editor_modified(bool modified);

  /**
   * @brief Puts raw bytes into the document, remembering their line endings.
   *
   * The ONLY place is_crlf_ is decided. Recorded before the text goes in,
   * because once it is in the document the evidence is gone: the editor
   * stores bare LF either way, and a caller that wrote through the
   * QPlainTextEdit directly silently threw the convention away -- which is
   * what made a freshly signed message stop verifying.
   */
  void load_bytes_into_editor(const QByteArray& bytes);
};

}  // namespace GpgFrontend::UI
