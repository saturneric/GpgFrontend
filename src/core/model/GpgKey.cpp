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

#include "core/model/GpgKey.h"

#include "core/utils/RustUtils.h"

namespace GpgFrontend {

GpgKey::GpgKey() = default;

GpgKey::GpgKey(gpgme_key_t key)
    : key_ref_(key, [](struct _gpgme_key *ptr) -> void {
        if (ptr != nullptr) gpgme_key_unref(ptr);
      }) {}

GpgKey::GpgKey(QSharedPointer<struct _gpgme_key> key_ref)
    : key_ref_(std::move(key_ref)) {}

GpgKey::operator gpgme_key_t() const {
  assert(km_ref_ == nullptr);
  return key_ref_.get();
}

GpgKey::GpgKey(const GpgKey &) = default;

GpgKey::~GpgKey() = default;

auto GpgKey::operator=(const GpgKey &) -> GpgKey & = default;

auto GpgKey::IsGood() const -> bool {
  return key_ref_ != nullptr || km_ref_ != nullptr;
}

auto GpgKey::ID() const -> QString {
  if (km_ref_ != nullptr) return km_ref_->key_id;
  return key_ref_->subkeys->keyid;
}

auto GpgKey::Name() const -> QString {
  if (km_ref_ != nullptr) {
    auto name =
        km_ref_->user_ids.empty() ? QString{} : km_ref_->user_ids.front().name;
    return name;
  }
  return key_ref_->uids->name;
}

auto GpgKey::Email() const -> QString {
  if (km_ref_ != nullptr) {
    auto email =
        km_ref_->user_ids.empty() ? QString{} : km_ref_->user_ids.front().email;
    return email;
  }
  return key_ref_->uids->email;
}
auto GpgKey::Comment() const -> QString {
  if (km_ref_ != nullptr) {
    auto comment = km_ref_->user_ids.empty()
                       ? QString{}
                       : km_ref_->user_ids.front().comment;
    return comment;
  }
  return key_ref_->uids->comment;
}

auto GpgKey::Fingerprint() const -> QString {
  if (km_ref_ != nullptr) return km_ref_->fpr;
  return key_ref_->fpr;
}

auto GpgKey::Protocol() const -> QString {
  if (km_ref_ != nullptr) return {};
  return gpgme_get_protocol_name(key_ref_->protocol);
}

auto GpgKey::OwnerTrust() const -> QString {
  if (km_ref_ != nullptr) return {};

  switch (key_ref_->owner_trust) {
    case GPGME_VALIDITY_UNKNOWN:
      return tr("Unknown");
    case GPGME_VALIDITY_UNDEFINED:
      return tr("Undefined");
    case GPGME_VALIDITY_NEVER:
      return tr("Never");
    case GPGME_VALIDITY_MARGINAL:
      return tr("Marginal");
    case GPGME_VALIDITY_FULL:
      return tr("Full");
    case GPGME_VALIDITY_ULTIMATE:
      return tr("Ultimate");
  }
  return "Invalid";
}

auto GpgKey::OwnerTrustLevel() const -> int {
  if (km_ref_ != nullptr) return 0;

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

auto GpgKey::PublicKeyAlgo() const -> QString {
  if (km_ref_ != nullptr) {
    return GfrKeyAlgo2KeyAlgoName(static_cast<Rust::GfrKeyAlgo>(km_ref_->algo));
  }

  return gpgme_pubkey_algo_name(key_ref_->subkeys->pubkey_algo);
}

auto GpgKey::Algo() const -> QString {
  if (km_ref_ != nullptr) {
    return GfrKeyAlgo2KeyAlgoName(static_cast<Rust::GfrKeyAlgo>(km_ref_->algo));
  }

  auto *buffer = gpgme_pubkey_algo_string(key_ref_->subkeys);
  auto algo = QString(buffer);
  gpgme_free(buffer);
  return algo.toUpper();
}

auto GpgKey::LastUpdateTime() const -> QDateTime {
  if (km_ref_ != nullptr) return {};
  return QDateTime::fromSecsSinceEpoch(
      static_cast<time_t>(key_ref_->last_update));
}

auto GpgKey::ExpirationTime() const -> QDateTime {
  if (km_ref_ != nullptr) return QDateTime::fromSecsSinceEpoch(0);
  return QDateTime::fromSecsSinceEpoch(key_ref_->subkeys->expires);
};

auto GpgKey::CreationTime() const -> QDateTime {
  if (km_ref_ != nullptr) {
    return QDateTime::fromSecsSinceEpoch(km_ref_->created_at);
  }
  return QDateTime::fromSecsSinceEpoch(key_ref_->subkeys->timestamp);
};

auto GpgKey::PrimaryKeyLength() const -> unsigned int {
  if (km_ref_ != nullptr) return 0;
  return key_ref_->subkeys->length;
}

auto GpgKey::IsHasEncrCap() const -> bool { return IsHasActualEncrCap(); }

auto GpgKey::IsHasSignCap() const -> bool { return IsHasActualSignCap(); }

auto GpgKey::IsHasCertCap() const -> bool { return IsHasActualCertCap(); }

auto GpgKey::IsHasAuthCap() const -> bool { return IsHasActualAuthCap(); }

auto GpgKey::IsHasCardKey() const -> bool {
  auto sub_keys = SubKeys();
  return std::any_of(
      sub_keys.begin(), sub_keys.end(),
      [](const GpgSubKey &subkey) -> bool { return subkey.IsCardKey(); });
}

auto GpgKey::IsPrivateKey() const -> bool {
  if (km_ref_ != nullptr) return km_ref_->has_secret;
  return key_ref_->secret;
}

auto GpgKey::IsExpired() const -> bool {
  if (km_ref_ != nullptr) return false;
  return key_ref_->expired;
}

auto GpgKey::IsRevoked() const -> bool {
  if (km_ref_ != nullptr) return false;
  return key_ref_->revoked;
}

auto GpgKey::IsDisabled() const -> bool {
  if (km_ref_ != nullptr) return false;
  return key_ref_->disabled;
}

auto GpgKey::IsHasMasterKey() const -> bool {
  if (km_ref_ != nullptr) return km_ref_->has_secret;
  return key_ref_->subkeys->secret;
}

auto GpgKey::SubKeys() const -> QContainer<GpgSubKey> {
  QContainer<GpgSubKey> ret;

  if (km_ref_ != nullptr) {
    for (const auto &sub : km_ref_->subkeys) {
      ret.push_back(GpgSubKey(km_ref_, sub));
    }
    return ret;
  }

  auto *next = key_ref_->subkeys;
  while (next != nullptr) {
    ret.push_back(GpgSubKey(key_ref_, next));
    next = next->next;
  }
  return ret;
}

auto GpgKey::UIDs() const -> QContainer<GpgUID> {
  QContainer<GpgUID> uids;

  if (km_ref_ != nullptr) {
    for (const auto &uid : km_ref_->user_ids) {
      uids.push_back(GpgUID(km_ref_, uid));
    }
    return uids;
  }

  auto *next = key_ref_->uids;
  while (next != nullptr) {
    uids.push_back(GpgUID(key_ref_, next));
    next = next->next;
  }

  return uids;
}

auto GpgKey::IsHasActualSignCap() const -> bool {
  auto s_keys = SubKeys();
  return std::any_of(
      s_keys.begin(), s_keys.end(), [](const GpgSubKey &s_key) -> bool {
        return s_key.IsSecretKey() && s_key.IsHasSignCap() &&
               !s_key.IsDisabled() && !s_key.IsRevoked() && !s_key.IsExpired();
      });
}

auto GpgKey::IsHasActualAuthCap() const -> bool {
  auto s_keys = SubKeys();
  return std::any_of(
      s_keys.begin(), s_keys.end(), [](const GpgSubKey &s_key) -> bool {
        return s_key.IsSecretKey() && s_key.IsHasAuthCap() &&
               !s_key.IsDisabled() && !s_key.IsRevoked() && !s_key.IsExpired();
      });
}

/**
 * check if key can certify(actually)
 * @param key target key
 * @return if key certify
 */
auto GpgKey::IsHasActualCertCap() const -> bool {
  return IsHasMasterKey() && !IsExpired() && !IsRevoked() && !IsDisabled();
}

/**
 * check if key can encrypt(actually)
 * @param key target key
 * @return if key encrypt
 */
auto GpgKey::IsHasActualEncrCap() const -> bool {
  auto s_keys = SubKeys();
  return std::any_of(s_keys.begin(), s_keys.end(),
                     [](const GpgSubKey &s_key) -> bool {
                       return s_key.IsHasEncrCap() && !s_key.IsDisabled() &&
                              !s_key.IsRevoked() && !s_key.IsExpired();
                     });
}

auto GpgKey::PrimaryKey() const -> GpgSubKey {
  if (km_ref_ != nullptr) return GpgSubKey(km_ref_, km_ref_->subkeys.front());
  return GpgSubKey(key_ref_, key_ref_->subkeys);
}

auto GpgKey::KeyType() const -> GpgAbstractKeyType {
  return GpgAbstractKeyType::kGPG_KEY;
}

GpgKey::GpgKey(const GFKeyMetadata &km_ref)
    : km_ref_(QSharedPointer<GFKeyMetadata>::create(km_ref)) {}
}  // namespace GpgFrontend