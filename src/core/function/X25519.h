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

#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief An X25519 (Curve25519) key pair used for ephemeral Diffie-Hellman.
 *
 * @c pub is the 32-byte public key; @c priv is the 32-byte secret scalar. Both
 * are held in GFBuffers so callers can Zeroize() them when done.
 */
struct X25519KeyPair {
  GFBuffer pub;   ///< 32-byte public key
  GFBuffer priv;  ///< 32-byte secret scalar
};

/**
 * @brief Thin wrapper over libsodium's Curve25519 scalar multiplication,
 * exposing raw X25519 Diffie-Hellman for the instant-messaging ratchet.
 *
 * OpenPGP already uses X25519 internally, but never exposes a callable DH; this
 * is the only new primitive the forward-secrecy machinery needs. All operations
 * require libsodium to be initialised and fail closed (return empty) otherwise.
 */
class GF_CORE_EXPORT X25519 {
 public:
  static constexpr int kPubKeyLen = 32;   ///< Curve25519 public-key length
  static constexpr int kPrivKeyLen = 32;  ///< Curve25519 scalar length
  static constexpr int kSharedLen = 32;   ///< DH shared-secret length

  /**
   * @brief Generate a fresh random X25519 key pair.
   *
   * @return the key pair, or empty on RNG / libsodium failure
   */
  static auto Generate() -> std::optional<X25519KeyPair>;

  /**
   * @brief Recompute the public key for a given secret scalar.
   *
   * @param priv 32-byte secret scalar
   * @return the 32-byte public key, or empty on failure
   */
  static auto DerivePublic(const GFBuffer& priv) -> GFBufferOrNone;

  /**
   * @brief Raw X25519 Diffie-Hellman.
   *
   * @param priv our 32-byte secret scalar
   * @param peer_pub the peer's 32-byte public key
   * @return the 32-byte shared secret, or empty on malformed input or if the
   * result is the all-zero point (small-subgroup / degenerate key)
   */
  static auto DH(const GFBuffer& priv, const GFBuffer& peer_pub)
      -> GFBufferOrNone;
};

}  // namespace GpgFrontend
