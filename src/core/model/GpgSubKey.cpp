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
namespace GpgFrontend {

GpgSubKey::GpgSubKey() = default;

GpgSubKey::GpgSubKey(QSharedPointer<struct _gpgme_key> key_ref,
                     gpgme_subkey_t s_key)
    : key_ref_(std::move(key_ref)), s_key_ref_(s_key) {}

GpgSubKey::GpgSubKey(const GpgSubKey&) = default;

GpgSubKey::~GpgSubKey() = default;

auto GpgSubKey::operator=(const GpgSubKey&) -> GpgSubKey& = default;

auto GpgSubKey::ID() const -> QString { return s_key_ref_->keyid; }

auto GpgSubKey::Fingerprint() const -> QString { return s_key_ref_->fpr; }

auto GpgSubKey::PublicKeyAlgo() const -> QString {
  return gpgme_pubkey_algo_name(s_key_ref_->pubkey_algo);
}

auto GpgSubKey::Algo() const -> QString {
  auto* buffer = gpgme_pubkey_algo_string(s_key_ref_);
  auto algo = QString(buffer);
  gpgme_free(buffer);
  return algo.toUpper();
}

auto GpgSubKey::KeyLength() const -> unsigned int { return s_key_ref_->length; }

auto GpgSubKey::IsHasEncrCap() const -> bool { return s_key_ref_->can_encrypt; }

auto GpgSubKey::IsHasSignCap() const -> bool { return s_key_ref_->can_sign; }

auto GpgSubKey::IsHasCertCap() const -> bool { return s_key_ref_->can_certify; }

auto GpgSubKey::IsHasAuthCap() const -> bool {
  return s_key_ref_->can_authenticate;
}

auto GpgSubKey::IsPrivateKey() const -> bool { return s_key_ref_->secret; }

auto GpgSubKey::IsExpired() const -> bool { return s_key_ref_->expired; }

auto GpgSubKey::IsRevoked() const -> bool { return s_key_ref_->revoked; }

auto GpgSubKey::IsDisabled() const -> bool { return s_key_ref_->disabled; }

auto GpgSubKey::IsSecretKey() const -> bool { return s_key_ref_->secret; }

auto GpgSubKey::IsCardKey() const -> bool { return s_key_ref_->is_cardkey; }

auto GpgSubKey::CreationTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(s_key_ref_->timestamp);
}

auto GpgSubKey::ExpirationTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(s_key_ref_->expires);
}

auto GpgSubKey::IsADSK() const -> bool { return s_key_ref_->can_renc; }

auto GpgSubKey::SmartCardSerialNumber() const -> QString {
  return QString::fromLatin1(s_key_ref_->card_number);
}

auto GpgSubKey::KeyType() const -> GpgAbstractKeyType {
  return GpgAbstractKeyType::kGPG_SUBKEY;
}

auto GpgSubKey::IsGood() const -> bool { return s_key_ref_ != nullptr; }

auto GpgSubKey::Convert2GpgKey() const -> QSharedPointer<GpgKey> {
  return SecureCreateSharedObject<GpgKey>(key_ref_);
}

auto GpgSubKey::Name() const -> QString { return key_ref_->uids->name; }

auto GpgSubKey::Email() const -> QString { return key_ref_->uids->email; }

auto GpgSubKey::Comment() const -> QString { return key_ref_->uids->comment; }

}  // namespace GpgFrontend
