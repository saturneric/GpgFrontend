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
    GpgError m_error, GpgDecrResult m_result)
    : error_(m_error), result_(std::move(m_result)) {}

void GpgFrontend::GpgDecryptResultAnalyse::doAnalyse() {
  stream_ << "[#] " << _("Decrypt Operation");

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "[" << _("Success") << "]" << std::endl;
  } else {
    stream_ << "[" << _("Failed") << "] " << gpgme_strerror(error_)
            << std::endl;
    setStatus(-1);
    if (result_ != nullptr && result_->unsupported_algorithm != nullptr) {
      stream_ << "------------>" << std::endl;
      stream_ << _("Unsupported Algo") << ": " << result_->unsupported_algorithm
              << std::endl;
    }
  }

  if (result_ != nullptr && result_->recipients != nullptr) {
    stream_ << "------------>" << std::endl;
    if (result_->file_name != nullptr) {
      stream_ << _("File Name") << ": " << result_->file_name << std::endl;
      stream_ << std::endl;
    }
    if (result_->is_mime) {
      stream_ << _("MIME") << ": " << _("true") << std::endl;
    }

    auto *recipient = result_->recipients;
    if (recipient != nullptr) stream_ << _("Recipient(s)") << ": " << std::endl;
    while (recipient != nullptr) {
      print_recipient(stream_, recipient);
      recipient = recipient->next;
    }
    stream_ << "<------------" << std::endl;
  }

  stream_ << std::endl;
}

void GpgFrontend::GpgDecryptResultAnalyse::print_recipient(
    std::stringstream &stream, gpgme_recipient_t recipient) {
  // check
  if (recipient->keyid == nullptr) return;

  stream << "  {>} " << _("Recipient") << ": ";
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(recipient->keyid);
  if (key.IsGood()) {
    stream << key.GetName().c_str();
    if (!key.GetEmail().empty()) {
      stream << "<" << key.GetEmail().c_str() << ">";
    }
  } else {
    stream << "<" << _("Unknown") << ">";
    setStatus(0);
  }

  stream << std::endl;

  stream << "         " << _("Key ID") << ": " << recipient->keyid << std::endl;
  stream << "         " << _("Public Key Algo") << ": "
         << gpgme_pubkey_algo_name(recipient->pubkey_algo) << std::endl;
}
