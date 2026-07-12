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

/// A one-time prekey: a short id and its X25519 public key.
struct ImOneTimePrekey {
  quint32 id{};
  GFBuffer pub;  ///< 32-byte X25519 public key
};

/**
 * @brief An instant-messaging "prekey bundle" — the public half of a user's
 * X25519 identity, published so a peer can start a forward-secret session.
 *
 * Contains the long-term identity key (@c ik_pub), a signed prekey
 * (@c spk_pub / @c spk_id), an optional pool of one-time prekeys, the owner's
 * OpenPGP fingerprint, and a detached OpenPGP signature over everything else.
 * The signature binds the X25519 keys to a real PGP identity; it is produced and
 * verified by ImHandshake, so this class treats it as an opaque blob and only
 * handles (de)serialization.
 */
struct GF_CORE_EXPORT ImContactBundle {
  GFBuffer ik_pub;            ///< 32-byte identity public key
  quint32 spk_id{};          ///< signed-prekey id
  GFBuffer spk_pub;          ///< 32-byte signed-prekey public key
  QVector<ImOneTimePrekey> opks;  ///< one-time prekeys (may be empty)
  QString pgp_fpr;           ///< owner's OpenPGP fingerprint
  QByteArray pgp_sig;        ///< detached PGP signature over SignedPortion()

  /**
   * @brief The exact bytes the PGP signature covers: the whole bundle except
   * the signature field. Deterministic, so signer and verifier agree.
   */
  [[nodiscard]] auto SignedPortion() const -> GFBuffer;

  /**
   * @brief Serialize the complete bundle (including @c pgp_sig) to bytes.
   */
  [[nodiscard]] auto Serialize() const -> GFBuffer;

  /**
   * @brief Parse a bundle produced by Serialize().
   *
   * @param bytes serialized bundle
   * @return the bundle, or empty if the input is malformed
   */
  static auto Parse(const GFBuffer& bytes) -> std::optional<ImContactBundle>;
};

}  // namespace GpgFrontend
