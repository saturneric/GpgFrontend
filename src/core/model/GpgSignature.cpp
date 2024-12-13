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

GpgSignature::GpgSignature(gpgme_signature_t sig) : signature_ref_(sig) {}

GpgSignature::~GpgSignature() = default;

GpgSignature::GpgSignature(const GpgSignature &) = default;

auto GpgSignature::operator=(const GpgSignature &) -> GpgSignature & = default;

/**
 * @brief
 *
 * @return gpgme_validity_t
 */
auto GpgSignature::GetValidity() const -> gpgme_validity_t {
  return signature_ref_->validity;
}

/**
 * @brief
 *
 * @return gpgme_error_t
 */
auto GpgSignature::GetStatus() const -> gpgme_error_t {
  return signature_ref_->status;
}

/**
 * @brief
 *
 * @return gpgme_error_t
 */
auto GpgSignature::GetSummary() const -> gpgme_error_t {
  return signature_ref_->summary;
}

/**
 * @brief
 *
 * @return QString
 */
auto GpgSignature::GetPubkeyAlgo() const -> QString {
  return gpgme_pubkey_algo_name(signature_ref_->pubkey_algo);
}

/**
 * @brief
 *
 * @return QString
 */
auto GpgSignature::GetHashAlgo() const -> QString {
  return gpgme_hash_algo_name(signature_ref_->hash_algo);
}

/**
 * @brief Create a time object
 *
 * @return QDateTime
 */
auto GpgSignature::GetCreateTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(signature_ref_->timestamp);
}

/**
 * @brief
 *
 * @return QDateTime
 */
auto GpgSignature::GetExpireTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(signature_ref_->exp_timestamp);
}

/**
 * @brief
 *
 * @return QString
 */
auto GpgSignature::GetFingerprint() const -> QString {
  return signature_ref_->fpr != nullptr ? signature_ref_->fpr : "";
}

}  // namespace GpgFrontend
