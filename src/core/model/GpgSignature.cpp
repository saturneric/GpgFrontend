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

namespace GpgFrontend {

GpgSignature::GpgSignature() = default;

GpgSignature::GpgSignature(const GFSignature &gf_sig)
    : gf_signature_ref_(QSharedPointer<GFSignature>::create(gf_sig)) {}

GpgSignature::~GpgSignature() = default;

GpgSignature::GpgSignature(const GpgSignature &) = default;

auto GpgSignature::operator=(const GpgSignature &) -> GpgSignature & = default;

namespace {
auto RpgpSignatureError(GFErrorCode code) -> GFError {
  return {code, GFErrorOrigin::kRPGP, 0};
}
}  // namespace

auto GpgSignature::GetValidity() const -> GpgValidity {
  if (gf_signature_ref_ == nullptr) return GpgValidity::kUNKNOWN;
  return gf_signature_ref_->validity.value_or(GpgValidity::kFULL);
}

auto GpgSignature::GetStatus() const -> GFError {
  if (gf_signature_ref_ != nullptr && gf_signature_ref_->engine_status) {
    return *gf_signature_ref_->engine_status;
  }
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
  return GFError::Application(GFErrorCode::kGENERAL);
}

auto GpgSignature::GetSummary() const -> std::uint32_t {
  if (gf_signature_ref_ != nullptr && gf_signature_ref_->summary) {
    return *gf_signature_ref_->summary;
  }
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
  return kSIGSUM_RED;
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
  return {};
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
  return {};
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
  return QDateTime::fromSecsSinceEpoch(0);
}

/**
 * @brief
 *
 * @return QDateTime
 */
auto GpgSignature::GetExpireTime() const -> QDateTime {
  if (gf_signature_ref_ == nullptr) return QDateTime::fromSecsSinceEpoch(0);
  return QDateTime::fromSecsSinceEpoch(gf_signature_ref_->expires_at);
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
  return {};
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
  return gf_signature_ref_ == nullptr || gf_signature_ref_->has_trust_model;
}

}  // namespace GpgFrontend
