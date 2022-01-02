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

#include "SMTPSendMailThread.h"

#include <boost/format.hpp>

#include "gpg/function/BasicOperator.h"
#include "gpg/function/GpgKeyGetter.h"
#include "gpg/function/GpgKeyImportExporter.h"

namespace GpgFrontend::UI {

void SMTPSendMailThread::run() {
  SmtpClient smtp(host_.c_str(), port_, connection_type_);

  if (identify_) {
    smtp.setUser(username_.c_str());
    smtp.setPassword(password_.c_str());
  }

  if (encrypt_content_ && public_key_ids_ != nullptr &&
      !public_key_ids_->empty()) {
    message.getContent().setContentType(
        "multipart/encrypted; micalg=pgp-md5; "
        "protocol=\"application/pgp-encrypted\"");
  }

  if (attach_signature_file_ && !private_key_id_.empty()) {
    message.getContent().setContentType(
        "multipart/signed; micalg=pgp-md5; "
        "protocol=\"application/pgp-signature\"");
  }

  int index = 0;
  for (auto& text : texts_) {
    const auto plain_text = text->getText().toStdString();
    // encrypt
    if (encrypt_content_ && public_key_ids_ != nullptr &&
        !public_key_ids_->empty()) {
      ByteArrayPtr out_buffer = nullptr;
      GpgEncrResult result;
      auto in_buffer = std::make_unique<ByteArray>(plain_text);
      auto keys = GpgKeyGetter::GetInstance().GetKeys(public_key_ids_);
      auto err = BasicOperator::GetInstance().Encrypt(
          std::move(keys), *in_buffer, out_buffer, result);

      if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
        emit signalSMTPResult("Fail to encrypt with gpg keys");
        return;
      }
      text->setText(out_buffer->c_str());
    }

    send_texts_.push_back(std::move(text));

    // sign
    if (attach_signature_file_ && !private_key_id_.empty()) {
      ByteArrayPtr out_buffer = nullptr;
      GpgSignResult result;

      auto& plain_mime_text = send_texts_.back();
      auto in_buffer =
          std::make_unique<ByteArray>(plain_mime_text->getText().toStdString());
      auto key = GpgKeyGetter::GetInstance().GetKey(private_key_id_);
      auto keys = std::make_unique<KeyArgsList>();
      keys->push_back(std::move(key));

      auto err = BasicOperator::GetInstance().Sign(
          std::move(keys), *in_buffer, out_buffer, GPGME_SIG_MODE_DETACH,
          result);

      if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
        emit signalSMTPResult("Fail to sign with gpg keys");
        return;
      }

      auto sign_content_name =
          boost::format("%1%_sign_%2%.asc") % private_key_id_ % index++;

      // Add MIME
      send_texts_.push_back(std::make_unique<MimeText>(out_buffer->c_str()));
      auto& sig_text = send_texts_.back();
      sig_text->setContentType("application/pgp-signature");
      sig_text->setEncoding(MimePart::_7Bit);
      sig_text->setContentName(sign_content_name.str().c_str());
    }
  }

  if (attach_public_key_file_ && !attached_public_key_ids_.empty()) {
    auto key = GpgKeyGetter::GetInstance().GetKey(attached_public_key_ids_);
    ByteArrayPtr out_buffer = nullptr;
    GpgKeyImportExporter::GetInstance().ExportKey(key, out_buffer);

    auto public_key_file_name =
        boost::format("%1%_pubkey.asc") % attached_public_key_ids_;
    addFileContent(public_key_file_name.str().c_str(), out_buffer->c_str());
    auto& key_file = files_.back();
    key_file->setEncoding(MimePart::_7Bit);
    key_file->setContentType("application/pgp-keys");
  }

  for (const auto& text : send_texts_) {
    message.addPart(text.get());
  }

  for (const auto& file : files_) {
    message.addPart(file.get());
  }

  // Now we can send the mail
  if (!smtp.connectToHost()) {
    emit signalSMTPResult("Fail to connect SMTP server");
    return;
  }
  if (!smtp.login()) {
    emit signalSMTPResult("Fail to login");
    return;
  }
  if (!smtp.sendMail(message)) {
    emit signalSMTPResult("Fail to send mail");
    return;
  }
  smtp.quit();
  emit signalSMTPResult("Succeed in sending a test email");
}

void SMTPSendMailThread::setBCC(const QString& bccs) {
  QStringList bcc_string_list = bccs.split(';');
  for (const auto& bcc : bcc_string_list) {
    if (!bcc.isEmpty()) message.addBcc(new EmailAddress(bcc.trimmed()));
  }
}

void SMTPSendMailThread::setCC(const QString& ccs) {
  QStringList cc_string_list = ccs.split(';');
  for (const auto& cc : cc_string_list) {
    if (!cc.isEmpty()) message.addCc(new EmailAddress(cc.trimmed()));
  }
}

void SMTPSendMailThread::setRecipient(const QString& recipients) {
  QStringList rcpt_string_list = recipients.split(';');
  for (const auto& rcpt : rcpt_string_list) {
    if (!rcpt.isEmpty()) message.addRecipient(new EmailAddress(rcpt.trimmed()));
  }
}

void SMTPSendMailThread::setSender(const QString& sender) {
  message.setSender(new EmailAddress(sender));
}

void SMTPSendMailThread::addTextContent(const QString& content) {
  auto text = std::make_unique<MimeText>(content);
  texts_.push_back(std::move(text));
}

void SMTPSendMailThread::addFileContent(const QString& file_name,
                                        const QByteArray& content) {
  auto file = std::make_unique<MimeFile>(content, file_name);
  files_.push_back(std::move(file));
}

void SMTPSendMailThread::setEncryptContent(
    bool encrypt_content, GpgFrontend::KeyIdArgsListPtr public_key_ids) {
  this->encrypt_content_ = encrypt_content;
  this->public_key_ids_ = std::move(public_key_ids);
}

void SMTPSendMailThread::setAttachSignatureFile(
    bool attach_signature_file, GpgFrontend::KeyId private_key_id) {
  this->attach_signature_file_ = attach_signature_file;
  this->private_key_id_ = std::move(private_key_id);
}

void SMTPSendMailThread::setAttachPublicKey(
    bool attach_public_key_file, GpgFrontend::KeyId attached_public_key_ids) {
  this->attach_public_key_file_ = attach_public_key_file;
  this->attached_public_key_ids_ = std::move(attached_public_key_ids);
}
}  // namespace GpgFrontend::UI
