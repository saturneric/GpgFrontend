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

#include "GpgSignature.h"

#include "core/function/gpg/GpgErrorMap.h"

namespace GpgFrontend {

GpgSignature::GpgSignature() = default;

GpgSignature::GpgSignature(gpgme_signature_t sig) : signature_ref_(sig) {}

GpgSignature::GpgSignature(const GFSignature &gf_sig)
    : gf_signature_ref_(QSharedPointer<GFSignature>::create(gf_sig)) {}

GpgSignature::~GpgSignature() = default;

GpgSignature::GpgSignature(const GpgSignature &) = default;

auto GpgSignature::operator=(const GpgSignature &) -> GpgSignature & = default;

namespace {
auto ValidityOf(gpgme_validity_t validity) -> GpgValidity {
  switch (validity) {
    case GPGME_VALIDITY_UNDEFINED:
      return GpgValidity::kUNDEFINED;
    case GPGME_VALIDITY_NEVER:
      return GpgValidity::kNEVER;
    case GPGME_VALIDITY_MARGINAL:
      return GpgValidity::kMARGINAL;
    case GPGME_VALIDITY_FULL:
      return GpgValidity::kFULL;
    case GPGME_VALIDITY_ULTIMATE:
      return GpgValidity::kULTIMATE;
    case GPGME_VALIDITY_UNKNOWN:
    default:
      return GpgValidity::kUNKNOWN;
  }
}

auto SummaryOf(gpgme_sigsum_t summary) -> std::uint32_t {
  std::uint32_t flags = kSIGSUM_NONE;
  const auto map = [&](unsigned gpgme_bit, std::uint32_t ours) {
    if ((summary & gpgme_bit) != 0) flags |= ours;
  };
  map(GPGME_SIGSUM_VALID, kSIGSUM_VALID);
  map(GPGME_SIGSUM_GREEN, kSIGSUM_GREEN);
  map(GPGME_SIGSUM_RED, kSIGSUM_RED);
  map(GPGME_SIGSUM_KEY_REVOKED, kSIGSUM_KEY_REVOKED);
  map(GPGME_SIGSUM_KEY_EXPIRED, kSIGSUM_KEY_EXPIRED);
  map(GPGME_SIGSUM_SIG_EXPIRED, kSIGSUM_SIG_EXPIRED);
  map(GPGME_SIGSUM_KEY_MISSING, kSIGSUM_KEY_MISSING);
  map(GPGME_SIGSUM_CRL_MISSING, kSIGSUM_CRL_MISSING);
  return flags;
}

auto RpgpSignatureError(GFErrorCode code) -> GFError {
  return {code, GFErrorOrigin::kRPGP, 0};
}
}  // namespace

auto GpgSignature::GetValidity() const -> GpgValidity {
  if (gf_signature_ref_ != nullptr) return GpgValidity::kFULL;
  if (signature_ref_ == nullptr) return GpgValidity::kUNKNOWN;
  return ValidityOf(signature_ref_->validity);
}

auto GpgSignature::GetStatus() const -> GFError {
  if (gf_signature_ref_ != nullptr) {
    switch (gf_signature_ref_->status) {
      case GFSignatureStatus::kVALID:
        return {};
      case GFSignatureStatus::kBAD_SIGNATURE:
        return RpgpSignatureError(GFErrorCode::kBAD_SIGNATURE);
      case GFSignatureStatus::kNO_KEY:
        return RpgpSignatureError(GFErrorCode::kNO_PUBLIC_KEY);
      case GFSignatureStatus::kKEY_REVOKED:
        return RpgpSignatureError(GFErrorCode::kKEY_REVOKED);
      case GFSignatureStatus::kKEY_EXPIRED:
        return RpgpSignatureError(GFErrorCode::kKEY_EXPIRED);
      case GFSignatureStatus::kKEY_NOT_AUTHORIZED:
        return RpgpSignatureError(GFErrorCode::kWRONG_KEY_USAGE);
      default:
        return RpgpSignatureError(GFErrorCode::kGENERAL);
    }
  }
  if (signature_ref_ == nullptr) {
    return GFError::Application(GFErrorCode::kGENERAL);
  }
  return FromGpgError(signature_ref_->status);
}

auto GpgSignature::GetSummary() const -> std::uint32_t {
  if (gf_signature_ref_ != nullptr) {
    switch (gf_signature_ref_->status) {
      case GFSignatureStatus::kVALID:
        return kSIGSUM_VALID | kSIGSUM_GREEN;
      case GFSignatureStatus::kKEY_REVOKED:
        return kSIGSUM_RED | kSIGSUM_KEY_REVOKED;
      case GFSignatureStatus::kKEY_EXPIRED:
        return kSIGSUM_RED | kSIGSUM_KEY_EXPIRED;
      default:
        return kSIGSUM_RED;
    }
  }
  if (signature_ref_ == nullptr) return kSIGSUM_RED;
  return SummaryOf(signature_ref_->summary);
}

/**
 * @brief
 *
 * @return QString
 */
auto GpgSignature::GetPubkeyAlgo() const -> QString {
  if (gf_signature_ref_ != nullptr) {
    return gf_signature_ref_->pub_algo;
  }
  if (signature_ref_ == nullptr) return {};
  return gpgme_pubkey_algo_name(signature_ref_->pubkey_algo);
}

/**
 * @brief
 *
 * @return QString
 */
auto GpgSignature::GetHashAlgo() const -> QString {
  if (gf_signature_ref_ != nullptr) {
    return gf_signature_ref_->hash_algo;
  }
  if (signature_ref_ == nullptr) return {};
  return gpgme_hash_algo_name(signature_ref_->hash_algo);
}

/**
 * @brief Create a time object
 *
 * @return QDateTime
 */
auto GpgSignature::GetCreateTime() const -> QDateTime {
  if (gf_signature_ref_ != nullptr) {
    return QDateTime::fromSecsSinceEpoch(gf_signature_ref_->created_at);
  }
  if (signature_ref_ == nullptr) return QDateTime::fromSecsSinceEpoch(0);
  return QDateTime::fromSecsSinceEpoch(signature_ref_->timestamp);
}

/**
 * @brief
 *
 * @return QDateTime
 */
auto GpgSignature::GetExpireTime() const -> QDateTime {
  if (gf_signature_ref_ != nullptr) {
    return QDateTime::fromSecsSinceEpoch(0);
  }
  if (signature_ref_ == nullptr) return QDateTime::fromSecsSinceEpoch(0);
  return QDateTime::fromSecsSinceEpoch(signature_ref_->exp_timestamp);
}

/**
 * @brief
 *
 * @return QString
 */
auto GpgSignature::GetFingerprint() const -> QString {
  if (gf_signature_ref_ != nullptr) {
    return gf_signature_ref_->issuer_fpr;
  }
  if (signature_ref_ == nullptr) return {};
  return signature_ref_->fpr != nullptr ? signature_ref_->fpr : "";
}

[[nodiscard]] auto GpgSignature::GetSigType() const -> QString {
  if (gf_signature_ref_ != nullptr) {
    return gf_signature_ref_->sig_type;
  }
  return {};
}

auto GpgSignature::IsKeyExpiredAtSigning() const -> bool {
  return gf_signature_ref_ != nullptr &&
         gf_signature_ref_->status == GFSignatureStatus::kKEY_EXPIRED;
}

auto GpgSignature::HasTrustModel() const -> bool {
  return gf_signature_ref_ == nullptr;
}

}  // namespace GpgFrontend
