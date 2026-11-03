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
#include "core/utils/CommonUtils.h"
#include "core/utils/MemoryUtils.h"
#include "core/utils/RustUtils.h"
namespace GpgFrontend {

GpgSubKey::GpgSubKey() = default;

GpgSubKey::GpgSubKey(QSharedPointer<GFKeyMetadata> km_ref,
                     const GFSubKeyMetadata& s_key_meta)
    : km_ref_(std::move(km_ref)),
      skm_ref_(QSharedPointer<GFSubKeyMetadata>::create(s_key_meta)) {}

GpgSubKey::GpgSubKey(const GpgSubKey&) = default;

GpgSubKey::~GpgSubKey() = default;

auto GpgSubKey::operator=(const GpgSubKey&) -> GpgSubKey& = default;

auto GpgSubKey::ID() const -> QString { return skm_ref_->key_id; }

auto GpgSubKey::Fingerprint() const -> QString { return skm_ref_->fpr; }

auto GpgSubKey::Keygrip() const -> QString { return skm_ref_->keygrip; }

auto GpgSubKey::PublicKeyAlgo() const -> QString {
  if (!skm_ref_->algo_name.isEmpty()) return skm_ref_->algo_name;
  return GfrKeyAlgo2KeyAlgoName(static_cast<Rust::GfrKeyAlgo>(skm_ref_->algo));
}

auto GpgSubKey::Algo() const -> QString {
  if (!skm_ref_->algo_string.isEmpty()) return skm_ref_->algo_string.toUpper();
  return GfrKeyAlgo2KeyAlgoName(static_cast<Rust::GfrKeyAlgo>(skm_ref_->algo));
}

auto GpgSubKey::AlgoId() const -> QString {
  if (!skm_ref_->algo_string.isEmpty()) {
    return AlgoIdFromGnuPGName(skm_ref_->algo_string);
  }
  return GfrKeyAlgo2KeyAlgoId(static_cast<Rust::GfrKeyAlgo>(skm_ref_->algo));
}

auto GpgSubKey::AlgoIdFromGnuPGName(const QString& gnupg_name) -> QString {
  auto id = gnupg_name.toLower();
  if (id.startsWith("ky1024_")) id.replace(0, 6, "kyber1024");
  if (id == "cv448" || id.endsWith("_cv448")) {
    id.chop(5);
    id += "x448";
  }
  return id;
}

auto GpgSubKey::KeyLength() const -> unsigned int {
  return skm_ref_->key_length;
}

auto GpgSubKey::IsHasEncrCap() const -> bool { return skm_ref_->can_encrypt; }

auto GpgSubKey::IsHasSignCap() const -> bool { return skm_ref_->can_sign; }

auto GpgSubKey::IsHasCertCap() const -> bool { return skm_ref_->can_certify; }

auto GpgSubKey::IsHasAuthCap() const -> bool { return skm_ref_->can_auth; }

auto GpgSubKey::IsPrivateKey() const -> bool { return skm_ref_->has_secret; }

auto GpgSubKey::IsExpired() const -> bool {
  if (skm_ref_->is_expired.has_value()) return *skm_ref_->is_expired;
  return skm_ref_->expires_at != 0 &&
         QDateTime::fromSecsSinceEpoch(skm_ref_->expires_at) <
             CurrentDateTime();
}

auto GpgSubKey::IsRevoked() const -> bool { return skm_ref_->is_revoked; }

auto GpgSubKey::IsDisabled() const -> bool { return skm_ref_->is_disabled; }

auto GpgSubKey::IsSecretKey() const -> bool { return skm_ref_->has_secret; }

auto GpgSubKey::IsCardKey() const -> bool { return skm_ref_->is_card_key; }

auto GpgSubKey::CreationTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(skm_ref_->created_at);
}

auto GpgSubKey::ExpirationTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(skm_ref_->expires_at);
}

auto GpgSubKey::IsADSK() const -> bool { return skm_ref_->can_renc; }

[[nodiscard]] auto GpgSubKey::IsMarked() const -> bool {
  return skm_ref_->marked;
}

auto GpgSubKey::SmartCardSerialNumber() const -> QString {
  return skm_ref_->card_serial;
}

auto GpgSubKey::KeyType() const -> GpgAbstractKeyType {
  return GpgAbstractKeyType::kGPG_SUBKEY;
}

auto GpgSubKey::IsGood() const -> bool { return skm_ref_ != nullptr; }

auto GpgSubKey::Convert2GpgKey() const -> QSharedPointer<GpgKey> {
  if (km_ref_ == nullptr) return nullptr;
  return SecureCreateSharedObject<GpgKey>(*km_ref_);
}

auto GpgSubKey::Name() const -> QString {
  if (km_ref_ == nullptr || km_ref_->user_ids.empty()) return {};
  return km_ref_->user_ids.front().name;
}

auto GpgSubKey::Email() const -> QString {
  if (km_ref_ == nullptr || km_ref_->user_ids.empty()) return {};
  return km_ref_->user_ids.front().email;
}

auto GpgSubKey::Comment() const -> QString {
  if (km_ref_ == nullptr || km_ref_->user_ids.empty()) return {};
  return km_ref_->user_ids.front().comment;
}

}  // namespace GpgFrontend
