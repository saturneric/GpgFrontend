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

#include "core/model/GpgKeySignature.h"

namespace GpgFrontend {

GpgKeySignature::GpgKeySignature() = default;

GpgKeySignature::~GpgKeySignature() = default;

GpgKeySignature::GpgKeySignature(gpgme_key_sig_t sig)
    : signature_ref_(sig, [&](gpgme_key_sig_t signature) {}) {}

GpgKeySignature::GpgKeySignature(GpgKeySignature &&) noexcept = default;

GpgKeySignature &GpgKeySignature::operator=(GpgKeySignature &&) noexcept =
    default;

bool GpgKeySignature::IsRevoked() const { return signature_ref_->revoked; }

bool GpgKeySignature::IsExpired() const { return signature_ref_->expired; }

bool GpgKeySignature::IsInvalid() const { return signature_ref_->invalid; }

bool GpgKeySignature::IsExportable() const {
  return signature_ref_->exportable;
}

gpgme_error_t GpgKeySignature::GetStatus() const {
  return signature_ref_->status;
}

QString GpgKeySignature::GetKeyID() const { return signature_ref_->keyid; }

QString GpgKeySignature::GetPubkeyAlgo() const {
  return gpgme_pubkey_algo_name(signature_ref_->pubkey_algo);
}

QDateTime GpgKeySignature::GetCreateTime() const {
  return QDateTime::fromSecsSinceEpoch(signature_ref_->timestamp);
}

QDateTime GpgKeySignature::GetExpireTime() const {
  return QDateTime::fromSecsSinceEpoch(signature_ref_->expires);
}

QString GpgKeySignature::GetUID() const { return signature_ref_->uid; }

QString GpgKeySignature::GetName() const { return signature_ref_->name; }

QString GpgKeySignature::GetEmail() const { return signature_ref_->email; }

QString GpgKeySignature::GetComment() const { return signature_ref_->comment; }
}  // namespace GpgFrontend