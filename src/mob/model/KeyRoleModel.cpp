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

#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "core/utils/GpgUtils.h"
#include "model/ChangeRelay.h"
#include "shell/models/GpgKeyTableModel.h"
#include "shell/present/KeyStatus.h"
#include "shell/rules/EncryptBlockReason.h"

namespace GpgFrontend::Mobile {

KeyRoleModel::KeyRoleModel(QObject* parent) : QIdentityProxyModel(parent) {
  connect(&LanguageRelay(), &ChangeRelay::SignalChanged, this, [this]() {
    if (rowCount() > 0) {
      emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
  });
}

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

auto KeyRoleModel::KeyAt(const QModelIndex& index) -> const GpgAbstractKey* {
  const auto* model = qobject_cast<const KeyRoleModel*>(index.model());
  if (model == nullptr) return nullptr;
  const auto source = model->mapToSource(index);
  if (!source.isValid()) return nullptr;
  const auto* item = static_cast<GpgKeyTableItem*>(source.internalPointer());
  return item != nullptr ? item->Key() : nullptr;
}

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
    case kUsableRole:
      return StatusFacts(key).value(QStringLiteral("usable"));
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
    case kExpiresRole: {
      // The engines give 0 for "never": no date, as describe() has it.
      const auto expires = key->ExpirationTime();
      return expires.isValid() && expires.toSecsSinceEpoch() > 0 ? expires
                                                                 : QDateTime();
    }
    case kAlgorithmRole:
      return key->Algo();
    case kSecretStateRole:
      return SecretState(key);
    case kCapabilitiesRole:
      return Capabilities(key);
    case kEncryptNoteRole:
      return Shell::EncryptBlockNote(Shell::CollectEncryptKeyFacts(key));
    case kSignNoteRole:
      return Shell::SignBlockNote(Shell::CollectSignKeyFacts(key));
    case kMembersRole: {
      // By its class, not its type alone: anything may say it is a group.
      const auto* group = dynamic_cast<const GpgKeyGroup*>(key);
      return group != nullptr ? group->KeyIds() : QStringList();
    }
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

auto KeyRoleModel::StatusFacts(const GpgAbstractKey* key) -> QVariantMap {
  if (key == nullptr) return {};
  const auto status = GetKeyStatus(key);
  return {
      {QStringLiteral("status"), StatusName(status)},
      {QStringLiteral("statusText"), DescribeKeyStatus(status)},
      // A key about to lapse is still a key to use.
      {QStringLiteral("usable"),
       status == GpgKeyStatus::kOk || status == GpgKeyStatus::kExpiringSoon},
  };
}

auto KeyRoleModel::SecretState(const GpgAbstractKey* key) -> QString {
  const auto* gpg_key = dynamic_cast<const GpgKey*>(key);
  if (gpg_key == nullptr) return {};
  if (!gpg_key->IsPrivateKey()) return QStringLiteral("none");
  if (!gpg_key->IsHasMasterKey()) return QStringLiteral("partial");
  const auto subkeys = gpg_key->SubKeys();
  // The primary is surfaced as the first of them.
  if (!subkeys.isEmpty() && subkeys.front().IsCardKey()) {
    return QStringLiteral("card");
  }
  return QStringLiteral("full");
}

auto KeyRoleModel::Capabilities(const GpgAbstractKey* key) -> QString {
  if (key == nullptr || key->KeyType() != GpgAbstractKeyType::kGPG_KEY) {
    return {};
  }
  // A key that cannot be used gives nothing, whatever its parts could do:
  // as GnuPG lists a key's overall capabilities only for a usable one.
  const auto status = GetKeyStatus(key);
  if (status != GpgKeyStatus::kOk && status != GpgKeyStatus::kExpiringSoon) {
    return {};
  }
  const auto* gpg_key = dynamic_cast<const GpgKey*>(key);
  // Anything else that says it is a key answers for itself.
  if (gpg_key == nullptr) return GetUsagesByAbstractKey(key);

  // What its usable parts -- the primary key first, then the subkeys -- can
  // do together, whoever holds their secrets: the key's own "can sign" and
  // "can certify" ask for the secret here too (what this device can do with
  // it), and a public key would then seem to sign nothing.
  bool certify = false;
  bool encrypt = false;
  bool sign = false;
  bool authenticate = false;
  for (const auto& part : gpg_key->SubKeys()) {
    if (part.IsRevoked() || part.IsExpired() || part.IsDisabled()) continue;
    certify = certify || part.IsHasCertCap();
    encrypt = encrypt || part.IsHasEncrCap();
    sign = sign || part.IsHasSignCap();
    authenticate = authenticate || part.IsHasAuthCap();
  }
  QString letters;
  if (certify) letters += QLatin1Char('C');
  if (encrypt) letters += QLatin1Char('E');
  if (sign) letters += QLatin1Char('S');
  if (authenticate) letters += QLatin1Char('A');
  return letters;
}

auto KeyRoleModel::MapOf(const QModelIndex& index) -> QVariantMap {
  QVariantMap map;
  if (!index.isValid()) return map;
  const auto names = index.model()->roleNames();
  for (auto it = names.begin(); it != names.end(); ++it) {
    if (it.key() < Qt::UserRole) continue;
    map.insert(QString::fromLatin1(it.value()), index.data(it.key()));
  }
  return map;
}

auto KeyRoleModel::keyOf(const QString& fingerprint) const -> QVariantMap {
  for (int row = 0; row < rowCount(); ++row) {
    const auto at = index(row, 0);
    if (at.data(kFingerprintRole)
            .toString()
            .compare(fingerprint, Qt::CaseInsensitive) == 0) {
      return MapOf(at);
    }
  }
  return {};
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
      {kMembersRole, "members"},
      {kSecretStateRole, "secretState"},
      {kCapabilitiesRole, "capabilities"},
      {kEncryptNoteRole, "encryptNote"},
      {kSignNoteRole, "signNote"},
  });
  return names;
}

}  // namespace GpgFrontend::Mobile
