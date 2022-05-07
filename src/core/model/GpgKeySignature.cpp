/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "core/model/GpgKeySignature.h"

GpgFrontend::GpgKeySignature::GpgKeySignature() = default;

GpgFrontend::GpgKeySignature::~GpgKeySignature() = default;

GpgFrontend::GpgKeySignature::GpgKeySignature(gpgme_key_sig_t sig)
    : signature_ref_(sig, [&](gpgme_key_sig_t signature) {}) {}

GpgFrontend::GpgKeySignature::GpgKeySignature(GpgKeySignature &&) noexcept =
    default;

GpgFrontend::GpgKeySignature &GpgFrontend::GpgKeySignature::operator=(
    GpgKeySignature &&) noexcept = default;

bool GpgFrontend::GpgKeySignature::IsRevoked() const {
  return signature_ref_->revoked;
}

bool GpgFrontend::GpgKeySignature::IsExpired() const {
  return signature_ref_->expired;
}

bool GpgFrontend::GpgKeySignature::IsInvalid() const {
  return signature_ref_->invalid;
}

bool GpgFrontend::GpgKeySignature::IsExportable() const {
  return signature_ref_->exportable;
}

gpgme_error_t GpgFrontend::GpgKeySignature::GetStatus() const {
  return signature_ref_->status;
}

std::string GpgFrontend::GpgKeySignature::GetKeyID() const {
  return signature_ref_->keyid;
}

std::string GpgFrontend::GpgKeySignature::GetPubkeyAlgo() const {
  return gpgme_pubkey_algo_name(signature_ref_->pubkey_algo);
}

boost::posix_time::ptime GpgFrontend::GpgKeySignature::GetCreateTime() const {
  return boost::posix_time::from_time_t(signature_ref_->timestamp);
}

boost::posix_time::ptime GpgFrontend::GpgKeySignature::GetExpireTime() const {
  return boost::posix_time::from_time_t(signature_ref_->expires);
}

std::string GpgFrontend::GpgKeySignature::GetUID() const {
  return signature_ref_->uid;
}

std::string GpgFrontend::GpgKeySignature::GetName() const {
  return signature_ref_->name;
}

std::string GpgFrontend::GpgKeySignature::GetEmail() const {
  return signature_ref_->email;
}

std::string GpgFrontend::GpgKeySignature::GetComment() const {
  return signature_ref_->comment;
}