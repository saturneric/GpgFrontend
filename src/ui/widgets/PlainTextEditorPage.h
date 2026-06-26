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

class Ui_PlainTextEditor;

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
   * Before clearing, existing text is overwritten with bullet characters to
   * reduce the chance that sensitive content remains in the document buffer.
   * Undo/redo history is cleared and the document is marked as unmodified.
   */
  void Clear();

  /**
   * @brief Reapplies editor appearance settings.
   *
   * This reloads the preferred monospace font and configured editor font size,
   * then updates the tab stop width.
   */
  void ApplyAppearanceSettings();

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

 signals:
  /**
   * @brief Emitted after a chunk of file bytes has been displayed in the
   * editor.
   *
   * FileReadTask uses this signal as a back-pressure mechanism. After the UI
   * has inserted one chunk, this signal requests the next chunk.
   */
  void SignalUIBytesDisplayed();

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
  QString full_file_path_;  ///< File path associated with this editor page.
  bool sign_marked_{};  ///< Whether OpenPGP signature metadata was formatted.
  bool read_done_ = false;  ///< Whether asynchronous file loading has finished.
  size_t read_bytes_ = 0;   ///< Number of file bytes inserted into the editor.
  bool is_crlf_ = false;    ///< Whether CRLF line endings were detected.
  bool last_insert_has_partial_cr_ =
      false;  ///< Whether previous chunk ended with '\r'.
  QTimer* sha256_timer_ = nullptr;  ///< Debounce timer for SHA-256 updates.

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
};

}  // namespace GpgFrontend::UI
