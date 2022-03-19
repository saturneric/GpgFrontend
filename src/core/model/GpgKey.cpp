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

#include "core/model/GpgKey.h"

GpgFrontend::GpgKey::GpgKey(gpgme_key_t &&key) : key_ref_(std::move(key)) {}

GpgFrontend::GpgKey::GpgKey(GpgKey &&k) noexcept { swap(key_ref_, k.key_ref_); }

GpgFrontend::GpgKey &GpgFrontend::GpgKey::operator=(GpgKey &&k) noexcept {
  swap(key_ref_, k.key_ref_);
  return *this;
}

std::unique_ptr<std::vector<GpgFrontend::GpgSubKey>>
GpgFrontend::GpgKey::GetSubKeys() const {
  auto p_keys = std::make_unique<std::vector<GpgSubKey>>();
  auto next = key_ref_->subkeys;
  while (next != nullptr) {
    p_keys->push_back(GpgSubKey(next));
    next = next->next;
  }
  return p_keys;
}

std::unique_ptr<std::vector<GpgFrontend::GpgUID>> GpgFrontend::GpgKey::GetUIDs()
    const {
  auto p_uids = std::make_unique<std::vector<GpgUID>>();
  auto uid_next = key_ref_->uids;
  while (uid_next != nullptr) {
    p_uids->push_back(GpgUID(uid_next));
    uid_next = uid_next->next;
  }
  return p_uids;
}

bool GpgFrontend::GpgKey::IsHasActualSigningCapability() const {
  auto subkeys = GetSubKeys();
  if (std::any_of(subkeys->begin(), subkeys->end(),
                  [](const GpgSubKey &subkey) -> bool {
                    return subkey.IsSecretKey() &&
                           subkey.IsHasSigningCapability() &&
                           !subkey.IsDisabled() && !subkey.IsRevoked() &&
                           !subkey.IsExpired();
                  }))
    return true;
  else
    return false;
}

bool GpgFrontend::GpgKey::IsHasActualAuthenticationCapability() const {
  auto subkeys = GetSubKeys();
  if (std::any_of(subkeys->begin(), subkeys->end(),
                  [](const GpgSubKey &subkey) -> bool {
                    return subkey.IsSecretKey() &&
                           subkey.IsHasAuthenticationCapability() &&
                           !subkey.IsDisabled() && !subkey.IsRevoked() &&
                           !subkey.IsExpired();
                  }))
    return true;
  else
    return false;
}

/**
 * check if key can certify(actually)
 * @param key target key
 * @return if key certify
 */
bool GpgFrontend::GpgKey::IsHasActualCertificationCapability() const {
  return IsHasMasterKey() && !IsExpired() && !IsRevoked() && !IsDisabled();
}

/**
 * check if key can encrypt(actually)
 * @param key target key
 * @return if key encrypt
 */
bool GpgFrontend::GpgKey::IsHasActualEncryptionCapability() const {
  auto subkeys = GetSubKeys();
  if (std::any_of(subkeys->begin(), subkeys->end(),
                  [](const GpgSubKey &subkey) -> bool {
                    return subkey.IsHasEncryptionCapability() &&
                           !subkey.IsDisabled() && !subkey.IsRevoked() &&
                           !subkey.IsExpired();
                  }))
    return true;
  else
    return false;
}
