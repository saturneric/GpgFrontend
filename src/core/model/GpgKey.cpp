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

GpgFrontend::GpgKey::GpgKey(gpgme_key_t &&key) : key_ref_(std::move(key)) {}

GpgFrontend::GpgKey::GpgKey(GpgKey &&k) noexcept { swap(key_ref_, k.key_ref_); }

GpgFrontend::GpgKey &GpgFrontend::GpgKey::operator=(GpgKey &&k) noexcept {
  swap(key_ref_, k.key_ref_);
  return *this;
}

bool GpgFrontend::GpgKey::operator==(const GpgKey &o) const {
  return o.GetId() == this->GetId();
}

bool GpgFrontend::GpgKey::operator<=(const GpgKey &o) const {
  return this->GetId() < o.GetId();
}

GpgFrontend::GpgKey::operator gpgme_key_t() const { return key_ref_.get(); }

bool GpgFrontend::GpgKey::IsGood() const { return key_ref_ != nullptr; }

std::string GpgFrontend::GpgKey::GetId() const {
  return key_ref_->subkeys->keyid;
}

std::string GpgFrontend::GpgKey::GetName() const {
  return key_ref_->uids->name;
};

std::string GpgFrontend::GpgKey::GetEmail() const {
  return key_ref_->uids->email;
}

std::string GpgFrontend::GpgKey::GetComment() const {
  return key_ref_->uids->comment;
}

std::string GpgFrontend::GpgKey::GetFingerprint() const {
  return key_ref_->fpr;
}

std::string GpgFrontend::GpgKey::GetProtocol() const {
  return gpgme_get_protocol_name(key_ref_->protocol);
}

std::string GpgFrontend::GpgKey::GetOwnerTrust() const {
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

int GpgFrontend::GpgKey::GetOwnerTrustLevel() const {
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

std::string GpgFrontend::GpgKey::GetPublicKeyAlgo() const {
  return gpgme_pubkey_algo_name(key_ref_->subkeys->pubkey_algo);
}

boost::posix_time::ptime GpgFrontend::GpgKey::GetLastUpdateTime() const {
  return boost::posix_time::from_time_t(
      static_cast<time_t>(key_ref_->last_update));
}

boost::posix_time::ptime GpgFrontend::GpgKey::GetExpireTime() const {
  return boost::posix_time::from_time_t(key_ref_->subkeys->expires);
};

boost::posix_time::ptime GpgFrontend::GpgKey::GetCreateTime() const {
  return boost::posix_time::from_time_t(key_ref_->subkeys->timestamp);
};

unsigned int GpgFrontend::GpgKey::GetPrimaryKeyLength() const {
  return key_ref_->subkeys->length;
}

bool GpgFrontend::GpgKey::IsHasEncryptionCapability() const {
  return key_ref_->can_encrypt;
}

bool GpgFrontend::GpgKey::IsHasSigningCapability() const {
  return key_ref_->can_sign;
}

bool GpgFrontend::GpgKey::IsHasCertificationCapability() const {
  return key_ref_->can_certify;
}

bool GpgFrontend::GpgKey::IsHasAuthenticationCapability() const {
  return key_ref_->can_authenticate;
}

bool GpgFrontend::GpgKey::IsHasCardKey() const {
  auto subkeys = GetSubKeys();
  return std::any_of(
      subkeys->begin(), subkeys->end(),
      [](const GpgSubKey &subkey) -> bool { return subkey.IsCardKey(); });
}

bool GpgFrontend::GpgKey::IsPrivateKey() const { return key_ref_->secret; }

bool GpgFrontend::GpgKey::IsExpired() const { return key_ref_->expired; }

bool GpgFrontend::GpgKey::IsRevoked() const { return key_ref_->revoked; }

bool GpgFrontend::GpgKey::IsDisabled() const { return key_ref_->disabled; }

bool GpgFrontend::GpgKey::IsHasMasterKey() const {
  return key_ref_->subkeys->secret;
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

GpgFrontend::GpgKey GpgFrontend::GpgKey::Copy() const {
  {
    const std::lock_guard<std::mutex> guard(gpgme_key_opera_mutex);
    gpgme_key_ref(key_ref_.get());
  }
  auto *_new_key_ref = key_ref_.get();
  return GpgKey(std::move(_new_key_ref));
}

void GpgFrontend::GpgKey::_key_ref_deleter::operator()(gpgme_key_t _key) {
  if (_key != nullptr) gpgme_key_unref(_key);
}
