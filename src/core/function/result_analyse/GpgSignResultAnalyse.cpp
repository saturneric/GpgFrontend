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

#include "GpgSignResultAnalyse.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"

namespace GpgFrontend {

GpgSignResultAnalyse::GpgSignResultAnalyse(GpgError error, GpgSignResult result)
    : error_(error), result_(std::move(result)) {}

void GpgSignResultAnalyse::doAnalyse() {
  SPDLOG_DEBUG("start sign result analyse");

  stream_ << "[#] " << _("Sign Operation") << " ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "[" << _("Success") << "]" << std::endl;
  } else {
    stream_ << "[" << _("Failed") << "] " << gpgme_strerror(error_)
            << std::endl;
    setStatus(-1);
  }

  if (result_ != nullptr &&
      (result_->signatures != nullptr || result_->invalid_signers != nullptr)) {
    SPDLOG_DEBUG("sign result analyse getting result");
    stream_ << "------------>" << std::endl;
    auto *new_sign = result_->signatures;

    while (new_sign != nullptr) {
      stream_ << "[>]" << _("New Signature") << ": " << std::endl;

      SPDLOG_DEBUG("signers fingerprint: ", new_sign->fpr);

      stream_ << "    " << _("Sign Mode") << ": ";
      if (new_sign->type == GPGME_SIG_MODE_NORMAL) {
        stream_ << _("Normal");
      } else if (new_sign->type == GPGME_SIG_MODE_CLEAR) {
        stream_ << _("Clear");
      } else if (new_sign->type == GPGME_SIG_MODE_DETACH) {
        stream_ << _("Detach");
      }

      stream_ << std::endl;

      auto singer_key = GpgKeyGetter::GetInstance().GetKey(new_sign->fpr);
      if (singer_key.IsGood()) {
        stream_ << "    " << _("Signer") << ": "
                << singer_key.GetUIDs()->front().GetUID() << std::endl;
      } else {
        stream_ << "    " << _("Signer") << ": "
                << "<unknown>" << std::endl;
      }
      stream_ << "    " << _("Public Key Algo") << ": "
              << gpgme_pubkey_algo_name(new_sign->pubkey_algo) << std::endl;
      stream_ << "    " << _("Hash Algo") << ": "
              << gpgme_hash_algo_name(new_sign->hash_algo) << std::endl;
      stream_ << "    " << _("Date") << "(" << _("UTC") << ")"
              << ": "
              << boost::posix_time::to_iso_extended_string(
                     boost::posix_time::from_time_t(new_sign->timestamp))
              << std::endl;

      stream_ << std::endl;

      new_sign = new_sign->next;
    }

    SPDLOG_DEBUG("sign result analyse getting invalid signer");

    auto *invalid_signer = result_->invalid_signers;

    if (invalid_signer != nullptr) {
      stream_ << _("Invalid Signers") << ": " << std::endl;
    }

    while (invalid_signer != nullptr) {
      setStatus(0);
      stream_ << "[>] " << _("Signer") << ": " << std::endl;
      stream_ << "      " << _("Fingerprint") << ": " << invalid_signer->fpr
              << std::endl;
      stream_ << "      " << _("Reason") << ": "
              << gpgme_strerror(invalid_signer->reason) << std::endl;
      stream_ << std::endl;

      invalid_signer = invalid_signer->next;
    }
    stream_ << "<------------" << std::endl;
  }
}

}  // namespace GpgFrontend