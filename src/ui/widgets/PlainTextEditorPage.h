/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef __EDITORPAGE_H__
#define __EDITORPAGE_H__

#include "gpg/GpgConstants.h"
#include "ui/GpgFrontendUI.h"

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
   * @param filePath Path of the file handled in this tab
   * @param parent Pointer to the parent widget
   */
  explicit PlainTextEditorPage(QString filePath = "",
                               QWidget* parent = nullptr);

  /**
   * @details Get the filepath of the currently activated tab.
   */
  [[nodiscard]] const QString& getFilePath() const;

  /**
   * @details Set filepath of currently activated tab.
   *
   * @param filePath The path to be set
   */
  void setFilePath(const QString& filePath);

  /**
   * @details Return pointer tp the textedit of the currently activated tab.
   */
  QPlainTextEdit* getTextPage();

  /**
   * @details Show additional widget at buttom of currently active tab
   *
   * @param widget The widget to be added
   * @param className The name to handle the added widget
   */
  void showNotificationWidget(QWidget* widget, const char* className);

  /**
   * @details Hide all widgets with the given className
   *
   * @param className The classname of the widgets to hide
   */
  void closeNoteByClass(const char* className);

  void ReadFile();

  [[nodiscard]] bool ReadDone() const { return this->read_done_; }

  void PrepareToDestroy();

 private:
  std::shared_ptr<Ui_PlainTextEditor> ui;
  QString full_file_path_; /** The path to the file handled in the tab */
  bool signMarked{}; /** true, if the signed header is marked, false if not */
  bool read_done_ = false;
  QThread* read_thread_ = nullptr;
  bool binary_mode_ = false;
  size_t read_bytes_ = 0;

  void detect_encoding(const std::string& data);

  void detect_cr_lf(const QString& data);

 private slots:

  /**
   * @details Format the gpg header in another font-style
   */
  void slotFormatGpgHeader();

  void slotInsertText(const std::string& data);
};

}  // namespace GpgFrontend::UI

#endif  // __EDITORPAGE_H__
