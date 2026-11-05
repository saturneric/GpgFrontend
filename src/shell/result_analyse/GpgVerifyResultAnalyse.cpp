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
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "shell/result_analyse/LocalizedUtils.h"

GpgFrontend::GpgVerifyResultAnalyse::GpgVerifyResultAnalyse(
    int channel, const GFError& error, GpgVerifyResult result)
    : GpgResultAnalyse(channel), error_(error), result_(result) {}

void GpgFrontend::GpgVerifyResultAnalyse::doAnalyse() {
  auto signatures = this->result_.GetSignature();

  setOperation(kOperation);
  recordEngineError(error_, result_.ErrorDetail());

  stream_ << "# " << tr("Verify Operation") << " (" << EngineInfo() << ") ";

  if (error_.Ok()) {
    stream_ << " - " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << " - " << tr("Failed") << ": " << DescribeGFError(error_).second
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
      switch (sign.GetStatus().code) {
        case GFErrorCode::kBAD_SIGNATURE:
          stream_ << tr("Invalid Signature") << Qt::endl;
          print_signer(stream_, GpgSignature(sign));
          stream_ << "  - " << tr("This signature could not be verified")
                  << Qt::endl;
          can_continue = false;
          setStatus(-1);
          break;
        case GFErrorCode::kOK: {
          // Determine overall status
          bool is_fully_valid = (sign.GetSummary() & kSIGSUM_VALID) != 0;
          bool is_green = (sign.GetSummary() & kSIGSUM_GREEN) != 0;
          bool is_red = (sign.GetSummary() & kSIGSUM_RED) != 0;
          // GnuPG's own verdict on the key: never valid, as with a TOFU
          // binding marked bad. gpg calls such a signature a likely forgery.
          const bool distrusted = sign.GetValidity() == GpgValidity::kNEVER;

          if (distrusted) {
            stream_ << tr("Signing Key Not to Be Trusted") << Qt::endl;
          } else if (is_fully_valid && is_green && !sign.HasTrustModel()) {
            // No trust model behind it: sound, but not vouched for.
            stream_ << tr("Valid") << Qt::endl;
          } else if (is_fully_valid && is_green) {
            stream_ << tr("Fully Valid") << Qt::endl;
          } else if (is_red) {
            stream_ << tr("Valid but with Issues") << Qt::endl;
          } else {
            stream_ << tr("Valid but Not Fully Trusted") << Qt::endl;
          }

          // List specific warnings
          QStringList warnings;
          if (distrusted) {
            warnings.append(
                tr("GnuPG marks the signing key as never valid: the signature "
                   "may be a forgery"));
          }
          if ((sign.GetSummary() & kSIGSUM_SIG_EXPIRED) != 0) {
            warnings.append(tr("Signature has expired"));
          }
          if ((sign.GetSummary() & kSIGSUM_KEY_MISSING) != 0) {
            warnings.append(tr("Signing key is missing"));
          }
          if ((sign.GetSummary() & kSIGSUM_KEY_REVOKED) != 0) {
            warnings.append(tr("Signing key has been revoked"));
          }
          if ((sign.GetSummary() & kSIGSUM_KEY_EXPIRED) != 0) {
            warnings.append(tr("Signing key has expired"));
          }
          if ((sign.GetSummary() & kSIGSUM_CRL_MISSING) != 0) {
            warnings.append(tr("Certificate revocation list is missing"));
          }

          if (!warnings.isEmpty()) {
            stream_ << "  - " << tr("Warnings") << ":" << Qt::endl;
            for (const auto& warning : warnings) {
              stream_ << "    - " << warning << Qt::endl;
            }
          }

          // Raising trust cannot help a key GnuPG itself rejected.
          if (!is_fully_valid && !is_red && warnings.isEmpty()) {
            stream_ << "  - " << tr("Tips") << ": "
                    << tr("Adjust trust level to make it fully valid")
                    << Qt::endl;
          }

          if ((sign.GetSummary() & kSIGSUM_KEY_MISSING) == 0U) {
            if (!print_signer(stream_, GpgSignature(sign))) setStatus(0);
          } else {
            stream_ << "  - " << tr("Key ID") << ": 0x" << sign.GetFingerprint()
                    << " (" << tr("not present in keyring") << ")" << Qt::endl;
          }

          setStatus(distrusted ? -1 : 1);
          break;
        }
        case GFErrorCode::kNO_PUBLIC_KEY:
          stream_ << tr("Cannot Verify due to Key Missing") << Qt::endl;
          stream_ << "  - "
                  << tr("The public key needed to verify this signature is not "
                        "available")
                  << Qt::endl;
          setStatus(0);  /// < key missing: unverified, not a failure
          print_signer_without_key(stream_, GpgSignature(sign));
          unknown_signer_fpr_list_.push_back(
              GpgSignature(sign).GetFingerprint());
          break;
        case GFErrorCode::kKEY_REVOKED:
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
        case GFErrorCode::kSIGNATURE_EXPIRED:
          stream_ << tr("Signature Expired") << Qt::endl;
          stream_ << "  - " << tr("The signature is valid but has expired")
                  << Qt::endl;
          if (!print_signer(stream_, GpgSignature(sign))) {
            setStatus(0);
          }
          setStatus(-1);
          break;
        case GFErrorCode::kKEY_EXPIRED:
          stream_ << tr("Signing Key Expired") << Qt::endl;
          if (sign.IsKeyExpiredAtSigning()) {
            // rPGP: the key had expired before it made this signature, so
            // the signature itself is not valid.
            stream_ << "  - "
                    << tr("The signing key had already expired when this "
                          "signature was made")
                    << Qt::endl;
          } else {
            stream_
                << "  - "
                << tr("The signature is valid but the signing key has expired")
                << Qt::endl;
          }
          if (!print_signer(stream_, GpgSignature(sign))) {
            setStatus(0);
          }
          if (sign.IsKeyExpiredAtSigning()) setStatus(-1);
          break;
        case GFErrorCode::kWRONG_KEY_USAGE:
          stream_ << tr("Signing Key Not Authorized") << Qt::endl;
          stream_ << "  - "
                  << tr("The signature is mathematically valid but the "
                        "certificate does not authorize this key to sign")
                  << Qt::endl;
          if (!print_signer(stream_, GpgSignature(sign))) {
            setStatus(0);
          }
          setStatus(-1);
          break;
        case GFErrorCode::kUNSUPPORTED_ALGORITHM:
          stream_ << tr("Weak Hash Algorithm") << Qt::endl;
          stream_ << "  - "
                  << tr("The signature relies on %1, a hash algorithm too "
                        "weak to trust, so it is not accepted")
                         .arg(sign.GetHashAlgo())
                  << Qt::endl;
          if (!print_signer(stream_, GpgSignature(sign))) {
            setStatus(0);
          }
          setStatus(-1);
          break;
        case GFErrorCode::kGENERAL:
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

  // A key the certificate does not let sign is kept apart from a bad
  // signature so the summary names the actual cause.
  bool any_bad_signature = false;
  bool any_not_authorized = false;
  // Whether any signer's validity comes from a trust model (GnuPG); rPGP has
  // none, so its valid signatures must not be summarized as trusted.
  bool any_trust_model = false;
  // Whether a signing key is carried by more than one key pair, so the
  // signature cannot be credited to one identity.
  bool any_shared_signer = false;
  // Whether GnuPG rated a signing key never valid (a likely forgery).
  bool any_distrusted = false;
  // Whether a signature was refused for its weak hash algorithm.
  bool any_weak_hash = false;

  for (const auto& sign : signatures) {
    GpgVerifySigInfo sig_info;
    sig_info.trustModel = sign.HasTrustModel();
    any_trust_model = any_trust_model || sig_info.trustModel;

    sig_info.signer.fingerprint = sign.GetFingerprint();
    sig_info.signer.pubkeyAlgo = sign.GetPubkeyAlgo();
    sig_info.signer.hashAlgo = sign.GetHashAlgo();
    sig_info.signer.signTime = sign.GetCreateTime().toUTC();

    auto key = AbstractKeyRepository::GetInstance(GetChannel())
                   .GetKey(sign.GetFingerprint());
    const auto owners = sharing_key_pairs(GpgSignature(sign));
    if (key != nullptr && !owners.isEmpty()) {
      // No single identity: list every key pair that may have signed.
      QStringList uids;
      for (const auto& owner : owners) {
        uids << QString("%1 (%2)").arg(owner->UID(), owner->ID());
      }
      sig_info.signer.uid = uids.join("; ");
      sig_info.signer.keyId = key->ID();
      sig_info.warnings << tr("Signing key is part of more than one "
                              "key pair; any of them may "
                              "have signed: %1")
                               .arg(uids.join("; "));
      op_info_.details << uids;
      any_shared_signer = true;
    } else if (key != nullptr) {
      sig_info.signer.uid = key->UID();
      sig_info.signer.keyId = key->ID();
      op_info_.details << key->UID();
    } else if (!sign.GetFingerprint().isEmpty()) {
      op_info_.details << sign.GetFingerprint();
    }

    switch (sign.GetStatus().code) {
      case GFErrorCode::kBAD_SIGNATURE:
        sig_info.validity = GpgSigValidity::kINVALID;
        any_bad_signature = true;
        break;
      case GFErrorCode::kOK: {
        bool is_fully_valid = (sign.GetSummary() & kSIGSUM_VALID) != 0;
        bool is_green = (sign.GetSummary() & kSIGSUM_GREEN) != 0;
        bool is_red = (sign.GetSummary() & kSIGSUM_RED) != 0;
        if (sign.GetValidity() == GpgValidity::kNEVER) {
          sig_info.validity = GpgSigValidity::kVALID_WITH_ISSUES;
          sig_info.rejected = true;
          sig_info.warnings << tr(
              "GnuPG marks the signing key as never valid: the signature may "
              "be a forgery");
          any_distrusted = true;
        } else if (is_fully_valid && is_green) {
          sig_info.validity = GpgSigValidity::kFULLY_VALID;
        } else if (is_red) {
          sig_info.validity = GpgSigValidity::kVALID_WITH_ISSUES;
        } else {
          sig_info.validity = GpgSigValidity::kVALID_NOT_FULLY_TRUSTED;
        }
        if ((sign.GetSummary() & kSIGSUM_SIG_EXPIRED) != 0)
          sig_info.warnings << tr("Signature has expired");
        if ((sign.GetSummary() & kSIGSUM_KEY_MISSING) != 0)
          sig_info.warnings << tr("Signing key is missing");
        if ((sign.GetSummary() & kSIGSUM_KEY_REVOKED) != 0)
          sig_info.warnings << tr("Signing key has been revoked");
        if ((sign.GetSummary() & kSIGSUM_KEY_EXPIRED) != 0)
          sig_info.warnings << tr("Signing key has expired");
        if ((sign.GetSummary() & kSIGSUM_CRL_MISSING) != 0)
          sig_info.warnings << tr("Certificate revocation list is missing");
        break;
      }
      case GFErrorCode::kNO_PUBLIC_KEY:
        sig_info.validity = GpgSigValidity::kKEY_MISSING;
        break;
      case GFErrorCode::kKEY_REVOKED:
        sig_info.validity = GpgSigValidity::kKEY_REVOKED;
        break;
      case GFErrorCode::kSIGNATURE_EXPIRED:
        sig_info.validity = GpgSigValidity::kSIG_EXPIRED;
        break;
      case GFErrorCode::kKEY_EXPIRED:
        sig_info.validity = GpgSigValidity::kKEY_EXPIRED;
        sig_info.rejected = sign.IsKeyExpiredAtSigning();
        break;
      case GFErrorCode::kWRONG_KEY_USAGE:
        sig_info.validity = GpgSigValidity::kKEY_NOT_AUTHORIZED;
        sig_info.warnings << tr(
            "Signing key is not authorized by its "
            "certificate");
        any_not_authorized = true;
        break;
      case GFErrorCode::kUNSUPPORTED_ALGORITHM:
        // Refused for its hash, not because it does not match the data.
        sig_info.validity = GpgSigValidity::kINVALID;
        sig_info.warnings
            << tr("Signature relies on a weak hash algorithm (%1)")
                   .arg(sign.GetHashAlgo());
        any_weak_hash = true;
        break;
      case GFErrorCode::kGENERAL:
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
    if (all_validity(GpgSigValidity::kFULLY_VALID) && !any_trust_model) {
      // rPGP has no trust model: a valid signature proves the data intact and
      // signed by that key, not that the key is trusted.
      const QString uid =
          sigs.size() == 1 ? sigs.first().signer.uid : QString();
      op_info_.description =
          uid.isEmpty()
              ? tr("The signature is valid. The data has not been tampered "
                   "with.")
              : tr("The signature by %1 is valid. The data has not been "
                   "tampered with.")
                    .arg(uid);
    } else if (all_validity(GpgSigValidity::kFULLY_VALID)) {
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
    if (any_shared_signer) {
      op_info_.description =
          tr("The signing key belongs to more than one key pair in your "
             "keyring, so it cannot be told which of them signed. Please "
             "review the details.");
    } else if (any_validity(GpgSigValidity::kKEY_MISSING)) {
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
    if (any_distrusted) {
      op_info_.description =
          tr("Verification failed. GnuPG marks the signing key as never "
             "valid, so the signature may be a forgery.");
    } else if (any_weak_hash && !any_bad_signature) {
      op_info_.description =
          tr("Verification failed. The signature relies on a hash algorithm "
             "too weak to trust.");
    } else if (any_bad_signature) {
      op_info_.description =
          tr("Invalid signature detected. The data may have been tampered "
             "with or the signature is corrupt.");
    } else if (any_not_authorized) {
      op_info_.description =
          tr("Signature verification failed. The signing key is not "
             "authorized to make signatures.");
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
  const auto owners = sharing_key_pairs(sign);
  if (key != nullptr && !owners.isEmpty()) {
    // The key alone cannot tell these key pairs apart, and naming the
    // one a lookup lands on would credit the signature to an identity it
    // does not prove.
    stream << "- " << tr("Signed By") << ": "
           << tr("Cannot be told apart, see below") << Qt::endl;
    stream << "- " << tr("Key ID") << ": " << key->ID() << Qt::endl;
    stream << "  - " << tr("Warning") << ": "
           << tr("This signing key is part of more than one key pair in "
                 "your keyring, and the signature does not say which of "
                 "them made it. Each of them may have signed:")
           << Qt::endl;
    for (const auto& owner : owners) {
      stream << "    - " << owner->UID() << " ("
             << BeautifyFingerprint(owner->Fingerprint()) << ")" << Qt::endl;
    }
    setStatus(0);
  } else if (key != nullptr) {
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

auto GpgFrontend::GpgVerifyResultAnalyse::sharing_key_pairs(
    const GpgSignature& sign) -> GpgKeyPtrList {
  if (sign.GetFingerprint().isEmpty()) return {};
  return GpgKeyRepository::GetInstance(GetChannel())
      .KeyPairsSharingKey(sign.GetFingerprint());
}

auto GpgFrontend::GpgVerifyResultAnalyse::TakeChargeOfResult()
    -> GpgFrontend::GpgVerifyResult {
  return result_;
}

auto GpgFrontend::GpgVerifyResultAnalyse::GetUnknownSignatures() const
    -> QStringList {
  return unknown_signer_fpr_list_;
}
