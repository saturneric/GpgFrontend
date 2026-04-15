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
#include "GpgSubKey.h"

#include "core/model/GpgKey.h"
#include "core/utils/MemoryUtils.h"
#include "core/utils/RustUtils.h"
namespace GpgFrontend {

GpgSubKey::GpgSubKey() = default;

GpgSubKey::GpgSubKey(QSharedPointer<struct _gpgme_key> key_ref,
                     gpgme_subkey_t s_key)
    : key_ref_(std::move(key_ref)), s_key_ref_(s_key) {}

GpgSubKey::GpgSubKey(QSharedPointer<GFKeyMetadata> km_ref,
                     const GFSubKeyMetadata& s_key_meta)
    : km_ref_(std::move(km_ref)),
      skm_ref_(QSharedPointer<GFSubKeyMetadata>::create(s_key_meta)) {}

GpgSubKey::GpgSubKey(const GpgSubKey&) = default;

GpgSubKey::~GpgSubKey() = default;

auto GpgSubKey::operator=(const GpgSubKey&) -> GpgSubKey& = default;

auto GpgSubKey::ID() const -> QString {
  if (skm_ref_ != nullptr) return skm_ref_->key_id;
  return s_key_ref_->keyid;
}

auto GpgSubKey::Fingerprint() const -> QString {
  if (skm_ref_ != nullptr) return skm_ref_->fpr;
  return s_key_ref_->fpr;
}

auto GpgSubKey::PublicKeyAlgo() const -> QString {
  if (skm_ref_ != nullptr) {
    return GfrKeyAlgo2KeyAlgoName(
        static_cast<Rust::GfrKeyAlgo>(skm_ref_->algo));
  }
  return gpgme_pubkey_algo_name(s_key_ref_->pubkey_algo);
}

auto GpgSubKey::Algo() const -> QString {
  if (skm_ref_ != nullptr) {
    return GfrKeyAlgo2KeyAlgoName(
        static_cast<Rust::GfrKeyAlgo>(skm_ref_->algo));
  }
  auto* buffer = gpgme_pubkey_algo_string(s_key_ref_);
  auto algo = QString(buffer);
  gpgme_free(buffer);
  return algo.toUpper();
}

auto GpgSubKey::KeyLength() const -> unsigned int {
  if (skm_ref_ != nullptr) return 0;
  return s_key_ref_->length;
}

auto GpgSubKey::IsHasEncrCap() const -> bool {
  if (skm_ref_ != nullptr) return skm_ref_->can_encrypt;
  return s_key_ref_->can_encrypt;
}

auto GpgSubKey::IsHasSignCap() const -> bool {
  if (skm_ref_ != nullptr) return skm_ref_->can_sign;
  return s_key_ref_->can_sign;
}

auto GpgSubKey::IsHasCertCap() const -> bool {
  if (skm_ref_ != nullptr) return skm_ref_->can_certify;
  return s_key_ref_->can_certify;
}

auto GpgSubKey::IsHasAuthCap() const -> bool {
  if (skm_ref_ != nullptr) return skm_ref_->can_auth;
  return s_key_ref_->can_authenticate;
}

auto GpgSubKey::IsPrivateKey() const -> bool {
  if (skm_ref_ != nullptr) return skm_ref_->has_secret;
  return s_key_ref_->secret;
}

auto GpgSubKey::IsExpired() const -> bool {
  if (skm_ref_ != nullptr) return false;
  return s_key_ref_->expired;
}

auto GpgSubKey::IsRevoked() const -> bool {
  if (skm_ref_ != nullptr) return false;
  return s_key_ref_->revoked;
}

auto GpgSubKey::IsDisabled() const -> bool {
  if (skm_ref_ != nullptr) return false;
  return s_key_ref_->disabled;
}

auto GpgSubKey::IsSecretKey() const -> bool {
  if (skm_ref_ != nullptr) return skm_ref_->has_secret;
  return s_key_ref_->secret;
}

auto GpgSubKey::IsCardKey() const -> bool {
  if (skm_ref_ != nullptr) return false;
  return s_key_ref_->is_cardkey;
}

auto GpgSubKey::CreationTime() const -> QDateTime {
  if (skm_ref_ != nullptr) {
    return QDateTime::fromSecsSinceEpoch(skm_ref_->created_at);
  }
  return QDateTime::fromSecsSinceEpoch(s_key_ref_->timestamp);
}

auto GpgSubKey::ExpirationTime() const -> QDateTime {
  if (skm_ref_ != nullptr) return QDateTime::fromSecsSinceEpoch(0);
  return QDateTime::fromSecsSinceEpoch(s_key_ref_->expires);
}

auto GpgSubKey::IsADSK() const -> bool {
  if (skm_ref_ != nullptr) return false;
  return s_key_ref_->can_renc;
}

[[nodiscard]] auto GpgSubKey::IsMarked() const -> bool {
  if (skm_ref_ != nullptr) return skm_ref_->marked;
  return false;
}

auto GpgSubKey::SmartCardSerialNumber() const -> QString {
  if (skm_ref_ != nullptr) return {};
  return QString::fromLatin1(s_key_ref_->card_number);
}

auto GpgSubKey::KeyType() const -> GpgAbstractKeyType {
  return GpgAbstractKeyType::kGPG_SUBKEY;
}

auto GpgSubKey::IsGood() const -> bool {
  return s_key_ref_ != nullptr || skm_ref_ != nullptr;
}

auto GpgSubKey::Convert2GpgKey() const -> QSharedPointer<GpgKey> {
  return SecureCreateSharedObject<GpgKey>(key_ref_);
}

auto GpgSubKey::Name() const -> QString {
  if (km_ref_ != nullptr) return km_ref_->user_id.split('<').first().trimmed();
  return key_ref_->uids->name;
}

auto GpgSubKey::Email() const -> QString {
  if (km_ref_ != nullptr) {
    auto user_id = km_ref_->user_id;
    auto email = user_id.split('<').last().split('>').first().trimmed();
    return email;
  }
  return key_ref_->uids->email;
}

auto GpgSubKey::Comment() const -> QString {
  if (km_ref_ != nullptr) {
    auto user_id = km_ref_->user_id;
    auto comment = user_id.split('<').last().split('>').last().trimmed();
    return comment;
  }
  return key_ref_->uids->comment;
}

}  // namespace GpgFrontend
