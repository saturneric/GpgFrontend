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

#include "core/model/GpgKeySignature.h"

namespace GpgFrontend {

GpgKeySignature::GpgKeySignature() = default;

GpgKeySignature::~GpgKeySignature() = default;

GpgKeySignature::GpgKeySignature(gpgme_key_sig_t sig) : signature_ref_(sig) {}

GpgKeySignature::GpgKeySignature(const GpgKeySignature &) = default;
auto GpgKeySignature::operator=(const GpgKeySignature &) -> GpgKeySignature & =
                                                                default;

auto GpgKeySignature::IsRevoked() const -> bool {
  return signature_ref_->revoked;
}

auto GpgKeySignature::IsExpired() const -> bool {
  return signature_ref_->expired;
}

auto GpgKeySignature::IsInvalid() const -> bool {
  return signature_ref_->invalid;
}

auto GpgKeySignature::IsExportable() const -> bool {
  return signature_ref_->exportable;
}

auto GpgKeySignature::GetStatus() const -> gpgme_error_t {
  return signature_ref_->status;
}

auto GpgKeySignature::GetKeyID() const -> QString {
  return signature_ref_->keyid;
}

auto GpgKeySignature::GetPubkeyAlgo() const -> QString {
  return gpgme_pubkey_algo_name(signature_ref_->pubkey_algo);
}

auto GpgKeySignature::GetCreateTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(signature_ref_->timestamp);
}

auto GpgKeySignature::GetExpireTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(signature_ref_->expires);
}

auto GpgKeySignature::GetUID() const -> QString { return signature_ref_->uid; }

auto GpgKeySignature::GetName() const -> QString {
  return signature_ref_->name;
}

auto GpgKeySignature::GetEmail() const -> QString {
  return signature_ref_->email;
}

auto GpgKeySignature::GetComment() const -> QString {
  return signature_ref_->comment;
}

}  // namespace GpgFrontend