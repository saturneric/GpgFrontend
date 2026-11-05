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

#include "shell/present/KeyStatus.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "core/model/SettingsObject.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto IsKeyNeverExpires(const GpgAbstractKey* key) -> bool {
  if (key == nullptr) return true;
  return key->ExpirationTime().toSecsSinceEpoch() == 0;
}

auto GetKeyExpiringSoonDays() -> int {
  auto days = SettingItems::kExpiringSoonDays.Read(GetSettings());
  return std::clamp(days, 1, 365);
}

auto IsBuiltinKeyCategory(const QString& id) -> bool {
  static const QSet<QString> kBuiltin = {
      u"all"_s,
      u"key_group"_s,
      u"only_public_key"_s,
      u"has_private_key"_s,
      u"expiring_soon"_s,
      u"no_primary_key"_s,
      u"revoked"_s,
      u"expired"_s,
      u"disabled"_s,
  };
  return kBuiltin.contains(id);
}

auto MatchesBuiltinKeyCategory(const QString& id, const GpgAbstractKey* key)
    -> std::optional<bool> {
  if (key == nullptr || !IsBuiltinKeyCategory(id)) return std::nullopt;
  const auto lapsed = key->IsRevoked() || key->IsDisabled() || key->IsExpired();
  if (id == u"all"_s) return true;
  if (id == u"key_group"_s) {
    return key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP;
  }
  if (id == u"only_public_key"_s) return !key->IsPrivateKey() && !lapsed;
  if (id == u"has_private_key"_s) return key->IsPrivateKey() && !lapsed;
  if (id == u"expiring_soon"_s) return IsKeyExpiringSoon(key);
  if (id == u"no_primary_key"_s) {
    if (key->KeyType() != GpgAbstractKeyType::kGPG_KEY) return false;
    return !dynamic_cast<const GpgKey*>(key)->IsHasMasterKey() && !lapsed;
  }
  if (id == u"revoked"_s) return key->IsRevoked();
  if (id == u"expired"_s) return key->IsExpired();
  if (id == u"disabled"_s) return key->IsDisabled();
  return std::nullopt;
}

auto IsKeyExpiringSoon(const GpgAbstractKey* key) -> bool {
  if (key == nullptr) return false;
  if (key->IsRevoked() || key->IsDisabled() || key->IsExpired()) return false;
  if (IsKeyNeverExpires(key)) return false;

  const auto now = CurrentDateTime();
  const auto expires = key->ExpirationTime();
  return expires > now && now.daysTo(expires) <= GetKeyExpiringSoonDays();
}

auto ClassifyKeyStatus(bool revoked, bool disabled, bool expired,
                       bool expiring_soon) -> GpgKeyStatus {
  // Order matters and is the whole point of this function: a key can be
  // several of these at once, and the row tint has always shown disabled
  // first, then expired-or-revoked, then expiring-soon.
  if (disabled) return GpgKeyStatus::kDisabled;
  if (revoked) return GpgKeyStatus::kRevoked;
  if (expired) return GpgKeyStatus::kExpired;
  if (expiring_soon) return GpgKeyStatus::kExpiringSoon;
  return GpgKeyStatus::kOk;
}

auto KeyStatusSortRank(GpgKeyStatus status) -> int {
  switch (status) {
    case GpgKeyStatus::kOk:
      return 0;
    case GpgKeyStatus::kExpiringSoon:
      return 1;
    case GpgKeyStatus::kExpired:
      return 2;
    case GpgKeyStatus::kRevoked:
      return 3;
    case GpgKeyStatus::kDisabled:
      return 4;
  }
  return 0;
}

auto DescribeKeyStatus(GpgKeyStatus status) -> QString {
  switch (status) {
    case GpgKeyStatus::kOk:
      return QCoreApplication::translate("GpgFrontend", "OK");
    case GpgKeyStatus::kExpiringSoon:
      return QCoreApplication::translate("GpgFrontend", "Expiring Soon");
    case GpgKeyStatus::kExpired:
      return QCoreApplication::translate("GpgFrontend", "Expired");
    case GpgKeyStatus::kRevoked:
      return QCoreApplication::translate("GpgFrontend", "Revoked");
    case GpgKeyStatus::kDisabled:
      return QCoreApplication::translate("GpgFrontend", "Disabled");
  }
  return {};
}

auto AggregateOwnerTrustRank(const QList<int>& levels) -> int {
  if (levels.isEmpty()) return -1;

  const auto first = levels.front();
  for (const auto level : levels) {
    if (level != first) return -1;
  }
  return first;
}

auto GetKeyStatus(const GpgAbstractKey* key) -> GpgKeyStatus {
  if (key == nullptr) return GpgKeyStatus::kOk;
  return ClassifyKeyStatus(key->IsRevoked(), key->IsDisabled(),
                           key->IsExpired(), IsKeyExpiringSoon(key));
}

}  // namespace GpgFrontend
