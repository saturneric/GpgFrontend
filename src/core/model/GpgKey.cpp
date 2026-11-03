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

#include "core/utils/CommonUtils.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {

GpgKey::GpgKey() = default;

GpgKey::GpgKey(const GpgKey &) = default;

GpgKey::~GpgKey() = default;

auto GpgKey::operator=(const GpgKey &) -> GpgKey & = default;

auto GpgKey::IsGood() const -> bool { return km_ref_ != nullptr; }

auto GpgKey::Native() const -> std::shared_ptr<void> {
  return km_ref_ != nullptr ? km_ref_->native : nullptr;
}

auto GpgKey::ID() const -> QString { return km_ref_->key_id; }

auto GpgKey::Name() const -> QString {
  return km_ref_->user_ids.empty() ? QString{} : km_ref_->user_ids.front().name;
}

auto GpgKey::Email() const -> QString {
  return km_ref_->user_ids.empty() ? QString{}
                                   : km_ref_->user_ids.front().email;
}

auto GpgKey::Comment() const -> QString {
  return km_ref_->user_ids.empty() ? QString{}
                                   : km_ref_->user_ids.front().comment;
}

auto GpgKey::Fingerprint() const -> QString { return km_ref_->fpr; }

auto GpgKey::Protocol() const -> QString { return km_ref_->protocol; }

auto GpgKey::OwnerTrust() const -> QString {
  switch (km_ref_->owner_trust) {
    case GpgValidity::kUNKNOWN:
      return tr("Unknown");
    case GpgValidity::kUNDEFINED:
      return tr("Undefined");
    case GpgValidity::kNEVER:
      return tr("Never");
    case GpgValidity::kMARGINAL:
      return tr("Marginal");
    case GpgValidity::kFULL:
      return tr("Full");
    case GpgValidity::kULTIMATE:
      return tr("Ultimate");
  }
  return "Invalid";
}

auto GpgKey::OwnerTrustLevel() const -> int {
  switch (km_ref_->owner_trust) {
    case GpgValidity::kUNKNOWN:
      return 0;
    case GpgValidity::kUNDEFINED:
      return 1;
    case GpgValidity::kNEVER:
      return 2;
    case GpgValidity::kMARGINAL:
      return 3;
    case GpgValidity::kFULL:
      return 4;
    case GpgValidity::kULTIMATE:
      return 5;
  }
  return 0;
}

auto GpgKey::PublicKeyAlgo() const -> QString {
  if (!km_ref_->subkeys.empty() &&
      !km_ref_->subkeys.front().algo_name.isEmpty()) {
    return km_ref_->subkeys.front().algo_name;
  }
  return GfrKeyAlgo2KeyAlgoName(static_cast<Rust::GfrKeyAlgo>(km_ref_->algo));
}

auto GpgKey::Algo() const -> QString {
  if (!km_ref_->subkeys.empty() &&
      !km_ref_->subkeys.front().algo_string.isEmpty()) {
    return km_ref_->subkeys.front().algo_string.toUpper();
  }
  return GfrKeyAlgo2KeyAlgoName(static_cast<Rust::GfrKeyAlgo>(km_ref_->algo));
}

auto GpgKey::LastUpdateTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(km_ref_->update_time);
}

auto GpgKey::ExpirationTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(km_ref_->expires_at);
}

auto GpgKey::CreationTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(km_ref_->created_at);
}

auto GpgKey::PrimaryKeyLength() const -> unsigned int {
  return km_ref_->key_length;
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

auto GpgKey::IsPrivateKey() const -> bool { return km_ref_->has_secret; }

auto GpgKey::IsExpired() const -> bool {
  if (km_ref_->is_expired.has_value()) return *km_ref_->is_expired;
  return km_ref_->expires_at != 0 &&
         QDateTime::fromSecsSinceEpoch(km_ref_->expires_at) < CurrentDateTime();
}

auto GpgKey::IsRevoked() const -> bool { return km_ref_->is_revoked; }

auto GpgKey::IsDisabled() const -> bool { return km_ref_->is_disabled; }

auto GpgKey::IsHasMasterKey() const -> bool {
  // The primary key's own secret: a key whose primary is offline still has
  // secret subkeys.
  if (!km_ref_->subkeys.empty()) return km_ref_->subkeys.front().has_secret;
  return km_ref_->has_secret;
}

auto GpgKey::SubKeys() const -> QList<GpgSubKey> {
  QList<GpgSubKey> ret;
  for (const auto &sub : km_ref_->subkeys) {
    ret.push_back(GpgSubKey(km_ref_, sub));
  }
  return ret;
}

auto GpgKey::UIDs() const -> QList<GpgUID> {
  QList<GpgUID> uids;
  for (const auto &uid : km_ref_->user_ids) {
    uids.push_back(GpgUID(km_ref_, uid));
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

auto GpgKey::IsHasActualCertCap() const -> bool {
  return IsHasMasterKey() && !IsExpired() && !IsRevoked() && !IsDisabled();
}

auto GpgKey::IsHasActualEncrCap() const -> bool {
  auto s_keys = SubKeys();
  return std::any_of(s_keys.begin(), s_keys.end(),
                     [](const GpgSubKey &s_key) -> bool {
                       return s_key.IsHasEncrCap() && !s_key.IsDisabled() &&
                              !s_key.IsRevoked() && !s_key.IsExpired();
                     });
}

auto GpgKey::PrimaryKey() const -> GpgSubKey {
  if (km_ref_->subkeys.empty()) return {};
  return GpgSubKey(km_ref_, km_ref_->subkeys.front());
}

[[nodiscard]] auto GpgKey::KeyVersion() const -> int {
  // 0 when the engine does not say, as GnuPG does not.
  return km_ref_->key_ver;
}

auto GpgKey::KeyType() const -> GpgAbstractKeyType {
  return GpgAbstractKeyType::kGPG_KEY;
}

GpgKey::GpgKey(const GFKeyMetadata &km_ref)
    : km_ref_(QSharedPointer<GFKeyMetadata>::create(km_ref)) {}
}  // namespace GpgFrontend
