/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_SMTPSENDMAILTHREAD_H
#define GPGFRONTEND_SMTPSENDMAILTHREAD_H

#include <utility>

#ifdef SMTP_SUPPORT
#include "smtp/SmtpMime"
#endif

#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {
class SMTPSendMailThread : public QThread {
  Q_OBJECT
 public:
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

  void setSender(const QString& sender);

  void setRecipient(const QString& recipients);

  void setCC(const QString& ccs);

  void setBCC(const QString& bccs);

  void setSubject(const QString& subject) { message.setSubject(subject); }

  void addTextContent(const QString& content);

  void addFileContent(const QString& file_name, const QByteArray& content);

  void setEncryptContent(bool encrypt_content,
                         GpgFrontend::KeyIdArgsListPtr public_key_ids);

  void setAttachSignatureFile(bool attach_signature_file,
                              GpgFrontend::KeyId private_key_id);

  void setAttachPublicKey(bool attach_public_key_file,
                          GpgFrontend::KeyId attached_public_key_ids);

 signals:
  void signalSMTPResult(const QString& result);

 protected:
  void run() override;

 private:
  // SMTP Options
  std::string host_;
  int port_;
  SmtpClient::ConnectionType connection_type_;

  bool identify_;
  std::string username_;
  std::string password_;

  MimeMessage message;
  std::vector<std::unique_ptr<MimeText>> texts_;
  std::vector<std::unique_ptr<MimeText>> send_texts_;
  std::vector<std::unique_ptr<MimeFile>> files_;

  // GPG Options
  bool encrypt_content_ = false;
  GpgFrontend::KeyIdArgsListPtr public_key_ids_;
  bool attach_signature_file_ = false;
  GpgFrontend::KeyId private_key_id_;
  bool attach_public_key_file_ = false;
  GpgFrontend::KeyId attached_public_key_ids_;
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SMTPSENDMAILTHREAD_H
