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

#include <QtNetwork>

#include "KeyImportDetailDialog.h"
#include "core/typedef/CoreTypedef.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyServerImportDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Key Server Import Dialog object
   *
   * @param automatic
   * @param parent
   */
  explicit KeyServerImportDialog(int channel, QWidget* parent);

 public slots:

  /**
   * @brief
   *
   * @param keys
   */
  void SlotImport(const KeyIdArgsList& keys);

  /**
   * @brief
   *
   * @param keyIds
   * @param keyserverUrl
   */
  void SlotImport(KeyIdArgsList key_ids_list, QString keyserver_url);

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
                            QString err_string, QByteArray buffer);

  /**
   * @brief
   *
   * @param keyid
   */
  void slot_import_finished(int channel, bool success, QString err_msg,
                            QByteArray buffer,
                            QSharedPointer<GpgImportInformation> info);

  /**
   * @brief
   *
   */
  void slot_search();

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
  void import_keys(ByteArray in_data);

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
  QComboBox* create_combo_box();

  QHBoxLayout* message_layout_;  ///<

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

  int current_gpg_context_channel_;
};

}  // namespace GpgFrontend::UI
