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

#pragma once

#include <QtQml/qqmlregistration.h>

#include <QIdentityProxyModel>

#include "core/model/GpgAbstractKey.h"

namespace GpgFrontend::Mobile {

/**
 * @brief The core's key table as a list QML can read by role.
 *
 * A proxy over GpgKeyTableModel, the model the desktop's key list shows: no
 * second copy of the keys. Each row's key is read directly rather than from
 * the table's display text, so a column the desktop renames or reorders does
 * not change what a role says.
 */
class KeyRoleModel : public QIdentityProxyModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("provided by KeyListController")

 public:
  enum Role : int {
    kNameRole = Qt::UserRole + 100,
    kEmailRole,
    kCommentRole,
    kKeyIdRole,
    kFingerprintRole,
    kIsKeyRole,  ///< a key, not a key group
    kHasSecretRole,
    kCanEncryptRole,
    kCanSignRole,
    kUsableRole,  ///< not expired, revoked or disabled
    kExpiredRole,
    kRevokedRole,
    kCreatedRole,
    kExpiresRole,  ///< invalid QDateTime when the key does not expire
    kAlgorithmRole,
    kCheckedRole,
    /// The one condition worth showing, as the desktop's key table has it
    /// (GetKeyStatus): "ok", "expiring_soon", "expired", "revoked",
    /// "disabled".
    kStatusRole,
    /// The same, in words (DescribeKeyStatus).
    kStatusTextRole,
    /// The direct members of a key group (their key or group ids); empty
    /// for a key.
    kMembersRole,
    /// What of the secret key is here, as GnuPG's listing marks it:
    /// "none" (pub: public only), "full" (sec: the primary's secret),
    /// "partial" (sec#: secret subkeys only, the primary offline), "card"
    /// (sec>: the primary on a smart card); "" for a key group. Read from
    /// the key material, never from whose key it is.
    kSecretStateRole,
    /// What the key as a whole can do, in GnuPG's letters and order (C
    /// certify, E encrypt, S sign, A authenticate: "CESA", "CES", "ES"):
    /// what its usable primary key and subkeys give it together, those
    /// expired, revoked or disabled left out, whether or not their secrets
    /// are here. Empty for a key that cannot be used at all (its status
    /// says why) and for a key group.
    kCapabilitiesRole,
    /// Why it cannot be encrypted to, in a word or two
    /// (Shell::EncryptBlockNote): "Expired", "Has no keys"; empty when it
    /// can be.
    kEncryptNoteRole,
    /// Why it cannot sign (Shell::SignBlockNote); empty when it can.
    kSignNoteRole,
  };
  Q_ENUM(Role)

  /// Its words (a key's status) are translated as they are read: the
  /// language changing says them all again.
  explicit KeyRoleModel(QObject* parent = nullptr);

  /**
   * @brief The key or key group at @p index of a KeyRoleModel, or of a model
   * that maps onto one; nullptr anywhere else.
   */
  [[nodiscard]] static auto KeyAt(const QModelIndex& index)
      -> const GpgAbstractKey*;

  /// kSecretStateRole for @p key.
  [[nodiscard]] static auto SecretState(const GpgAbstractKey* key) -> QString;

  /// kCapabilitiesRole for @p key.
  [[nodiscard]] static auto Capabilities(const GpgAbstractKey* key) -> QString;

  /// kStatusRole, kStatusTextRole and kUsableRole for @p key, as
  /// {status, statusText, usable}: what the list says of it, said again
  /// where a key is read anew (its page, after a change).
  [[nodiscard]] static auto StatusFacts(const GpgAbstractKey* key)
      -> QVariantMap;

  [[nodiscard]] auto data(const QModelIndex& index, int role) const
      -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex& index, const QVariant& value,
                             int role) -> bool override;
  [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;
};

}  // namespace GpgFrontend::Mobile
