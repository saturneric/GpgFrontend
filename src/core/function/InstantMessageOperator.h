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
 * @brief Compact, instant-messaging-friendly wrapper for a binary OpenPGP
 * message.
 *
 * A normal ASCII-armored PGP message is verbose and multi-line: the
 * "-----BEGIN PGP MESSAGE-----" boilerplate, a version header, blank lines, a
 * CRC24 footer and a newline every 64 characters all get mangled or truncated
 * when pasted into a chat app. This codec instead takes the raw binary OpenPGP
 * message and emits it as a single line of Base58 (the canonical Bitcoin/IPFS
 * alphabet). It is deliberately purely alphanumeric and omits the ambiguous
 * 0 O I l: unlike Base64 there is no '-', '_', '+', '/' or '=', so no character
 * can be interpreted by a messenger as markdown (e.g. '_' italic, '-'/'~'
 * strikethrough), as a link, or as a soft-wrap / word break. The token stays
 * one unbreakable, copy-paste-safe word.
 *
 * Deliberately there is **no marker or prefix** — the output must not be
 * recognisable as coming from GpgFrontend. Before Base58 the bytes form a small
 * versioned envelope:
 *
 *     version(1) | book_id(4) | seed(16) | raw_message XOR keystream
 *
 * A "password book" (a shared 256-byte table) is used as key material, never
 * applied directly. On every encryption a fresh random 16-byte seed is drawn
 * and a keystream is derived as SHA-256(book ‖ seed ‖ counter); the raw OpenPGP
 * message is XORed with it. Because the book only ever enters through the hash
 * with a fresh seed, its structure never appears in the output and never
 * repeats across messages. The book is identified by the SHA-256 prefix
 * @c book_id so decrypt can select the right book, and the @c version byte lets
 * the format evolve. This is an obfuscation/format layer, NOT cryptographic
 * protection — real confidentiality is the OpenPGP encryption underneath.
 *
 * Detection re-derives the keystream from the referenced book and seed,
 * reverses the XOR, and confirms the recovered bytes begin with a public-key
 * (PKESK, tag 1) or symmetric (SKESK, tag 3) session-key packet — how every
 * encrypted OpenPGP message starts. The wrapped OpenPGP message authenticates
 * itself (MDC / AEAD), so a corrupted token simply fails to decrypt.
 */
class GF_CORE_EXPORT InstantMessageOperator {
 public:
  /// Metadata recovered from a token, for display in the status panel.
  struct ImMessageInfo {
    int version{};         ///< envelope version byte
    QByteArray book_id;    ///< password-book hash the token references
    int encoded_length{};  ///< number of Base58 characters in the token
    int payload_size{};    ///< decoded OpenPGP message size in bytes
  };

  /// Outcome of inspecting text for an instant-messaging token.
  enum class DetectStatus : uint8_t {
    kNOT_TOKEN,  ///< not an IM token; the caller should handle input normally
    kBOOK_MISMATCH,  ///< an IM token, but no matching book (wrong phrase/seed)
    kMALFORMED,  ///< an IM token with a known book, but the payload is invalid
    kOK,         ///< a valid IM token; @c out / @c info are populated
  };

  /**
   * @brief Wrap a binary OpenPGP message into a compact single-line token.
   *
   * @param binary_message the raw (non-armored) OpenPGP message bytes
   * @return the Base58 token, or an empty string if @p binary_message is empty
   */
  static auto Encode(const GFBuffer& binary_message) -> QString;

  /**
   * @brief Inspect @p text and classify it as (not) an instant-messaging token,
   * distinguishing a wrong-book/wrong-phrase token and a malformed one so the
   * caller can report a precise error instead of a generic decrypt failure.
   *
   * @param text buffer that may be a compact token (possibly with stray
   * whitespace/line breaks introduced by a messenger)
   * @param[out] out the decoded binary OpenPGP message on kOk
   * @param[out] info optional metadata about the token (populated on any status
   * except kNOT_TOKEN)
   */
  static auto Inspect(const QString& text, GFBuffer& out,
                      ImMessageInfo* info = nullptr) -> DetectStatus;

  /**
   * @brief Convenience wrapper over Inspect(): true iff a valid token was
   * decoded (status kOk).
   */
  static auto Detect(const QString& text, GFBuffer& out,
                     ImMessageInfo* info = nullptr) -> bool;

  /**
   * @brief Cheap probe: is @p text a decodable token?
   */
  static auto Contains(const QString& text) -> bool;

  /**
   * @brief The current envelope format version embedded in every token.
   */
  static auto FormatVersion() -> int;

  /**
   * @brief Identifier (SHA-256 prefix) of the active password book — the
   * phrase-derived book if one is configured, otherwise the shared default.
   * This is the hash a freshly encoded token references.
   */
  static auto ActiveBookId() -> QByteArray;
};

}  // namespace GpgFrontend
