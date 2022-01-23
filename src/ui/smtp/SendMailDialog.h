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

#ifndef GPGFRONTEND_SENDMAILDIALOG_H
#define GPGFRONTEND_SENDMAILDIALOG_H

#include "ui/GpgFrontendUI.h"

class Ui_SendMailDialog;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class SendMailDialog : public QDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Send Mail Dialog object
   *
   * @param text
   * @param parent
   */
  explicit SendMailDialog(const QString& text, QWidget* parent = nullptr);

  /**
   * @brief Set the Content Encryption object
   *
   * @param on
   */
  void SetContentEncryption(bool on);

  /**
   * @brief Set the Attach Signature object
   *
   * @param on
   */
  void SetAttachSignature(bool on);

 private slots:

  /**
   * @brief
   *
   */
  void slot_confirm();

  /**
   * @brief
   *
   * @param result
   */
  void slot_test_smtp_connection_result(const QString& result);

 private:
  /**
   * @brief
   *
   */
  void init_settings();

  std::shared_ptr<Ui_SendMailDialog> ui_;  ///<

  bool ability_enable_ = false;                ///<
  bool identity_enable_ = false;               ///<
  QString smtp_address_;                       ///<
  QString username_;                           ///<
  QString password_;                           ///<
  QString default_sender_;                     ///<
  QString connection_type_settings_ = "None";  ///<
  QString default_sender_gpg_key_id_ = {};     ///<
  int port_ = 25;                              ///<

  GpgFrontend::KeyId sender_key_id_;  ///<
  GpgFrontend::KeyIdArgsListPtr recipients_key_ids_ =
      std::make_unique<GpgFrontend::KeyIdArgsList>();  ///<

  QRegularExpression re_email_{
      R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};

  /**
   * @brief
   *
   * @param str
   * @return true
   * @return false
   */
  bool check_email_address(const QString& str);

  /**
   * @brief Set the sender value label object
   *
   */
  void set_sender_value_label();

  /**
   * @brief Set the recipients value label object
   *
   */
  void set_recipients_value_label();
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SENDMAILDIALOG_H
