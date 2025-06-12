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
 * @brief Class for handling a single tab of the tabwidget
 *
 */
class PlainTextEditorPage : public QWidget {
  Q_OBJECT
 public:
  /**
   * @details Add layout and add plaintextedit
   *
   * @param file_path Path of the file handled in this tab
   * @param parent Pointer to the parent widget
   */
  explicit PlainTextEditorPage(QString file_path = {},
                               QWidget* parent = nullptr);

  /**
   * @details Get the filepath of the currently activated tab.
   */
  [[nodiscard]] auto GetFilePath() const -> const QString&;

  /**
   * @details Set filepath of currently activated tab.
   *
   * @param filePath The path to be set
   */
  void SetFilePath(const QString& filePath);

  /**
   * @details Return pointer tp the textedit of the currently activated tab.
   */
  auto GetTextPage() -> QPlainTextEdit*;

  /**
   * @brief Get the Plain Text object
   *
   * @return QString
   */
  auto GetPlainText() -> QString;

  /**
   * @details Show additional widget at buttom of currently active tab
   *
   * @param widget The widget to be added
   * @param className The name to handle the added widget
   */
  void ShowNotificationWidget(QWidget* widget, const char* className);

  /**
   * @details Hide all widgets with the given className
   *
   * @param className The classname of the widgets to hide
   */
  void CloseNoteByClass(const char* className);

  /**
   * @brief
   *
   */
  void ReadFile();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto ReadDone() const -> bool;

  /**
   * @brief notify the user that the file has been saved.
   *
   */
  void NotifyFileSaved();

  /**
   * @brief
   *
   */
  void Clear();

 signals:

  /**
   * @brief this signal is emitted when the bytes has been append in texteditor.
   *
   */
  void SignalUIBytesDisplayed();

 protected:
  QSharedPointer<Ui_PlainTextEditor> ui_;  ///<

  void closeEvent(QCloseEvent* event) override;

 private:
  QString full_file_path_;  ///< The path to the file handled in the tab
  bool sign_marked_{};  ///< true, if the signed header is marked, false if not
  bool read_done_ = false;  ///<
  size_t read_bytes_ = 0;   ///<
  bool is_crlf_ = false;    ///<
  bool last_insert_has_partial_cr_ = false;

 private slots:

  /**
   * @details Format the gpg header in another font-style
   */
  void slot_format_gpg_header();

  /**
   * @brief
   *
   * @param data
   */
  void slot_insert_text(QByteArray bytes_data);
};

}  // namespace GpgFrontend::UI
