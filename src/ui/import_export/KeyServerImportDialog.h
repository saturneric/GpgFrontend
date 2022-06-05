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

#ifndef __KEY_SERVER_IMPORT_DIALOG_H__
#define __KEY_SERVER_IMPORT_DIALOG_H__

#include <string>

#include "KeyImportDetailDialog.h"
#include "core/GpgContext.h"
#include "ui/GpgFrontendUI.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyServerImportDialog : public QDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Server Import Dialog object
   *
   * @param automatic
   * @param parent
   */
  KeyServerImportDialog(bool automatic, QWidget* parent);

  /**
   * @brief Construct a new Key Server Import Dialog object
   *
   * @param parent
   */
  explicit KeyServerImportDialog(QWidget* parent);

 public slots:

  /**
   * @brief
   *
   * @param keys
   */
  void SlotImport(const KeyIdArgsListPtr& keys);

  /**
   * @brief
   *
   * @param keyIds
   * @param keyserverUrl
   */
  void SlotImport(std::vector<std::string> key_ids_list,
                  std::string keyserver_url);

 signals:

  /**
   * @brief
   *
   */
  void SignalKeyImported();

 private slots:

  /**
   * @brief import key(s) for the key table selection
   *
   */
  void slot_import();

  /**
   * @brief
   *
   */
  void slot_search_finished(QNetworkReply::NetworkError reply,
                            QByteArray buffer);

  /**
   * @brief
   *
   * @param keyid
   */
  void slot_import_finished(QNetworkReply::NetworkError error,
                            QByteArray buffer);

  /**
   * @brief
   *
   */
  void slot_search();

  /**
   * @brief
   *
   */
  void slot_save_window_state();

 private:
  /**
   * @brief Create a keys table object
   *
   */
  void create_keys_table();

  /**
   * @brief Set the message object
   *
   * @param text
   * @param error
   */
  void set_message(const QString& text, bool error);

  /**
   * @brief
   *
   * @param in_data
   */
  void import_keys(ByteArrayPtr in_data);

  /**
   * @brief Set the loading object
   *
   * @param status
   */
  void set_loading(bool status);

  /**
   * @brief Create a button object
   *
   * @param text
   * @param member
   * @return QPushButton*
   */
  QPushButton* create_button(const QString& text, const char* member);

  /**
   * @brief Create a comboBox object
   *
   * @return QComboBox*
   */
  QComboBox* create_comboBox();

  bool m_automatic_ = false;  ///<

  QLineEdit* search_line_edit_{};      ///<
  QComboBox* key_server_combo_box_{};  ///<
  QProgressBar* waiting_bar_;          ///<
  QLabel* search_label_{};             ///<
  QLabel* key_server_label_{};         ///<
  QLabel* message_{};                  ///<
  QLabel* icon_{};                     ///<
  QPushButton* close_button_{};        ///<
  QPushButton* import_button_{};       ///<
  QPushButton* search_button_{};       ///<
  QTableWidget* keys_table_{};         ///<
};

}  // namespace GpgFrontend::UI

#endif  // __KEY_SERVER_IMPORT_DIALOG_H__
