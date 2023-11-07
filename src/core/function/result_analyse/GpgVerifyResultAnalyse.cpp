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

#include "GpgVerifyResultAnalyse.h"

#include <boost/format.hpp>

#include "GpgFrontend.h"
#include "core/GpgConstants.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/CommonUtils.h"

GpgFrontend::GpgVerifyResultAnalyse::GpgVerifyResultAnalyse(
    GpgError error, GpgVerifyResult result)
    : error_(error), result_(std::move(result)) {}

void GpgFrontend::GpgVerifyResultAnalyse::doAnalyse() {
  SPDLOG_DEBUG("started");

  stream_ << "[#] " << _("Verify Operation") << " ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << "[" << _("Success") << "]" << std::endl;
  } else {
    stream_ << "[" << _("Failed") << "] " << gpgme_strerror(error_)
            << std::endl;
    setStatus(-1);
  }

  if (result_ != nullptr && result_->signatures != nullptr) {
    stream_ << "------------>" << std::endl;
    auto *sign = result_->signatures;

    stream_ << "[>] " << _("Signed On") << "(" << _("UTC") << ")"
            << " "
            << boost::posix_time::to_iso_extended_string(
                   boost::posix_time::from_time_t(sign->timestamp))
            << std::endl;

    stream_ << std::endl << "[>] " << _("Signatures List") << ":" << std::endl;

    bool can_continue = true;

    int count = 1;
    while ((sign != nullptr) && can_continue) {
      stream_ << boost::format(_("Signature [%1%]:")) % count++ << std::endl;
      switch (gpg_err_code(sign->status)) {
        case GPG_ERR_BAD_SIGNATURE:
          stream_ << _("A Bad Signature.") << std::endl;
          print_signer(stream_, sign);
          stream_ << _("This Signature is invalid.") << std::endl;
          can_continue = false;
          setStatus(-1);
          break;
        case GPG_ERR_NO_ERROR:
          stream_ << _("A") << " ";
          if ((sign->summary & GPGME_SIGSUM_GREEN) != 0) {
            stream_ << _("Good") << " ";
          }
          if ((sign->summary & GPGME_SIGSUM_RED) != 0) {
            stream_ << _("Bad") << " ";
          }
          if ((sign->summary & GPGME_SIGSUM_SIG_EXPIRED) != 0) {
            stream_ << _("Expired") << " ";
          }
          if ((sign->summary & GPGME_SIGSUM_KEY_MISSING) != 0) {
            stream_ << _("Missing Key's") << " ";
          }
          if ((sign->summary & GPGME_SIGSUM_KEY_REVOKED) != 0) {
            stream_ << _("Revoked Key's") << " ";
          }
          if ((sign->summary & GPGME_SIGSUM_KEY_EXPIRED) != 0) {
            stream_ << _("Expired Key's") << " ";
          }
          if ((sign->summary & GPGME_SIGSUM_CRL_MISSING) != 0) {
            stream_ << _("Missing CRL's") << " ";
          }

          if ((sign->summary & GPGME_SIGSUM_VALID) != 0) {
            stream_ << _("Signature Fully Valid.") << std::endl;
          } else {
            stream_ << _("Signature Not Fully Valid.") << std::endl;
            stream_ << _("(May used a subkey to sign)") << std::endl;
          }

          if ((sign->status & GPGME_SIGSUM_KEY_MISSING) == 0U) {
            if (!print_signer(stream_, sign)) setStatus(0);
          } else {
            stream_ << _("Key is NOT present with ID 0x") << sign->fpr
                    << std::endl;
          }

          setStatus(1);

          break;
        case GPG_ERR_NO_PUBKEY:
          stream_ << _("A signature could NOT be verified due to a Missing Key")
                  << std::endl;
          setStatus(-2);
          break;
        case GPG_ERR_CERT_REVOKED:
          stream_ << _("A signature is valid but the key used to verify the "
                       "signature has been revoked")
                  << std::endl;
          if (!print_signer(stream_, sign)) {
            setStatus(0);
          }
          setStatus(-1);
          break;
        case GPG_ERR_SIG_EXPIRED:
          stream_ << _("A signature is valid but expired") << std::endl;
          if (!print_signer(stream_, sign)) {
            setStatus(0);
          }
          setStatus(-1);
          break;
        case GPG_ERR_KEY_EXPIRED:
          stream_ << _("A signature is valid but the key used to "
                       "verify the signature has expired.")
                  << std::endl;
          if (!print_signer(stream_, sign)) {
            setStatus(0);
          }
          break;
        case GPG_ERR_GENERAL:
          stream_ << _("There was some other error which prevented "
                       "the signature verification.")
                  << std::endl;
          status_ = -1;
          can_continue = false;
          break;
        default:
          auto fpr = std::string(sign->fpr);
          stream_ << _("Error for key with fingerprint") << " "
                  << GpgFrontend::BeautifyFingerprint(fpr);
          setStatus(-1);
      }
      stream_ << std::endl;
      sign = sign->next;
    }
    stream_ << "<------------" << std::endl;
  } else {
    stream_
        << "[>] "
        << _("Could not find information that can be used for verification.")
        << std::endl;
    setStatus(0);
    return;
  }
}

auto GpgFrontend::GpgVerifyResultAnalyse::print_signer(
    std::stringstream &stream, gpgme_signature_t sign) -> bool {
  bool key_found = true;
  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(sign->fpr);

  if (!key.IsGood()) {
    stream << "    " << _("Signed By") << ": "
           << "<" << _("Unknown") << ">" << std::endl;
    setStatus(0);
    key_found = false;
  } else {
    stream << "    " << _("Signed By") << ": "
           << key.GetUIDs()->front().GetUID() << std::endl;
  }
  if (sign->pubkey_algo != 0U) {
    stream << "    " << _("Public Key Algo") << ": "
           << gpgme_pubkey_algo_name(sign->pubkey_algo) << std::endl;
  }
  if (sign->hash_algo != 0U) {
    stream << "    " << _("Hash Algo") << ": "
           << gpgme_hash_algo_name(sign->hash_algo) << std::endl;
  }
  if (sign->timestamp != 0U) {
    stream << "    " << _("Date") << "(" << _("UTC") << ")"
           << ": "
           << boost::posix_time::to_iso_extended_string(
                  boost::posix_time::from_time_t(sign->timestamp))
           << std::endl;
  }
  stream << std::endl;
  return key_found;
}

auto GpgFrontend::GpgVerifyResultAnalyse::GetSignatures() const
    -> gpgme_signature_t {
  if (result_) {
    return result_->signatures;
  }
  return nullptr;
}

auto GpgFrontend::GpgVerifyResultAnalyse::TakeChargeOfResult()
    -> GpgFrontend::GpgVerifyResult {
  return std::move(result_);
}
