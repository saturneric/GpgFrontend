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

#include <QObject>
#include <QSharedPointer>
#include <QVariantList>
#include <QVariantMap>
#include <optional>

#include "shell/rules/KeyGenProfile.h"

namespace GpgFrontend::Mobile {

/**
 * @brief The key generation profiles, as the desktop's Settings edit them,
 * and the choices an expert key or subkey is made of.
 *
 * The profiles are the profile's own (kKeyGenSettingsObject), shared with the
 * desktop: what is changed here is what the desktop offers, and the other
 * way round. A profile the engine cannot make is kept and listed, marked
 * unavailable, never rewritten.
 *
 * An algorithm is named by its id and type, as a profile stores it; QML
 * passes it as {id, type}.
 */
class KeyGenProfilesController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  /**
   * [{index, name, storedName, description, algorithms, available, hidden,
   * isDefault, primary {id, type}, primarySub, validity, hasSubkey, subkey,
   * subkeySub, subkeyValidity}], in stored order.
   */
  Q_PROPERTY(QVariantList profiles READ Profiles NOTIFY SignalProfilesChanged)
  /// What can be made as a primary key here: [{id, type, name,
  /// postQuantum, canEncrypt, canSign, canAuthenticate}].
  Q_PROPERTY(QVariantList primaryAlgos READ PrimaryAlgos CONSTANT)
  /// What can be made as a subkey here, as primaryAlgos.
  Q_PROPERTY(QVariantList subkeyAlgos READ SubkeyAlgos CONSTANT)
  /// Whether key and subkey creation open in Expert Mode; remembered, and
  /// the desktop's dialogs follow the same switch.
  Q_PROPERTY(bool expertMode READ ExpertMode WRITE SetExpertMode NOTIFY
                 SignalExpertModeChanged)

 public:
  explicit KeyGenProfilesController(QObject* parent = nullptr);

  [[nodiscard]] auto Profiles() const -> QVariantList;
  [[nodiscard]] auto PrimaryAlgos() const -> QVariantList;
  [[nodiscard]] auto SubkeyAlgos() const -> QVariantList;
  [[nodiscard]] auto ExpertMode() const -> bool;
  void SetExpertMode(bool on);

  /// What a subkey of the key @p fingerprint can be made of.
  [[nodiscard]] Q_INVOKABLE QVariantList
  subkeyAlgosFor(const QString& fingerprint) const;

  /// What the hybrid @p algo ({id, type}) pairs with; empty for one that is
  /// not a hybrid.
  [[nodiscard]] Q_INVOKABLE QVariantList
  pairings(const QVariantMap& algo) const;

  /**
   * @brief What a key of @p algo may be used for: {encrypt, sign,
   * authenticate} on by default, and {changeEncrypt, changeSign,
   * changeAuthenticate} whether each can be switched.
   */
  [[nodiscard]] Q_INVOKABLE QVariantMap usage(const QVariantMap& algo,
                                              bool subkey) const;

  /// The subkey algorithm suggested under a primary key of @p primary for
  /// @p purpose ("encrypt", "sign", "authenticate"): {id, type}.
  [[nodiscard]] Q_INVOKABLE QVariantMap
  suggestSubkey(const QVariantMap& primary, const QString& purpose) const;

  /**
   * @brief Store @p profile (fields as Profiles() lists them, without the
   * shown ones) at @p index, or as a new one when @p index is -1.
   * @return why it was not stored ("" when it was)
   */
  /// A made-up user id for a key nobody needs to recognise, from the
  /// pattern kept with the key generation settings (the desktop's "Fill In
  /// Anonymously"): {name, email, comment}.
  [[nodiscard]] Q_INVOKABLE QVariantMap anonymousUid() const;

  Q_INVOKABLE QString save(int index, const QVariantMap& profile);

  /// Remove the profile at @p index; false when it is the default, which
  /// stays until another is chosen.
  Q_INVOKABLE bool remove(int index);

  /// Move the profile at @p from to @p to: the order Create a key lists them.
  Q_INVOKABLE void move(int from, int to);

  /// Make the profile at @p index the one Quick Generate uses.
  Q_INVOKABLE void setDefault(int index);

  /// Add back the built-in profiles that are missing; nothing is removed.
  Q_INVOKABLE void restoreDefaults();

  /// Read the profiles again (another page may have changed them).
  Q_INVOKABLE void reload();

  /**
   * @brief The parameters of one key or subkey from the expert choices
   * @p choices: {algo {id, type}, pair {id, type} (a hybrid's second half),
   * usage {encrypt, sign, authenticate}, validity}, from @p algos.
   * A usage the algorithm does not let change keeps its default.
   * @return the parameters, or null when the algorithm is not among @p algos
   */
  static auto InfoFromChoices(const QVariantMap& choices, bool subkey,
                              const QList<KeyAlgo>& algos, int channel)
      -> QSharedPointer<KeyGenerateInfo>;

 signals:
  void SignalProfilesChanged();
  void SignalExpertModeChanged();

 private:
  /// Write @p settings and tell the pages. Every change reads the stored
  /// settings first, so no page writes back a copy older than another's.
  void store(const KeyGenSO& settings);
};

}  // namespace GpgFrontend::Mobile
