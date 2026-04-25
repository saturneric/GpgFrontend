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

#include "GpgDecryptResultAnalyse.h"

#include "core/function/openpgp/AbstractKeyRepository.h"

GpgFrontend::GpgDecryptResultAnalyse::GpgDecryptResultAnalyse(
    int channel, GpgError m_error, GpgDecryptResult m_result)
    : GpgResultAnalyse(channel), error_(m_error), result_(m_result) {}

void GpgFrontend::GpgDecryptResultAnalyse::doAnalyse() {
  auto recipients = this->result_.Recipients();

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "- " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << "- " << tr("Failed") << ": " << gpgme_strerror(error_)
            << Qt::endl;
    setStatus(-1);
    if (!result_.UnsupportedAlgorithm().isEmpty()) {
      stream_ << Qt::endl;
      stream_ << "## " << tr("Unsupported Algo") << ": "
              << result_.UnsupportedAlgorithm() << Qt::endl;
    }
  }

  stream_ << Qt::endl;

  if (!recipients.empty()) {
    stream_ << Qt::endl;

    stream_ << "## " << tr("General State") << ": " << Qt::endl;

    if (!result_.Filename().isEmpty()) {
      stream_ << "- " << tr("File Name") << ": " << result_.Filename()
              << Qt::endl;
    }
    stream_ << "- " << tr("MIME") << ": "
            << (result_.MIME() ? tr("true") : tr("false")) << Qt::endl;

    stream_ << "- " << tr("Message Integrity Protection") << ": "
            << (result_.MessageIntegrityProtected() ? tr("true") : tr("false"))
            << Qt::endl;
    if (!result_.MessageIntegrityProtected()) {
      setStatus(0);  /// < unsafe situation
    }

    if (!result_.SymmetricEncryptionAlgorithm().isEmpty()) {
      stream_ << "- " << tr("Symmetric Encryption Algorithm") << ": "
              << result_.SymmetricEncryptionAlgorithm() << Qt::endl;
    }

    stream_ << Qt::endl << Qt::endl;

    if (recipients.size() > 0) {
      stream_ << "## " << tr("Recipient(s)") << ": " << Qt::endl << Qt::endl;
    }

    int index = 0;
    for (const auto& recipient : recipients) {
      // check
      if (recipient.keyid == nullptr) return;
      stream_ << "### " << tr("Recipient") << " [" << ++index << "]: ";
      print_recipient(stream_, recipient);
      stream_ << Qt::endl
              << "---------------------------------------" << Qt::endl
              << Qt::endl;
    }

    stream_ << Qt::endl;
  }

  stream_ << Qt::endl;
}

void GpgFrontend::GpgDecryptResultAnalyse::print_recipient(
    QTextStream& stream, const GpgRecipient& recipient) {
  auto key =
      AbstractKeyRepository::GetInstance(GetChannel()).GetKey(recipient.keyid);

  if (key != nullptr) {
    stream << key->Name();
    if (!key->Comment().isEmpty()) stream << "(" << key->Comment() << ")";
    if (!key->Email().isEmpty()) stream << "<" << key->Email() << ">";
  } else {
    stream << "<" << tr("unknown") << ">";
    setStatus(0);
  }

  stream << Qt::endl;

  stream << "- " << tr("Key ID") << ": " << recipient.keyid;
  if (key != nullptr) {
    stream << " ("
           << (key->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY
                   ? tr("Subkey")
                   : tr("Primary Key"))
           << ")";
  }

  stream << Qt::endl;

  stream << "- " << tr("Public Key Algo") << ": " << recipient.pubkey_algo
         << Qt::endl;
  stream << "- " << tr("Status") << ": " << gpgme_strerror(recipient.status)
         << Qt::endl;
}
