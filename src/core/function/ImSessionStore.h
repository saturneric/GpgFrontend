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

#include "core/function/DoubleRatchet.h"
#include "core/function/ImContactBundle.h"
#include "core/function/X25519.h"
#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

/// A local one-time prekey: id plus the full X25519 key pair.
struct ImLocalPrekey {
  quint32 id{};
  X25519KeyPair kp;
};

/// The device-local X25519 identity: long-term key, signed prekey, OPK pool.
struct ImLocalIdentity {
  X25519KeyPair ik;   ///< long-term identity key pair
  quint32 spk_id{};   ///< current signed-prekey id
  X25519KeyPair spk;  ///< current signed-prekey pair
  QVector<ImLocalPrekey> opks;  ///< remaining one-time prekeys
};

/**
 * @brief Persists the instant-messaging Double Ratchet state: the device's own
 * X25519 identity/prekeys, per-peer ratchet sessions, imported peer bundles, and
 * the session-id -> peer index.
 *
 * All blobs are stored through DataObjectOperator, which encrypts them at rest
 * with the application's secure key material. A recursive mutex serializes the
 * read-modify-write cycles so a send/receive never races itself.
 */
class GF_CORE_EXPORT ImSessionStore
    : public SingletonFunctionObject<ImSessionStore> {
 public:
  /// Number of one-time prekeys minted with a fresh identity.
  static constexpr int kOneTimePrekeyPool = 20;

  explicit ImSessionStore(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /// Load the local identity, lazily creating and persisting it on first use.
  auto LoadOrCreateIdentity() -> ImLocalIdentity;

  /// Key material for our published bundle (caller fills fpr + PGP signature).
  auto PublicBundleKeys() -> ImContactBundle;

  /// Return the signed-prekey pair for @p spk_id, or empty if it has rotated.
  auto GetSignedPrekeyById(quint32 spk_id) -> std::optional<X25519KeyPair>;

  /// Remove and return the one-time prekey @p opk_id (empty if already used).
  auto ConsumeOneTimePrekey(quint32 opk_id) -> std::optional<X25519KeyPair>;

  auto LoadSession(const QString& peer_fpr) -> std::optional<RatchetState>;
  auto SaveSession(const QString& peer_fpr, const RatchetState& st) -> bool;
  void DeleteSession(const QString& peer_fpr);

  // Loopback session — the *other* end held locally only when messaging
  // yourself (recipient is one of your own keys), so the sender and receiver
  // ratchets don't collide on the single self-fingerprint.
  auto LoadLoopbackSession(const QString& peer_fpr)
      -> std::optional<RatchetState>;
  auto SaveLoopbackSession(const QString& peer_fpr, const RatchetState& st)
      -> bool;

  /// Store an imported peer bundle (keyed by its pgp_fpr).
  auto SavePeerBundle(const ImContactBundle& bundle) -> bool;
  auto LoadPeerBundle(const QString& peer_fpr) -> std::optional<ImContactBundle>;

  void SetSessionPeer(const QByteArray& session_id, const QString& peer_fpr);
  auto SessionIdToPeer(const QByteArray& session_id) -> std::optional<QString>;

  /// The stable session id a sender reuses for every PFS_MSG to this peer.
  void SavePeerSessionId(const QString& peer_fpr, const QByteArray& session_id);
  auto GetPeerSessionId(const QString& peer_fpr) -> std::optional<QByteArray>;

  /// Convergence flag: record that a PFS token was received from @p peer_fpr,
  /// so we stop attaching our bundle to outgoing NORMAL messages.
  void MarkPfsConfirmed(const QString& peer_fpr);
  void ClearPfsConfirmed(const QString& peer_fpr);
  auto IsPfsConfirmed(const QString& peer_fpr) -> bool;

 private:
  QRecursiveMutex mutex_;

  auto CreateIdentity() -> ImLocalIdentity;
  auto PersistIdentity(const ImLocalIdentity& id) -> bool;
};

}  // namespace GpgFrontend
