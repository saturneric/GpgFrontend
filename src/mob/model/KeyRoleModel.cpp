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

#include "model/KeyRoleModel.h"

#include "core/model/GpgKeyTableModel.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Mobile {

namespace {

auto StatusName(GpgKeyStatus status) -> QString {
  switch (status) {
    case GpgKeyStatus::kOk:
      return QStringLiteral("ok");
    case GpgKeyStatus::kExpiringSoon:
      return QStringLiteral("expiring_soon");
    case GpgKeyStatus::kExpired:
      return QStringLiteral("expired");
    case GpgKeyStatus::kRevoked:
      return QStringLiteral("revoked");
    case GpgKeyStatus::kDisabled:
      return QStringLiteral("disabled");
  }
  return QStringLiteral("ok");
}

}  // namespace

auto KeyRoleModel::data(const QModelIndex& index, int role) const -> QVariant {
  if (role < kNameRole) return QIdentityProxyModel::data(index, role);

  const auto source = mapToSource(index);
  if (!source.isValid()) return {};

  if (role == kCheckedRole) {
    return sourceModel()->data(source.siblingAtColumn(0), Qt::CheckStateRole) ==
           Qt::Checked;
  }

  const auto* item = static_cast<GpgKeyTableItem*>(source.internalPointer());
  const auto* key = item != nullptr ? item->Key() : nullptr;
  if (key == nullptr) return {};

  switch (role) {
    case kNameRole:
      return key->Name();
    case kEmailRole:
      return key->Email();
    case kCommentRole:
      return key->Comment();
    case kKeyIdRole:
      return key->ID();
    case kFingerprintRole:
      return key->Fingerprint();
    case kIsKeyRole:
      return key->KeyType() == GpgAbstractKeyType::kGPG_KEY;
    case kHasSecretRole:
      return key->IsPrivateKey();
    case kCanEncryptRole:
      return key->IsHasEncrCap();
    case kCanSignRole:
      return key->IsHasSignCap();
    case kUsableRole: {
      // A key about to lapse is still a key to use.
      const auto status = GetKeyStatus(key);
      return status == GpgKeyStatus::kOk ||
             status == GpgKeyStatus::kExpiringSoon;
    }
    case kStatusRole:
      return StatusName(GetKeyStatus(key));
    case kStatusTextRole:
      return DescribeKeyStatus(GetKeyStatus(key));
    case kExpiredRole:
      return key->IsExpired();
    case kRevokedRole:
      return key->IsRevoked();
    case kCreatedRole:
      return key->CreationTime();
    case kExpiresRole:
      return key->ExpirationTime();
    case kAlgorithmRole:
      return key->Algo();
    default:
      return {};
  }
}

auto KeyRoleModel::setData(const QModelIndex& index, const QVariant& value,
                           int role) -> bool {
  if (role != kCheckedRole)
    return QIdentityProxyModel::setData(index, value, role);
  const auto source = mapToSource(index).siblingAtColumn(0);
  const bool changed = sourceModel()->setData(
      source, value.toBool() ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
  if (changed) emit dataChanged(index, index, {kCheckedRole});
  return changed;
}

auto KeyRoleModel::roleNames() const -> QHash<int, QByteArray> {
  auto names = QIdentityProxyModel::roleNames();
  names.insert({
      {kNameRole, "name"},
      {kEmailRole, "email"},
      {kCommentRole, "comment"},
      {kKeyIdRole, "keyId"},
      {kFingerprintRole, "fingerprint"},
      {kIsKeyRole, "isKey"},
      {kHasSecretRole, "hasSecret"},
      {kCanEncryptRole, "canEncrypt"},
      {kCanSignRole, "canSign"},
      {kUsableRole, "usable"},
      {kExpiredRole, "expired"},
      {kRevokedRole, "revoked"},
      {kCreatedRole, "created"},
      {kExpiresRole, "expires"},
      {kAlgorithmRole, "algorithm"},
      {kCheckedRole, "checked"},
      {kStatusRole, "status"},
      {kStatusTextRole, "statusText"},
  });
  return names;
}

}  // namespace GpgFrontend::Mobile
