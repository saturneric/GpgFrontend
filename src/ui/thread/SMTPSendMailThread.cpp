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

#include "SMTPSendMailThread.h"

#include <boost/format.hpp>

#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"

namespace GpgFrontend::UI {

void SMTPSendMailThread::run() {
  SmtpClient smtp(host_.c_str(), port_, connection_type_);

  if (identify_) {
    smtp.setUser(username_.c_str());
    smtp.setPassword(password_.c_str());
  }

  if (encrypt_content_ && public_key_ids_ != nullptr &&
      !public_key_ids_->empty() && !attach_signature_file_) {
    message.getContent().setContentType(
        "multipart/encrypted; "
        "protocol=\"application/pgp-encrypted\"");
  }

  if (attach_signature_file_ && !private_key_id_.empty()) {
    message.getContent().setContentType(
        "multipart/signed; "
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
      auto err = GpgBasicOperator::GetInstance().Encrypt(
          std::move(keys), *in_buffer, out_buffer, result);

      if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
        emit SignalSMTPResult("Fail to encrypt with gpg keys");
        return;
      }
      text->setText(out_buffer->c_str());

      //   The multipart/encrypted MIME body MUST consist of exactly two body
      //   parts, the first with content type "application/pgp-encrypted".  This
      //   body contains the control information.  A message complying with this
      //   standard MUST contain a "Version: 1" field in this body.  Since the
      //   OpenPGP packet format contains all other information necessary for
      //   decrypting, no other information is required here.
      auto control_text = std::make_unique<MimeText>("Version: 1\r\n");
      control_text->setContentType("application/pgp-encrypted");
      send_texts_.push_back(std::move(control_text));
      //   The second MIME body part MUST contain the actual encrypted data.  It
      //   MUST be labeled with a content type of "application/octet-stream".
      text->setContentType("application/octet-stream");
    }

    send_texts_.push_back(std::move(text));

    // sign
    if (attach_signature_file_ && !private_key_id_.empty()) {
      ByteArrayPtr out_buffer = nullptr;
      GpgSignResult result;

      auto& plain_mime_text = send_texts_.back();
      //   In particular, line endings in the encoded data
      //   MUST use the canonical <CR><LF> sequence where appropriate
      auto encoded_text = plain_mime_text->getText();
      //   As described in section 3 of this document, any trailing
      //   whitespace MUST then be removed from the signed material.
      encoded_text = encoded_text.trimmed();
      encoded_text = encoded_text.replace('\n', "\r\n");
      //   An implementation which elects to adhere to the OpenPGP convention
      //   has to make sure it inserts a <CR><LF> pair on the last line of the
      //   data to be signed and transmitted.
      encoded_text.append("\r\n");
      plain_mime_text->setText(encoded_text);

      //    This presents serious problems
      //    for multipart/signed, in particular, where the signature is
      //    invalidated when such an operation occurs.  For this reason all data
      //    signed according to this protocol MUST be constrained to 7 bits (8-
      //    bit data MUST be encoded using either Quoted-Printable or Base64).
      plain_mime_text->setEncoding(MimePart::_7Bit);

      // As described in [2], the digital signature MUST be calculated
      // over both the data to be signed and its set of content headers.
      auto text_calculated = plain_mime_text->toString().toStdString();

      auto in_buffer = std::make_unique<ByteArray>(text_calculated);
      auto key = GpgKeyGetter::GetInstance().GetKey(private_key_id_);
      auto keys = std::make_unique<KeyArgsList>();
      keys->push_back(std::move(key));

      //   The signature MUST be generated detached from the signed data
      //   so that the process does not alter the signed data in any way.
      auto err = GpgBasicOperator::GetInstance().Sign(
          std::move(keys), *in_buffer, out_buffer, GPGME_SIG_MODE_DETACH,
          result);

      if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) {
        emit SignalSMTPResult("Fail to sign with gpg keys");
        return;
      }

      auto sign_content_name =
          boost::format("%1%_sign_%2%.asc") % private_key_id_ % index++;

      // Set MIME Options
      send_texts_.push_back(std::make_unique<MimeText>(out_buffer->c_str()));
      auto& sig_text = send_texts_.back();
      sig_text->setContentType("application/pgp-signature");
      sig_text->setEncoding(MimePart::_7Bit);
      sig_text->setContentName(sign_content_name.str().c_str());

      // set Message Integrity Check (MIC) algorithm
      if (result->signatures != nullptr) {
        //   The "micalg" parameter for the "application/pgp-signature"
        //   protocol
        //   MUST contain exactly one hash-symbol of the format "pgp-<hash-
        //   identifier>", where <hash-identifier> identifies the Message
        //   Integrity Check (MIC) algorithm used to generate the signature.
        //   Hash-symbols are constructed from the text names registered in [1]
        //   or according to the mechanism defined in that document by
        //   converting the text name to lower case and prefixing it with the
        //   four characters "pgp-".
        auto hash_algo_name =
            std::string(gpgme_hash_algo_name(result->signatures->hash_algo));
        boost::algorithm::to_lower(hash_algo_name);
        std::stringstream ss;
        ss << message.getContent().getContentType().toStdString();
        ss << "; micalg=pgp-" << hash_algo_name;
        message.getContent().setContentType(ss.str().c_str());
      }
    }
  }

  if (attach_public_key_file_ && !attached_public_key_ids_.empty()) {
    auto key = GpgKeyGetter::GetInstance().GetKey(attached_public_key_ids_);
    ByteArrayPtr out_buffer = nullptr;
    GpgKeyImportExporter::GetInstance().ExportKey(key, out_buffer);

    auto public_key_file_name =
        boost::format("%1%_pubkey.asc") % attached_public_key_ids_;
    AddFileContent(public_key_file_name.str().c_str(), out_buffer->c_str());
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
    emit SignalSMTPResult("Fail to connect SMTP server");
    return;
  }
  if (!smtp.login()) {
    emit SignalSMTPResult("Fail to login");
    return;
  }
  if (!smtp.sendMail(message)) {
    emit SignalSMTPResult("Fail to send mail");
    return;
  }
  smtp.quit();
  emit SignalSMTPResult("Succeed in sending a test email");
}

void SMTPSendMailThread::SetBCC(const QString& bccs) {
  QStringList bcc_string_list = bccs.split(';');
  for (const auto& bcc : bcc_string_list) {
    if (!bcc.isEmpty()) message.addBcc(new EmailAddress(bcc.trimmed()));
  }
}

void SMTPSendMailThread::SetCC(const QString& ccs) {
  QStringList cc_string_list = ccs.split(';');
  for (const auto& cc : cc_string_list) {
    if (!cc.isEmpty()) message.addCc(new EmailAddress(cc.trimmed()));
  }
}

void SMTPSendMailThread::SetRecipient(const QString& recipients) {
  QStringList rcpt_string_list = recipients.split(';');
  for (const auto& rcpt : rcpt_string_list) {
    if (!rcpt.isEmpty()) message.addRecipient(new EmailAddress(rcpt.trimmed()));
  }
}

void SMTPSendMailThread::SetSender(const QString& sender) {
  message.setSender(new EmailAddress(sender));
}

void SMTPSendMailThread::AddTextContent(const QString& content) {
  auto text = std::make_unique<MimeText>(content.trimmed());
  texts_.push_back(std::move(text));
}

void SMTPSendMailThread::AddFileContent(const QString& file_name,
                                        const QByteArray& content) {
  auto file = std::make_unique<MimeFile>(content, file_name);
  files_.push_back(std::move(file));
}

void SMTPSendMailThread::SetEncryptContent(
    bool encrypt_content, GpgFrontend::KeyIdArgsListPtr public_key_ids) {
  this->encrypt_content_ = encrypt_content;
  this->public_key_ids_ = std::move(public_key_ids);
}

void SMTPSendMailThread::SetAttachSignatureFile(
    bool attach_signature_file, GpgFrontend::KeyId private_key_id) {
  this->attach_signature_file_ = attach_signature_file;
  this->private_key_id_ = std::move(private_key_id);
}

void SMTPSendMailThread::SetAttachPublicKey(
    bool attach_public_key_file, GpgFrontend::KeyId attached_public_key_ids) {
  this->attach_public_key_file_ = attach_public_key_file;
  this->attached_public_key_ids_ = std::move(attached_public_key_ids);
}
}  // namespace GpgFrontend::UI
