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

#include "GpgVerifyResultAnalyse.h"

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/LocalizedUtils.h"

GpgFrontend::GpgVerifyResultAnalyse::GpgVerifyResultAnalyse(
    int channel, GpgError error, GpgVerifyResult result)
    : GpgResultAnalyse(channel), error_(error), result_(result) {}

void GpgFrontend::GpgVerifyResultAnalyse::doAnalyse() {
  auto signatures = this->result_.GetSignature();

  op_info_.operation = tr("Verify");
  op_info_.engine = EngineInfo();

  stream_ << "# " << tr("Verify Operation") << " (" << EngineInfo() << ") ";

  if (gpgme_err_code(error_) == GPG_ERR_NO_ERROR) {
    stream_ << " - " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << " - " << tr("Failed") << ": " << gpgme_strerror(error_)
            << Qt::endl;
    if (!result_.ErrorDetail().isEmpty()) {
      stream_ << "    - " << tr("Detail") << ": " << result_.ErrorDetail()
              << Qt::endl;
    }
    setStatus(-1);
  }

  if (!signatures.empty()) {
    stream_ << Qt::endl;
    auto sign = signatures.front();

    stream_ << "-> " << tr("Signed On") << "(" << tr("UTC") << ")"
            << ": "
            << GetUTCDateByTimestamp(sign.GetCreateTime().toSecsSinceEpoch())
            << Qt::endl;

    stream_ << "-> " << tr("Signed On") << "(" << tr("Localized") << ")"
            << ": "
            << GetLocalizedDateByTimestamp(
                   sign.GetCreateTime().toSecsSinceEpoch())
            << Qt::endl;

    stream_ << Qt::endl << "## " << tr("Signatures List") << ":" << Qt::endl;
    stream_ << Qt::endl;

    bool can_continue = true;

    int count = 1;
    for (const auto& sign : signatures) {
      stream_ << "### " << tr("Signature [%1]:").arg(count++) << Qt::endl;
      stream_ << "- " << tr("Status") << ": ";
      switch (gpg_err_code(sign.GetStatus())) {
        case GPG_ERR_BAD_SIGNATURE:
          stream_ << tr("Invalid Signature") << Qt::endl;
          print_signer(stream_, GpgSignature(sign));
          stream_ << "  - " << tr("This signature could not be verified")
                  << Qt::endl;
          can_continue = false;
          setStatus(-1);
          break;
        case GPG_ERR_NO_ERROR: {
          // Determine overall status
          bool is_fully_valid = (sign.GetSummary() & GPGME_SIGSUM_VALID) != 0;
          bool is_green = (sign.GetSummary() & GPGME_SIGSUM_GREEN) != 0;
          bool is_red = (sign.GetSummary() & GPGME_SIGSUM_RED) != 0;

          if (is_fully_valid && is_green) {
            stream_ << tr("Fully Valid") << Qt::endl;
          } else if (is_red) {
            stream_ << tr("Valid but with Issues") << Qt::endl;
          } else {
            stream_ << tr("Valid but Not Fully Trusted") << Qt::endl;
          }

          // List specific warnings
          QStringList warnings;
          if ((sign.GetSummary() & GPGME_SIGSUM_SIG_EXPIRED) != 0) {
            warnings.append(tr("Signature has expired"));
          }
          if ((sign.GetSummary() & GPGME_SIGSUM_KEY_MISSING) != 0) {
            warnings.append(tr("Signing key is missing"));
          }
          if ((sign.GetSummary() & GPGME_SIGSUM_KEY_REVOKED) != 0) {
            warnings.append(tr("Signing key has been revoked"));
          }
          if ((sign.GetSummary() & GPGME_SIGSUM_KEY_EXPIRED) != 0) {
            warnings.append(tr("Signing key has expired"));
          }
          if ((sign.GetSummary() & GPGME_SIGSUM_CRL_MISSING) != 0) {
            warnings.append(tr("Certificate revocation list is missing"));
          }

          if (!warnings.isEmpty()) {
            stream_ << "  - " << tr("Warnings") << ":" << Qt::endl;
            for (const auto& warning : warnings) {
              stream_ << "    - " << warning << Qt::endl;
            }
          }

          if (!is_fully_valid && warnings.isEmpty()) {
            stream_ << "  - " << tr("Tips") << ": "
                    << tr("Adjust trust level to make it fully valid")
                    << Qt::endl;
          }

          if ((sign.GetStatus() & GPGME_SIGSUM_KEY_MISSING) == 0U) {
            if (!print_signer(stream_, GpgSignature(sign))) setStatus(0);
          } else {
            stream_ << "  - " << tr("Key ID") << ": 0x" << sign.GetFingerprint()
                    << " (" << tr("not present in keyring") << ")" << Qt::endl;
          }

          setStatus(1);
          break;
        }
        case GPG_ERR_NO_PUBKEY:
          stream_ << tr("Cannot Verify due to Key Missing") << Qt::endl;
          stream_ << "  - "
                  << tr("The public key needed to verify this signature is not "
                        "available")
                  << Qt::endl;
          setStatus(-2);
          print_signer_without_key(stream_, GpgSignature(sign));
          unknown_signer_fpr_list_.push_back(
              GpgSignature(sign).GetFingerprint());
          break;
        case GPG_ERR_CERT_REVOKED:
          stream_ << tr("Key Revoked") << Qt::endl;
          stream_ << "  - "
                  << tr("The signature is mathematically valid but the signing "
                        "key has been revoked")
                  << Qt::endl;
          if (!print_signer(stream_, GpgSignature(sign))) {
            setStatus(0);
          }
          setStatus(-1);
          break;
        case GPG_ERR_SIG_EXPIRED:
          stream_ << tr("Signature Expired") << Qt::endl;
          stream_ << "  - " << tr("The signature is valid but has expired")
                  << Qt::endl;
          if (!print_signer(stream_, GpgSignature(sign))) {
            setStatus(0);
          }
          setStatus(-1);
          break;
        case GPG_ERR_KEY_EXPIRED:
          stream_ << tr("Signing Key Expired") << Qt::endl;
          stream_
              << "  - "
              << tr("The signature is valid but the signing key has expired")
              << Qt::endl;
          if (!print_signer(stream_, GpgSignature(sign))) {
            setStatus(0);
          }
          break;
        case GPG_ERR_GENERAL:
          stream_ << tr("Verification Error") << Qt::endl;
          stream_ << "  - "
                  << tr("An error occurred during signature verification")
                  << Qt::endl;
          status_ = -1;
          can_continue = false;
          break;
        default:
          auto fpr = QString(sign.GetFingerprint());
          stream_ << tr("Unknown Error") << Qt::endl;
          stream_ << "  - " << tr("Key Fingerprint") << ": "
                  << GpgFrontend::BeautifyFingerprint(fpr) << Qt::endl;
          setStatus(-1);
      }
      stream_ << Qt::endl;

      if (!can_continue) {
        stream_ << tr("Verification process stopped due to errors.")
                << Qt::endl;
        break;
      }
    }
    stream_ << Qt::endl;
  } else {
    stream_
        << "-> "
        << tr("Could not find information that can be used for verification.")
        << Qt::endl;
    setStatus(0);
    op_info_.description =
        tr("No verifiable signatures were found in the data.");
    return;
  }

  for (const auto& sign : signatures) {
    GpgVerifySigInfo sig_info;

    sig_info.signer.fingerprint = sign.GetFingerprint();
    sig_info.signer.pubkeyAlgo = sign.GetPubkeyAlgo();
    sig_info.signer.hashAlgo = sign.GetHashAlgo();
    sig_info.signer.signTime = sign.GetCreateTime().toUTC();

    auto key = AbstractKeyRepository::GetInstance(GetChannel())
                   .GetKey(sign.GetFingerprint());
    if (key != nullptr) {
      sig_info.signer.uid = key->UID();
      sig_info.signer.keyId = key->ID();
      op_info_.details << key->UID();
    } else if (!sign.GetFingerprint().isEmpty()) {
      op_info_.details << sign.GetFingerprint();
    }

    switch (gpg_err_code(sign.GetStatus())) {
      case GPG_ERR_BAD_SIGNATURE:
        sig_info.validity = GpgSigValidity::kINVALID;
        break;
      case GPG_ERR_NO_ERROR: {
        bool is_fully_valid = (sign.GetSummary() & GPGME_SIGSUM_VALID) != 0;
        bool is_green = (sign.GetSummary() & GPGME_SIGSUM_GREEN) != 0;
        bool is_red = (sign.GetSummary() & GPGME_SIGSUM_RED) != 0;
        if (is_fully_valid && is_green) {
          sig_info.validity = GpgSigValidity::kFULLY_VALID;
        } else if (is_red) {
          sig_info.validity = GpgSigValidity::kVALID_WITH_ISSUES;
        } else {
          sig_info.validity = GpgSigValidity::kVALID_NOT_FULLY_TRUSTED;
        }
        if ((sign.GetSummary() & GPGME_SIGSUM_SIG_EXPIRED) != 0)
          sig_info.warnings << tr("Signature has expired");
        if ((sign.GetSummary() & GPGME_SIGSUM_KEY_MISSING) != 0)
          sig_info.warnings << tr("Signing key is missing");
        if ((sign.GetSummary() & GPGME_SIGSUM_KEY_REVOKED) != 0)
          sig_info.warnings << tr("Signing key has been revoked");
        if ((sign.GetSummary() & GPGME_SIGSUM_KEY_EXPIRED) != 0)
          sig_info.warnings << tr("Signing key has expired");
        if ((sign.GetSummary() & GPGME_SIGSUM_CRL_MISSING) != 0)
          sig_info.warnings << tr("Certificate revocation list is missing");
        break;
      }
      case GPG_ERR_NO_PUBKEY:
        sig_info.validity = GpgSigValidity::kKEY_MISSING;
        break;
      case GPG_ERR_CERT_REVOKED:
        sig_info.validity = GpgSigValidity::kKEY_REVOKED;
        break;
      case GPG_ERR_SIG_EXPIRED:
        sig_info.validity = GpgSigValidity::kSIG_EXPIRED;
        break;
      case GPG_ERR_KEY_EXPIRED:
        sig_info.validity = GpgSigValidity::kKEY_EXPIRED;
        break;
      case GPG_ERR_GENERAL:
        sig_info.validity = GpgSigValidity::kERROR;
        break;
      default:
        sig_info.validity = GpgSigValidity::kUNKNOWN;
    }

    op_info_.signatures << sig_info;
  }

  const auto& sigs = op_info_.signatures;

  auto any_validity = [&](GpgSigValidity v) -> bool {
    return std::any_of(
        sigs.begin(), sigs.end(),
        [v](const GpgVerifySigInfo& s) -> bool { return s.validity == v; });
  };
  auto all_validity = [&](GpgSigValidity v) -> bool {
    return !sigs.isEmpty() &&
           std::all_of(sigs.begin(), sigs.end(),
                       [v](const GpgVerifySigInfo& s) -> bool {
                         return s.validity == v;
                       });
  };

  if (status_ > 0) {
    if (all_validity(GpgSigValidity::kFULLY_VALID)) {
      const QString uid =
          sigs.size() == 1 ? sigs.first().signer.uid : QString();
      op_info_.description =
          uid.isEmpty()
              ? tr("The signature is fully valid and trusted. The data has "
                   "not been tampered with.")
              : tr("The signature by %1 is fully valid and trusted. The data "
                   "has not been tampered with.")
                    .arg(uid);
    } else {
      op_info_.description =
          tr("The signature is valid, but the signing key is not fully "
             "trusted. You may need to set a higher trust level.");
    }
  } else if (status_ == 0) {
    if (any_validity(GpgSigValidity::kKEY_MISSING)) {
      op_info_.description =
          tr("Verification incomplete. The signing key is not in your "
             "keyring. Please import the signer's public key.");
    } else if (any_validity(GpgSigValidity::kVALID_NOT_FULLY_TRUSTED)) {
      op_info_.description =
          tr("The signature is valid but the signing key is not fully "
             "trusted. Consider setting a higher trust level for that key.");
    } else {
      op_info_.description =
          tr("Verification completed with warnings. Please review the "
             "details.");
    }
  } else {
    if (any_validity(GpgSigValidity::kINVALID)) {
      op_info_.description =
          tr("Invalid signature detected. The data may have been tampered "
             "with or the signature is corrupt.");
    } else if (any_validity(GpgSigValidity::kKEY_REVOKED)) {
      op_info_.description =
          tr("Signature verification failed. The signing key has been "
             "revoked.");
    } else if (any_validity(GpgSigValidity::kSIG_EXPIRED)) {
      op_info_.description =
          tr("Signature verification failed. This signature has expired.");
    } else if (any_validity(GpgSigValidity::kKEY_EXPIRED)) {
      op_info_.description =
          tr("Signature verification failed. The signing key has expired.");
    } else if (any_validity(GpgSigValidity::kKEY_MISSING)) {
      op_info_.description =
          tr("Cannot verify. The signing key is not available in your "
             "keyring.");
    } else {
      op_info_.description =
          tr("Signature verification failed. Please review the details.");
    }
  }
}

auto GpgFrontend::GpgVerifyResultAnalyse::print_signer_without_key(
    QTextStream& stream, GpgSignature sign) -> bool {
  stream_ << "- " << tr("Signed By") << "(" << tr("Fingerprint") << ")"
          << ": "
          << (sign.GetFingerprint().isEmpty() ? tr("<unknown>")
                                              : sign.GetFingerprint())
          << Qt::endl;
  stream << "- " << tr("Public Key Algo") << ": " << sign.GetPubkeyAlgo()
         << Qt::endl;
  stream << "- " << tr("Hash Algo") << ": " << sign.GetHashAlgo() << Qt::endl;
  stream << "- " << tr("Sign Date") << "(" << tr("UTC") << ")"
         << ": " << QLocale().toString(sign.GetCreateTime().toUTC())
         << Qt::endl;
  stream << "- " << tr("Sign Date") << "(" << tr("Localized") << ")"
         << ": " << QLocale().toString(sign.GetCreateTime()) << Qt::endl;
  return true;
}

auto GpgFrontend::GpgVerifyResultAnalyse::print_signer(QTextStream& stream,
                                                       GpgSignature sign)
    -> bool {
  auto fingerprint = sign.GetFingerprint();

  LOG_D() << "Looking up key for fingerprint: " << fingerprint;

  auto key =
      AbstractKeyRepository::GetInstance(GetChannel()).GetKey(fingerprint);
  if (key != nullptr) {
    stream << "- " << tr("Signed By") << ": " << key->UID() << Qt::endl;

    if (key->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
      stream << "- " << tr("Key ID") << ": " << key->ID() << " ("
             << tr("Subkey") << ")" << Qt::endl;
    } else {
      stream << "- " << tr("Key ID") << ": " << key->ID() << " ("
             << tr("Primary Key") << ")" << Qt::endl;
    }
    stream << "- " << tr("Key Create Date") << ": "
           << QLocale().toString(key->CreationTime()) << Qt::endl;

  } else {
    stream_ << "- " << tr("Signed By") << "(" << tr("Fingerprint") << ")"
            << ": " << (fingerprint.isEmpty() ? tr("<unknown>") : fingerprint)
            << Qt::endl;
    setStatus(0);
  }

  stream << "- " << tr("Public Key Algo") << ": " << sign.GetPubkeyAlgo()
         << Qt::endl;
  stream << "- " << tr("Hash Algo") << ": " << sign.GetHashAlgo() << Qt::endl;
  stream << "- " << tr("Sign Date") << "(" << tr("UTC") << ")"
         << ": " << QLocale().toString(sign.GetCreateTime().toUTC())
         << Qt::endl;
  stream << "- " << tr("Sign Date") << "(" << tr("Localized") << ")"
         << ": " << QLocale().toString(sign.GetCreateTime()) << Qt::endl;
  stream << Qt::endl;

  return key != nullptr;
}

auto GpgFrontend::GpgVerifyResultAnalyse::GetSignatures() const
    -> gpgme_signature_t {
  if (result_.IsGood()) {
    return result_.GetRaw()->signatures;
  }
  return nullptr;
}

auto GpgFrontend::GpgVerifyResultAnalyse::TakeChargeOfResult()
    -> GpgFrontend::GpgVerifyResult {
  return result_;
}

auto GpgFrontend::GpgVerifyResultAnalyse::GetUnknownSignatures() const
    -> QStringList {
  return unknown_signer_fpr_list_;
}
