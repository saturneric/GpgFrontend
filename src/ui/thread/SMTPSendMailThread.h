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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_SMTPSENDMAILTHREAD_H
#define GPGFRONTEND_SMTPSENDMAILTHREAD_H

#include <utility>

#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {
/**
 * @brief
 *
 */
class SMTPSendMailThread : public QThread {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new SMTPSendMailThread object
   *
   * @param host
   * @param port
   * @param connection_type
   * @param identify
   * @param username
   * @param password
   * @param parent
   */
  explicit SMTPSendMailThread(std::string host, int port,
                              SmtpClient::ConnectionType connection_type,
                              bool identify, std::string username,
                              std::string password, QWidget* parent = nullptr)
      : QThread(parent),
        host_(std::move(host)),
        port_(port),
        connection_type_(connection_type),
        identify_(identify),
        username_(std::move(username)),
        password_(std::move(password)) {}

  void SetSender(const QString& sender);

  /**
   * @brief Set the Recipient object
   *
   * @param recipients
   */
  void SetRecipient(const QString& recipients);

  /**
   * @brief
   *
   * @param ccs
   */
  void SetCC(const QString& ccs);

  /**
   * @brief
   *
   * @param bccs
   */
  void SetBCC(const QString& bccs);

  /**
   * @brief Set the Subject object
   *
   * @param subject
   */
  void SetSubject(const QString& subject) { message.setSubject(subject); }

  /**
   * @brief
   *
   * @param content
   */
  void AddTextContent(const QString& content);

  /**
   * @brief
   *
   * @param file_name
   * @param content
   */
  void AddFileContent(const QString& file_name, const QByteArray& content);

  /**
   * @brief Set the Encrypt Content object
   *
   * @param encrypt_content
   * @param public_key_ids
   */
  void SetEncryptContent(bool encrypt_content,
                         GpgFrontend::KeyIdArgsListPtr public_key_ids);

  /**
   * @brief Set the Attach Signature File object
   *
   * @param attach_signature_file
   * @param private_key_id
   */
  void SetAttachSignatureFile(bool attach_signature_file,
                              GpgFrontend::KeyId private_key_id);

  /**
   * @brief Set the Attach Public Key object
   *
   * @param attach_public_key_file
   * @param attached_public_key_ids
   */
  void SetAttachPublicKey(bool attach_public_key_file,
                          GpgFrontend::KeyId attached_public_key_ids);

 signals:
  /**
   * @brief
   *
   * @param result
   */
  void SignalSMTPResult(const QString& result);

 protected:
  /**
   * @brief
   *
   */
  void run() override;

 private:
  // SMTP Options

  std::string host_;                            ///<
  int port_;                                    ///<
  SmtpClient::ConnectionType connection_type_;  ///<

  bool identify_;         ///<
  std::string username_;  ///<
  std::string password_;  ///<

  MimeMessage message;                                 ///<
  std::vector<std::unique_ptr<MimeText>> texts_;       ///<
  std::vector<std::unique_ptr<MimeText>> send_texts_;  ///<
  std::vector<std::unique_ptr<MimeFile>> files_;       ///<

  // GPG Options

  bool encrypt_content_ = false;                  ///<
  GpgFrontend::KeyIdArgsListPtr public_key_ids_;  ///<
  bool attach_signature_file_ = false;            ///<
  GpgFrontend::KeyId private_key_id_;             ///<
  bool attach_public_key_file_ = false;           ///<
  GpgFrontend::KeyId attached_public_key_ids_;    ///<
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SMTPSENDMAILTHREAD_H
