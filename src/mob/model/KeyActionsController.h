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

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace GpgFrontend::Mobile {

/**
 * @brief Make, export and delete keys, for the Keys pages.
 *
 * Each action runs on the GPG task runner and ends with SignalFinished(
 * action, succeeded, detail), action being "generate", "export" or "delete":
 *
 *  - generate: detail {fingerprint}; a new key's passphrase is asked through
 *    the PassphraseBroker.
 *  - export: detail {armored}, the public key as text.
 *  - delete: detail {}.
 *
 * A failed action's detail has {error}. After generate and delete the key
 * list wants a refresh; the page asks for it.
 */
class KeyActionsController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)
  /// What generate() can make here: [{name, description}], the recommended
  /// one first.
  Q_PROPERTY(QVariantList profiles READ Profiles NOTIFY SignalProfilesChanged)
  /// How long a key or subkey may last, as the desktop offers it:
  /// [{id ("1y", "forever", ...), name ("1 year", "Never")}].
  Q_PROPERTY(QVariantList validities READ Validities CONSTANT)

 public:
  explicit KeyActionsController(QObject* parent = nullptr);

  [[nodiscard]] auto Busy() const -> bool;
  [[nodiscard]] auto Profiles() const -> QVariantList;
  [[nodiscard]] auto Validities() const -> QVariantList;

  /**
   * @brief Generate a key for @p name and @p email with the profile at
   * @p profile in Profiles(); protected by a passphrase unless
   * @p with_passphrase is false.
   */
  Q_INVOKABLE void generate(const QString& name, const QString& email,
                            int profile, bool with_passphrase);

  /**
   * @brief Generate a key as chosen in Expert Mode: @p primary and, when
   * given, @p subkey are KeyGenProfilesController::InfoFromChoices()'s
   * choices; protected by a passphrase unless @p with_passphrase is false.
   * Action "generate", as generate()'s.
   */
  Q_INVOKABLE void generateCustom(const QString& name, const QString& email,
                                  const QVariantMap& primary,
                                  const QVariantMap& subkey,
                                  bool with_passphrase);

  /// Read the profiles again, after Settings changed them.
  Q_INVOKABLE void reloadProfiles();

  /// The public key @p fingerprint, armored.
  Q_INVOKABLE void exportPublic(const QString& fingerprint);

  /// Delete the key @p fingerprint, its secret part too.
  Q_INVOKABLE void remove(const QString& fingerprint);

  /**
   * @brief Everything the key details show of the key @p fingerprint, as the
   * desktop's details dialog does; empty when there is no such key.
   *
   * {name, email, comment, keyId, fingerprint, version, algorithm,
   * algorithmDetail, keySize, usage {certify, encrypt, sign, authenticate},
   * created, expires (invalid: never), lastUpdate (invalid: not known),
   * hasSecret, primarySecret (the primary key's secret part is here),
   * cardKey, expired, revoked, disabled, uids [{uid, name, email, comment,
   * primary, revoked, invalid}], subkeys [{index, primary, keyId,
   * fingerprint, algorithm, algorithmDetail, keySize, usage, created,
   * expires, secret, card, expired, revoked, disabled, adsk}], can
   * {addSubkey, setExpiration, revokeSubkey, deleteSubkey,
   * changePassphrase, addUid, setPrimaryUid, revokeUid, deleteUid}}. The
   * primary key is subkeys[0], as the engines list it; "can" says what this
   * engine does and this key allows.
   */
  [[nodiscard]] Q_INVOKABLE QVariantMap
  describe(const QString& fingerprint) const;

  /**
   * @brief Add a subkey to the key @p fingerprint for @p purpose ("encrypt",
   * "sign" or "authenticate"), of the algorithm the desktop suggests for it,
   * lasting @p validity (an id of Validities()); protected by a passphrase
   * unless @p with_passphrase is false. Action "add_subkey".
   */
  Q_INVOKABLE void addSubkey(const QString& fingerprint, const QString& purpose,
                             const QString& validity, bool with_passphrase);

  /// Add a subkey to the key @p fingerprint as chosen in Expert Mode
  /// (KeyGenProfilesController::InfoFromChoices()). Action "add_subkey".
  Q_INVOKABLE void addCustomSubkey(const QString& fingerprint,
                                   const QVariantMap& choices,
                                   bool with_passphrase);

  /// Make the primary key of @p fingerprint (@p subkey empty), or its subkey
  /// @p subkey (a fingerprint), last @p validity from now (an id of
  /// Validities()). Action "expiration".
  Q_INVOKABLE void setExpiration(const QString& fingerprint,
                                 const QString& subkey,
                                 const QString& validity);

  /// Revoke the subkey @p subkey of @p fingerprint for @p reason (0 none, 1
  /// compromised, 2 superseded, 3 no longer used). Action "revoke_subkey".
  Q_INVOKABLE void revokeSubkey(const QString& fingerprint,
                                const QString& subkey, int reason,
                                const QString& text);

  /// Delete the subkey @p subkey of @p fingerprint. Action "delete_subkey".
  Q_INVOKABLE void deleteSubkey(const QString& fingerprint,
                                const QString& subkey);

  /// Add the user ID @p name <@p email> (@p comment) to the key
  /// @p fingerprint. Action "add_uid".
  Q_INVOKABLE void addUid(const QString& fingerprint, const QString& name,
                          const QString& email, const QString& comment);

  /// Make @p uid (as describe() lists it) the key's primary user ID.
  /// Action "primary_uid".
  Q_INVOKABLE void setPrimaryUid(const QString& fingerprint,
                                 const QString& uid);

  /// Revoke the user ID @p uid for @p reason (0 none, 1 compromised, 2
  /// superseded, 3 no longer valid). Action "revoke_uid".
  Q_INVOKABLE void revokeUid(const QString& fingerprint, const QString& uid,
                             int reason, const QString& text);

  /// Delete the user ID @p uid. Action "delete_uid".
  Q_INVOKABLE void deleteUid(const QString& fingerprint, const QString& uid);

  /// Change the passphrase of the key @p fingerprint, asked for in turn.
  /// Action "passphrase".
  Q_INVOKABLE void changePassphrase(const QString& fingerprint);

 signals:
  void SignalBusyChanged();
  void SignalProfilesChanged();
  void SignalFinished(const QString& action, bool succeeded,
                      const QVariantMap& detail);

 private:
  bool busy_ = false;

  auto begin() -> bool;
  void run(const QString& action, std::function<QVariantMap()> work);
};

}  // namespace GpgFrontend::Mobile
