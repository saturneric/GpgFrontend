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
#include "GpgSubKey.h"

namespace GpgFrontend {

GpgSubKey::GpgSubKey() = default;

GpgSubKey::GpgSubKey(gpgme_subkey_t subkey)
    : subkey_ref_(subkey, [&](gpgme_subkey_t subkey) {}) {}

GpgSubKey::GpgSubKey(GpgSubKey&& o) noexcept {
  swap(subkey_ref_, o.subkey_ref_);
}

auto GpgSubKey::operator=(GpgSubKey&& o) noexcept -> GpgSubKey& {
  swap(subkey_ref_, o.subkey_ref_);
  return *this;
};

auto GpgSubKey::operator==(const GpgSubKey& o) const -> bool {
  return GetFingerprint() == o.GetFingerprint();
}

auto GpgSubKey::GetID() const -> QString { return subkey_ref_->keyid; }

auto GpgSubKey::GetFingerprint() const -> QString { return subkey_ref_->fpr; }

auto GpgSubKey::GetPubkeyAlgo() const -> QString {
  return gpgme_pubkey_algo_name(subkey_ref_->pubkey_algo);
}

auto GpgSubKey::GetKeyAlgo() const -> QString {
  auto* buffer = gpgme_pubkey_algo_string(subkey_ref_.get());
  auto algo = QString(buffer);
  gpgme_free(buffer);
  return algo.toUpper();
}

auto GpgSubKey::GetKeyLength() const -> unsigned int {
  return subkey_ref_->length;
}

auto GpgSubKey::IsHasEncryptionCapability() const -> bool {
  return subkey_ref_->can_encrypt;
}

auto GpgSubKey::IsHasSigningCapability() const -> bool {
  return subkey_ref_->can_sign;
}

auto GpgSubKey::IsHasCertificationCapability() const -> bool {
  return subkey_ref_->can_certify;
}

auto GpgSubKey::IsHasAuthenticationCapability() const -> bool {
  return subkey_ref_->can_authenticate;
}

auto GpgSubKey::IsPrivateKey() const -> bool { return subkey_ref_->secret; }

auto GpgSubKey::IsExpired() const -> bool { return subkey_ref_->expired; }

auto GpgSubKey::IsRevoked() const -> bool { return subkey_ref_->revoked; }

auto GpgSubKey::IsDisabled() const -> bool { return subkey_ref_->disabled; }

auto GpgSubKey::IsSecretKey() const -> bool { return subkey_ref_->secret; }

auto GpgSubKey::IsCardKey() const -> bool { return subkey_ref_->is_cardkey; }

auto GpgSubKey::GetCreateTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(subkey_ref_->timestamp);
}

auto GpgSubKey::GetExpireTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(subkey_ref_->expires);
}

}  // namespace GpgFrontend
