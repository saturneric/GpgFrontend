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

#include "GpgSignature.h"

/**
 * @brief Construct a new Gpg Signature object
 *
 */
GpgFrontend::GpgSignature::GpgSignature(GpgSignature &&) noexcept = default;

/**
 * @brief
 *
 * @return GpgSignature&
 */
GpgFrontend::GpgSignature &GpgFrontend::GpgSignature::operator=(
    GpgFrontend::GpgSignature &&) noexcept = default;

GpgFrontend::GpgSignature::GpgSignature(gpgme_signature_t sig)
    : signature_ref_(sig, [&](gpgme_signature_t signature) {}) {}

/**
 * @brief
 *
 * @return gpgme_validity_t
 */
gpgme_validity_t GpgFrontend::GpgSignature::GetValidity() const {
  return signature_ref_->validity;
}

/**
 * @brief
 *
 * @return gpgme_error_t
 */
gpgme_error_t GpgFrontend::GpgSignature::GetStatus() const {
  return signature_ref_->status;
}

/**
 * @brief
 *
 * @return gpgme_error_t
 */
gpgme_error_t GpgFrontend::GpgSignature::GetSummary() const {
  return signature_ref_->summary;
}

/**
 * @brief
 *
 * @return std::string
 */
std::string GpgFrontend::GpgSignature::GetPubkeyAlgo() const {
  return gpgme_pubkey_algo_name(signature_ref_->pubkey_algo);
}

/**
 * @brief
 *
 * @return std::string
 */
std::string GpgFrontend::GpgSignature::GetHashAlgo() const {
  return gpgme_hash_algo_name(signature_ref_->hash_algo);
}

/**
 * @brief Create a time object
 *
 * @return boost::posix_time::ptime
 */
boost::posix_time::ptime GpgFrontend::GpgSignature::GetCreateTime() const {
  return boost::posix_time::from_time_t(signature_ref_->timestamp);
}

/**
 * @brief
 *
 * @return boost::posix_time::ptime
 */
boost::posix_time::ptime GpgFrontend::GpgSignature::GetExpireTime() const {
  return boost::posix_time::from_time_t(signature_ref_->exp_timestamp);
}

/**
 * @brief
 *
 * @return std::string
 */
std::string GpgFrontend::GpgSignature::GetFingerprint() const {
  return signature_ref_->fpr;
}

/**
 * @brief Construct a new Gpg Signature object
 *
 */
GpgFrontend::GpgSignature::GpgSignature() = default;

/**
 * @brief Destroy the Gpg Signature object
 *
 */
GpgFrontend::GpgSignature::~GpgSignature() = default;
