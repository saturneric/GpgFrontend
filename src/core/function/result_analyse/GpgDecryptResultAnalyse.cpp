/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"

GpgFrontend::GpgDecryptResultAnalyse::GpgDecryptResultAnalyse(
    GpgError m_error, GpgDecryptResult m_result)
    : error_(m_error), result_(m_result) {}

void GpgFrontend::GpgDecryptResultAnalyse::doAnalyse() {
  auto *result = result_.GetRaw();

  stream_ << "# " << tr("Decrypt Operation") << " ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "- " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << "- " << tr("Failed") << ": " << gpgme_strerror(error_)
            << Qt::endl;
    setStatus(-1);
    if (result != nullptr && result->unsupported_algorithm != nullptr) {
      stream_ << Qt::endl;
      stream_ << "## " << tr("Unsupported Algo") << ": "
              << result->unsupported_algorithm << Qt::endl;
    }
  }

  if (result != nullptr && result->recipients != nullptr) {
    stream_ << Qt::endl;

    stream_ << "## " << tr("General State") << ": " << Qt::endl;

    if (result->file_name != nullptr) {
      stream_ << "- " << tr("File Name") << ": " << result->file_name
              << Qt::endl;
    }
    stream_ << "- " << tr("MIME") << ": "
            << (result->is_mime == 0 ? tr("false") : tr("true")) << Qt::endl;

    stream_ << "- " << tr("Message Integrity Protection") << ": "
            << (result->legacy_cipher_nomdc == 0 ? tr("true") : tr("false"))
            << Qt::endl;
    if (result->legacy_cipher_nomdc == 1) setStatus(0);  /// < unsafe situation

    if (result->symkey_algo != nullptr) {
      stream_ << "- " << tr("Symmetric Encryption Algorithm") << ": "
              << result->symkey_algo << Qt::endl;
    }

    if (result->session_key != nullptr) {
      stream_ << "- " << tr("Session Key") << ": " << result->session_key
              << Qt::endl;
    }

    stream_ << "- " << tr("German Encryption Standards") << ": "
            << (result->is_de_vs == 0 ? tr("false") : tr("true")) << Qt::endl;

    stream_ << Qt::endl << Qt::endl;

    auto *recipient = result->recipients;
    auto index = 0;
    if (recipient != nullptr) {
      stream_ << "## " << tr("Recipient(s)") << ": " << Qt::endl << Qt::endl;
    }

    while (recipient != nullptr) {
      // check
      if (recipient->keyid == nullptr) return;
      stream_ << "### " << tr("Recipient") << " [" << ++index << "]: ";
      print_recipient(stream_, recipient);
      stream_ << Qt::endl
              << "---------------------------------------" << Qt::endl
              << Qt::endl;
      recipient = recipient->next;
    }

    stream_ << Qt::endl;
  }

  stream_ << Qt::endl;
}

void GpgFrontend::GpgDecryptResultAnalyse::print_recipient(
    QTextStream &stream, gpgme_recipient_t recipient) {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(recipient->keyid);
  if (key.IsGood()) {
    stream << key.GetName();
    if (!key.GetComment().isEmpty()) stream << "(" << key.GetComment() << ")";
    if (!key.GetEmail().isEmpty()) stream << "<" << key.GetEmail() << ">";
  } else {
    stream << "<" << tr("unknown") << ">";
    setStatus(0);
  }

  stream << Qt::endl;

  stream << "- " << tr("Key ID") << ": " << recipient->keyid << Qt::endl;
  stream << "- " << tr("Public Key Algo") << ": "
         << gpgme_pubkey_algo_name(recipient->pubkey_algo) << Qt::endl;
  stream << "- " << tr("Status") << ": " << gpgme_strerror(recipient->status)
         << Qt::endl;
}
