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

#include "core/function/X25519.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Per-message ratchet header, transmitted (authenticated) alongside the
 * ciphertext.
 */
struct RatchetHeader {
  GFBuffer dh_pub;  ///< sender's current ratchet public key (32 bytes)
  quint32 pn{};     ///< number of messages in the previous sending chain
  quint32 n{};      ///< message index in the current sending chain
};

/**
 * @brief Full Double Ratchet session state for one conversation.
 *
 * Holds the asymmetric ratchet key pair, the peer's latest ratchet public key,
 * the root key, the sending/receiving chain keys, the message counters, and the
 * store of message keys skipped by out-of-order delivery.
 */
struct RatchetState {
  X25519KeyPair dhs;  ///< our current ratchet key pair (sending)
  GFBuffer dhr_pub;   ///< peer's latest ratchet public key (empty until first recv)
  GFBuffer rk;        ///< 32-byte root key
  GFBuffer cks;       ///< sending chain key (empty until first send after a recv)
  GFBuffer ckr;       ///< receiving chain key (empty until first recv)
  quint32 ns{};       ///< messages sent in the current sending chain
  quint32 nr{};       ///< messages received in the current receiving chain
  quint32 pn{};       ///< length of the previous sending chain
  QMap<QPair<QByteArray, quint32>, GFBuffer>
      skipped;  ///< (ratchet_pub, n) -> message key, for out-of-order delivery
};

/**
 * @brief A Signal-style Double Ratchet: X25519 DH ratchet + symmetric chains,
 * giving forward secrecy and post-compromise (future) secrecy.
 *
 * Pure and stateless-of-its-own: all state lives in the RatchetState the caller
 * owns and persists. No Qt settings, no disk, no OpenPGP. The initial shared
 * secret (@c sk) is supplied by the X3DH handshake; confidentiality of each
 * message comes solely from the ratchet's XChaCha20-Poly1305 AEAD.
 */
class GF_CORE_EXPORT DoubleRatchet {
 public:
  /// Cap on message keys skipped in one step; a larger gap is rejected.
  static constexpr quint32 kMaxSkip = 1000;

  /// Serialized ratchet-header length: dh_pub(32) | pn(4 BE) | n(4 BE).
  static constexpr int kHeaderLen = X25519::kPubKeyLen + 4 + 4;

  /**
   * @brief Initialise the initiator ("Alice") side after X3DH.
   *
   * @param sk the 32-byte shared secret from the handshake
   * @param peer_ratchet_pub the responder's signed-prekey public (first DHr)
   * @return the ready-to-send state, or empty on failure
   */
  static auto InitSender(const GFBuffer& sk, const GFBuffer& peer_ratchet_pub)
      -> std::optional<RatchetState>;

  /**
   * @brief Initialise the responder ("Bob") side after X3DH.
   *
   * @param sk the 32-byte shared secret from the handshake
   * @param own_ratchet_kp the signed-prekey pair used as the first ratchet key
   * @return the ready-to-receive state
   */
  static auto InitReceiver(const GFBuffer& sk,
                           const X25519KeyPair& own_ratchet_kp) -> RatchetState;

  /**
   * @brief Ratchet-encrypt a message, advancing the sending chain.
   *
   * @param st the session state (mutated)
   * @param plaintext the message to encrypt
   * @param ad outer associated data bound into the AEAD (e.g. wire header)
   * @param[out] out_header the ratchet header to transmit
   * @return ciphertext‖tag, or empty on failure
   */
  static auto Encrypt(RatchetState& st, const GFBuffer& plaintext,
                      const GFBuffer& ad, RatchetHeader& out_header)
      -> GFBufferOrNone;

  /**
   * @brief Ratchet-decrypt a message, performing a DH step and harvesting
   * skipped keys as needed.
   *
   * The state is only mutated if decryption succeeds, so a forged or corrupted
   * token cannot desynchronise the session. Returns empty if the message key is
   * gone (already consumed / beyond kMaxSkip) or authentication fails.
   *
   * @param st the session state (mutated only on success)
   * @param header the received ratchet header
   * @param ciphertext the received ciphertext‖tag
   * @param ad outer associated data bound into the AEAD (must match sender)
   * @return the recovered plaintext, or empty
   */
  static auto Decrypt(RatchetState& st, const RatchetHeader& header,
                      const GFBuffer& ciphertext, const GFBuffer& ad)
      -> GFBufferOrNone;

  /**
   * @brief Serialize a ratchet header to its wire bytes (dh_pub | pn | n).
   */
  static auto SerializeHeader(const RatchetHeader& header) -> GFBuffer;

  /**
   * @brief Parse a ratchet header from @p kHeaderLen wire bytes.
   */
  static auto ParseHeader(const GFBuffer& bytes) -> std::optional<RatchetHeader>;

  /**
   * @brief Serialize the full session state for encrypted-at-rest persistence.
   */
  static auto SerializeState(const RatchetState& st) -> GFBuffer;

  /**
   * @brief Restore a session state produced by SerializeState().
   */
  static auto DeserializeState(const GFBuffer& bytes)
      -> std::optional<RatchetState>;
};

}  // namespace GpgFrontend
