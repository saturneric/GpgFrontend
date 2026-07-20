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
 * @brief Compact, instant-messaging-friendly container for an OpenPGP message,
 * emitted as a single Base58 word so it survives pasting into any chat app.
 *
 * A normal ASCII-armored PGP message is verbose and multi-line; pasted into a
 * messenger its boilerplate, line-wraps and CRC footer get mangled. This codec
 * emits one unbreakable Base58 word (the Bitcoin/IPFS alphabet, no ambiguous
 * 0 O I l and nothing a messenger turns into markdown, a link or a soft-wrap).
 *
 * There is deliberately no marker, prefix or magic on the wire: the whole
 * container is "whitened" with the shared password book. A per-message random
 * seed and the book derive — via a memory-hard KDF — a set of secret bytes;
 * part of them re-shuffles the byte order, the rest XORs the payload, and a
 * secret-controlled amount of random padding is interleaved to hide the true
 * length. The GpgFrontend recognition tag is a secret check value derived from
 * the book (not a fixed constant), so there is no known-plaintext to search
 * for; it lives inside this whitened blob, invisible on the wire and revealed
 * only after a receiver un-whitens with the same book.
 *
 * The consequence: without the book, a token is indistinguishable from random
 * Base58. Even *detecting* whether a token is one of ours costs the adversary a
 * memory-hard derivation per candidate book, since there is no cheap
 * pre-filter.
 *
 * The shared "password book" is a 256-byte table derived from an optional
 * phrase (Argon2id) or a fixed default. The empty-phrase default only hides the
 * format from naive scanners; real indistinguishability against a GpgFrontend-
 * aware adversary requires both sides to share a secret phrase.
 */
class GF_CORE_EXPORT InstantMessageOperator {
 public:
  /// The result of decoding a token.
  struct DecodeResult {
    bool ok{false};  ///< the token was one of ours and un-whitened cleanly
    GFBuffer pgp_message;  ///< the wrapped OpenPGP message to decrypt
  };

  /**
   * @brief Wrap an already-OpenPGP-encrypted message as one whitened Base58
   * token.
   *
   * @param pgp_message the binary OpenPGP message to wrap
   * @return the Base58 token, or empty on failure
   */
  static auto Encode(const GFBuffer& pgp_message) -> QString;

  /**
   * @brief Un-whiten @p text with the active password book and recover the
   * wrapped OpenPGP message. @c ok is false when @p text is not one of our
   * tokens (plain chat, or a different book).
   */
  static auto Decode(const QString& text) -> DecodeResult;

  /**
   * @brief Cheap probe: does @p text un-whiten to one of our tokens at all?
   */
  static auto Contains(const QString& text) -> bool;

  /**
   * @brief The current container format version (the inner tag's version).
   */
  static auto FormatVersion() -> int;

  /**
   * @brief Whether a shared book phrase is configured. Without one the default
   * book is used, which only hides the format from naive scanners.
   */
  static auto BookConfigured() -> bool;
};

}  // namespace GpgFrontend
