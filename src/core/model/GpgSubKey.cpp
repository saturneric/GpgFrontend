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
#include "core/model/GpgSubKey.h"

GpgFrontend::GpgSubKey::GpgSubKey() = default;

GpgFrontend::GpgSubKey::GpgSubKey(gpgme_subkey_t subkey)
    : _subkey_ref(subkey, [&](gpgme_subkey_t subkey) {}) {}

GpgFrontend::GpgSubKey::GpgSubKey(GpgSubKey&& o) noexcept {
  swap(_subkey_ref, o._subkey_ref);
}

GpgFrontend::GpgSubKey& GpgFrontend::GpgSubKey::operator=(
    GpgSubKey&& o) noexcept {
  swap(_subkey_ref, o._subkey_ref);
  return *this;
};

bool GpgFrontend::GpgSubKey::operator==(const GpgSubKey& o) const {
  return GetFingerprint() == o.GetFingerprint();
}

std::string GpgFrontend::GpgSubKey::GetID() const { return _subkey_ref->keyid; }

std::string GpgFrontend::GpgSubKey::GetFingerprint() const {
  return _subkey_ref->fpr;
}

std::string GpgFrontend::GpgSubKey::GetPubkeyAlgo() const {
  return gpgme_pubkey_algo_name(_subkey_ref->pubkey_algo);
}

unsigned int GpgFrontend::GpgSubKey::GetKeyLength() const {
  return _subkey_ref->length;
}

bool GpgFrontend::GpgSubKey::IsHasEncryptionCapability() const {
  return _subkey_ref->can_encrypt;
}

bool GpgFrontend::GpgSubKey::IsHasSigningCapability() const {
  return _subkey_ref->can_sign;
}

bool GpgFrontend::GpgSubKey::IsHasCertificationCapability() const {
  return _subkey_ref->can_certify;
}

bool GpgFrontend::GpgSubKey::IsHasAuthenticationCapability() const {
  return _subkey_ref->can_authenticate;
}

bool GpgFrontend::GpgSubKey::IsPrivateKey() const {
  return _subkey_ref->secret;
}

bool GpgFrontend::GpgSubKey::IsExpired() const { return _subkey_ref->expired; }

bool GpgFrontend::GpgSubKey::IsRevoked() const { return _subkey_ref->revoked; }

bool GpgFrontend::GpgSubKey::IsDisabled() const {
  return _subkey_ref->disabled;
}

bool GpgFrontend::GpgSubKey::IsSecretKey() const { return _subkey_ref->secret; }

bool GpgFrontend::GpgSubKey::IsCardKey() const {
  return _subkey_ref->is_cardkey;
}

boost::posix_time::ptime GpgFrontend::GpgSubKey::GetCreateTime() const {
  return boost::posix_time::from_time_t(_subkey_ref->timestamp);
}

boost::posix_time::ptime GpgFrontend::GpgSubKey::GetExpireTime() const {
  return boost::posix_time::from_time_t(_subkey_ref->expires);
}
