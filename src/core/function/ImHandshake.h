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

#include "core/function/ImContactBundle.h"
#include "core/function/ImSessionStore.h"
#include "core/function/X25519.h"
#include "core/model/GpgKey.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief The X3DH-style session handshake for instant messaging: derives the
 * initial shared secret from X25519 prekeys and the shared password book, and
 * authenticates it with an OpenPGP detached signature over the transcript.
 *
 * Pure orchestration over X25519 + libsodium + the OpenPGP sign/verify engine;
 * ratchet stepping is DoubleRatchet's job and persistence is ImSessionStore's.
 */
class GF_CORE_EXPORT ImHandshake {
 public:
  /// The public fields an initiator must transmit in a PFS_INIT token.
  struct InitPayload {
    GFBuffer ik_pub;   ///< initiator identity public
    GFBuffer ek_pub;   ///< initiator ephemeral public
    quint32 spk_id{};  ///< responder signed-prekey id used
    quint32 opk_id{};  ///< responder one-time-prekey id used (0 = none)
  };

  /**
   * @brief Detached-sign @p data with the given OpenPGP signer key.
   *
   * @return the detached signature bytes, or empty on failure
   */
  static auto SignDetached(int channel, const GpgKeyPtr& signer,
                           const GFBuffer& data) -> std::optional<QByteArray>;

  /**
   * @brief Verify a detached signature over @p data.
   *
   * @return the (good) signer's OpenPGP fingerprint, or empty if the signature
   * is missing, bad, or not fully valid
   */
  static auto VerifyDetached(int channel, const GFBuffer& data,
                             const QByteArray& sig) -> std::optional<QString>;

  /**
   * @brief Initiator side: run X3DH against a peer bundle and start the ratchet.
   *
   * @param own the local identity (provides IK)
   * @param peer the peer's imported bundle (provides IK/SPK/optional OPK)
   * @param book the shared password-book bytes mixed into the secret
   * @param[out] payload the public init fields to transmit
   * @return the ready-to-send ratchet state, or empty on failure
   */
  static auto Initiate(const ImLocalIdentity& own, const ImContactBundle& peer,
                       const GFBuffer& book, InitPayload& payload)
      -> std::optional<RatchetState>;

  /**
   * @brief Responder side: reconstruct the X3DH secret and start the ratchet.
   *
   * @param own the local identity (provides IK)
   * @param own_spk the signed-prekey pair the initiator referenced
   * @param own_opk the one-time-prekey pair the initiator referenced (or none)
   * @param payload the received init fields
   * @param book the shared password-book bytes
   * @return the ready-to-receive ratchet state, or empty on failure
   */
  static auto Accept(const ImLocalIdentity& own, const X25519KeyPair& own_spk,
                     const std::optional<X25519KeyPair>& own_opk,
                     const InitPayload& payload, const GFBuffer& book)
      -> std::optional<RatchetState>;
};

}  // namespace GpgFrontend
