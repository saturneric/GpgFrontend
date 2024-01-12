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

#include "core/model/GpgKey.h"

#include <mutex>

namespace GpgFrontend {

GpgKey::GpgKey(gpgme_key_t &&key) : key_ref_(key) {}

GpgKey::GpgKey(GpgKey &&k) noexcept { swap(key_ref_, k.key_ref_); }

auto GpgKey::operator=(GpgKey &&k) noexcept -> GpgKey & {
  swap(key_ref_, k.key_ref_);
  return *this;
}

GpgKey::GpgKey(const GpgKey &key) {
  auto *key_ref = key.key_ref_.get();
  gpgme_key_ref(key_ref);
  this->key_ref_ = KeyRefHandler(key_ref);
}

auto GpgKey::operator=(const GpgKey &key) -> GpgKey & {
  if (this == &key) {
    return *this;
  }

  auto *key_ref = key.key_ref_.get();
  gpgme_key_ref(key_ref);

  this->key_ref_ = KeyRefHandler(key_ref);
  return *this;
}

auto GpgKey::operator==(const GpgKey &o) const -> bool {
  return o.GetId() == this->GetId();
}

auto GpgKey::operator<=(const GpgKey &o) const -> bool {
  return this->GetId() < o.GetId();
}

GpgKey::operator gpgme_key_t() const { return key_ref_.get(); }

auto GpgKey::IsGood() const -> bool { return key_ref_ != nullptr; }

auto GpgKey::GetId() const -> QString { return key_ref_->subkeys->keyid; }

auto GpgKey::GetName() const -> QString { return key_ref_->uids->name; };

auto GpgKey::GetEmail() const -> QString { return key_ref_->uids->email; }

auto GpgKey::GetComment() const -> QString { return key_ref_->uids->comment; }

auto GpgKey::GetFingerprint() const -> QString { return key_ref_->fpr; }

auto GpgKey::GetProtocol() const -> QString {
  return gpgme_get_protocol_name(key_ref_->protocol);
}

auto GpgKey::GetOwnerTrust() const -> QString {
  switch (key_ref_->owner_trust) {
    case GPGME_VALIDITY_UNKNOWN:
      return _("Unknown");
    case GPGME_VALIDITY_UNDEFINED:
      return _("Undefined");
    case GPGME_VALIDITY_NEVER:
      return _("Never");
    case GPGME_VALIDITY_MARGINAL:
      return _("Marginal");
    case GPGME_VALIDITY_FULL:
      return _("Full");
    case GPGME_VALIDITY_ULTIMATE:
      return _("Ultimate");
  }
  return "Invalid";
}

auto GpgKey::GetOwnerTrustLevel() const -> int {
  switch (key_ref_->owner_trust) {
    case GPGME_VALIDITY_UNKNOWN:
      return 0;
    case GPGME_VALIDITY_UNDEFINED:
      return 1;
    case GPGME_VALIDITY_NEVER:
      return 2;
    case GPGME_VALIDITY_MARGINAL:
      return 3;
    case GPGME_VALIDITY_FULL:
      return 4;
    case GPGME_VALIDITY_ULTIMATE:
      return 5;
  }
  return 0;
}

auto GpgKey::GetPublicKeyAlgo() const -> QString {
  return gpgme_pubkey_algo_name(key_ref_->subkeys->pubkey_algo);
}

auto GpgKey::GetLastUpdateTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(
      static_cast<time_t>(key_ref_->last_update));
}

auto GpgKey::GetExpireTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(key_ref_->subkeys->expires);
};

auto GpgKey::GetCreateTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(key_ref_->subkeys->timestamp);
};

auto GpgKey::GetPrimaryKeyLength() const -> unsigned int {
  return key_ref_->subkeys->length;
}

auto GpgKey::IsHasEncryptionCapability() const -> bool {
  return key_ref_->can_encrypt;
}

auto GpgKey::IsHasSigningCapability() const -> bool {
  return key_ref_->can_sign;
}

auto GpgKey::IsHasCertificationCapability() const -> bool {
  return key_ref_->can_certify;
}

auto GpgKey::IsHasAuthenticationCapability() const -> bool {
  return key_ref_->can_authenticate;
}

auto GpgKey::IsHasCardKey() const -> bool {
  auto subkeys = GetSubKeys();
  return std::any_of(
      subkeys->begin(), subkeys->end(),
      [](const GpgSubKey &subkey) -> bool { return subkey.IsCardKey(); });
}

auto GpgKey::IsPrivateKey() const -> bool { return key_ref_->secret; }

auto GpgKey::IsExpired() const -> bool { return key_ref_->expired; }

auto GpgKey::IsRevoked() const -> bool { return key_ref_->revoked; }

auto GpgKey::IsDisabled() const -> bool { return key_ref_->disabled; }

auto GpgKey::IsHasMasterKey() const -> bool {
  return key_ref_->subkeys->secret;
}

auto GpgKey::GetSubKeys() const -> std::unique_ptr<std::vector<GpgSubKey>> {
  auto p_keys = std::make_unique<std::vector<GpgSubKey>>();
  auto *next = key_ref_->subkeys;
  while (next != nullptr) {
    p_keys->push_back(GpgSubKey(next));
    next = next->next;
  }
  return p_keys;
}

auto GpgKey::GetUIDs() const -> std::unique_ptr<std::vector<GpgUID>> {
  auto p_uids = std::make_unique<std::vector<GpgUID>>();
  auto *uid_next = key_ref_->uids;
  while (uid_next != nullptr) {
    p_uids->push_back(GpgUID(uid_next));
    uid_next = uid_next->next;
  }
  return p_uids;
}

auto GpgKey::IsHasActualSigningCapability() const -> bool {
  auto subkeys = GetSubKeys();
  return std::any_of(
      subkeys->begin(), subkeys->end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.IsSecretKey() && subkey.IsHasSigningCapability() &&
               !subkey.IsDisabled() && !subkey.IsRevoked() &&
               !subkey.IsExpired();
      });
}

auto GpgKey::IsHasActualAuthenticationCapability() const -> bool {
  auto subkeys = GetSubKeys();
  return std::any_of(
      subkeys->begin(), subkeys->end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.IsSecretKey() && subkey.IsHasAuthenticationCapability() &&
               !subkey.IsDisabled() && !subkey.IsRevoked() &&
               !subkey.IsExpired();
      });
}

/**
 * check if key can certify(actually)
 * @param key target key
 * @return if key certify
 */
auto GpgKey::IsHasActualCertificationCapability() const -> bool {
  return IsHasMasterKey() && !IsExpired() && !IsRevoked() && !IsDisabled();
}

/**
 * check if key can encrypt(actually)
 * @param key target key
 * @return if key encrypt
 */
auto GpgKey::IsHasActualEncryptionCapability() const -> bool {
  auto subkeys = GetSubKeys();
  return std::any_of(
      subkeys->begin(), subkeys->end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.IsHasEncryptionCapability() && !subkey.IsDisabled() &&
               !subkey.IsRevoked() && !subkey.IsExpired();
      });
}

void GpgKey::KeyRefDeleter::operator()(gpgme_key_t _key) {
  if (_key != nullptr) gpgme_key_unref(_key);
}

}  // namespace GpgFrontend