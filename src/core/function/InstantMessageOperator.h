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
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Compact, instant-messaging-friendly container for a secure message,
 * emitted as a single Base58 word so it survives pasting into any chat app.
 *
 * A normal ASCII-armored PGP message is verbose and multi-line; pasted into a
 * messenger its boilerplate, line-wraps and CRC footer get mangled. This codec
 * emits one unbreakable Base58 word (the Bitcoin/IPFS alphabet, no ambiguous
 * 0 O I l and nothing a messenger turns into markdown, a link or a soft-wrap).
 * There is deliberately no marker or prefix.
 *
 * The container carries two message types, distinguished by a @c type byte so
 * decoding is fully automatic — the user never chooses:
 *
 * - @b NORMAL: a stateless wrapper around an ordinary OpenPGP message (encrypted
 *   to the recipient's long-term key). Used for the very first contact and as a
 *   no-forward-secrecy fallback. It can piggyback the sender's signed prekey
 *   bundle so the peer silently gains the ability to reply with forward secrecy.
 * - @b PFS: a Signal-style Double Ratchet message (PFS_INIT establishes the
 *   session via an X3DH handshake authenticated by the sender's OpenPGP key;
 *   PFS_MSG is the compact steady-state message). Confidentiality here comes
 *   from the ratchet's AEAD — there is no OpenPGP layer inside.
 *
 * The shared "password book" (a phrase-derived or default 256-byte table) is
 * mixed into the X3DH secret as an extra shared input; it is no longer used to
 * obfuscate NORMAL messages.
 */
class GF_CORE_EXPORT InstantMessageOperator {
 public:
  /// Which container type an outgoing message should use.
  enum class SendMode : uint8_t {
    kNORMAL,  ///< stateless PGP-in-token (no session / bundle yet)
    kPFS,     ///< forward-secret ratchet message (session or bundle available)
  };

  /// Outcome of decoding an incoming token.
  enum class DecodeStatus : uint8_t {
    kNOT_TOKEN,             ///< not one of our tokens; handle input normally
    kNORMAL_OK,             ///< a NORMAL token; @c pgp_message must be decrypted
    kPFS_OK,                ///< a PFS token; @c plaintext holds the message
    kNO_SESSION,            ///< PFS token for a session we don't have / lost
    kHANDSHAKE_AUTH_FAILED, ///< PFS_INIT whose PGP signature did not verify
    kUNDECRYPTABLE,         ///< PFS token whose message key is gone / bad
    kMALFORMED,             ///< structurally one of ours but corrupt
  };

  /// The result of decoding a token.
  struct DecodeResult {
    DecodeStatus status{DecodeStatus::kNOT_TOKEN};
    QString peer_fpr;      ///< the peer's OpenPGP fingerprint, when known
    GFBuffer pgp_message;  ///< NORMAL: the wrapped OpenPGP message to decrypt
    GFBuffer plaintext;    ///< PFS: the recovered chat plaintext
    bool imported_bundle{};  ///< a peer bundle was auto-imported from this token
  };

  /**
   * @brief Decide whether to send to @p peer_fpr as NORMAL or PFS right now.
   */
  static auto ChooseSendMode(const QString& peer_fpr) -> SendMode;

  /**
   * @brief Whether a NORMAL send to @p peer_fpr should attach our bundle
   * (true until the peer has proven, via a PFS token, that it holds it).
   */
  static auto ShouldAttachBundle(const QString& peer_fpr) -> bool;

  /**
   * @brief Wrap an already-OpenPGP-encrypted message as a NORMAL token,
   * optionally attaching our signed prekey bundle for auto-bootstrap.
   *
   * @param channel OpenPGP engine channel
   * @param signer our OpenPGP key (signs the attached bundle); may be null if
   * @p attach_bundle is false
   * @param pgp_message the binary OpenPGP message to wrap
   * @param attach_bundle attach our signed bundle for the peer to auto-import
   * @return the Base58 token, or empty on failure
   */
  static auto EncodeNormal(int channel, const GpgKeyPtr& signer,
                           const GFBuffer& pgp_message, bool attach_bundle)
      -> QString;

  /**
   * @brief Ratchet-encrypt @p plaintext to @p peer_fpr, emitting PFS_INIT on
   * the first message of a session or PFS_MSG thereafter.
   *
   * @param channel OpenPGP engine channel
   * @param signer our OpenPGP identity key (signs a PFS_INIT transcript)
   * @param peer_fpr the recipient's OpenPGP fingerprint
   * @param plaintext the chat message
   * @return the Base58 token, or empty on failure (e.g. no session and no
   * bundle — the caller should fall back to EncodeNormal)
   */
  static auto EncodePfs(int channel, const GpgKeyPtr& signer,
                        const QString& peer_fpr, const GFBuffer& plaintext)
      -> QString;

  /**
   * @brief Decode any token, automatically dispatching on its type, running the
   * X3DH handshake / ratchet and auto-importing a piggybacked bundle as needed.
   */
  static auto Decode(int channel, const QString& text) -> DecodeResult;

  /**
   * @brief Cheap probe: is @p text one of our tokens at all?
   */
  static auto Contains(const QString& text) -> bool;

  /**
   * @brief Build our signed prekey bundle for manual export (Settings).
   *
   * @param channel OpenPGP engine channel
   * @param signer our OpenPGP identity key
   * @return the Base58-encoded signed bundle, or empty on failure
   */
  static auto ExportBundle(int channel, const GpgKeyPtr& signer) -> QString;

  /**
   * @brief Import a bundle produced by ExportBundle(), verifying its signature.
   *
   * @param channel OpenPGP engine channel
   * @param text the Base58-encoded bundle
   * @return the imported peer's fingerprint, or empty on failure
   */
  static auto ImportBundle(int channel, const QString& text) -> QString;

  /**
   * @brief The current container format version.
   */
  static auto FormatVersion() -> int;
};

}  // namespace GpgFrontend
