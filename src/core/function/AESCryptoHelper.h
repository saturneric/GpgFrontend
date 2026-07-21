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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Singleton helper for symmetric authenticated encryption using
 * XChaCha20-Poly1305.
 *
 * Two variants are provided:
 * - Standard (`Encrypt`/`Decrypt`): derives the session key with Argon2id,
 *   suitable for lower-entropy passphrases but computationally expensive.
 * - Lite (`EncryptLite`/`DecryptLite`): derives the session key with BLAKE2b,
 *   fast and intended for already-high-entropy raw keys such as KDF output.
 *
 * On-disk format: magic (6 bytes) | salt | nonce | ciphertext | Poly1305 tag.
 */
class GF_CORE_EXPORT AESCryptoHelper
    : public SingletonFunctionObject<AESCryptoHelper> {
 public:
  AESCryptoHelper();

  /**
   * @brief Encrypt plaintext using XChaCha20-Poly1305 with Argon2id key
   * derivation.
   *
   * Suitable for passphrases or other lower-entropy key material; Argon2id
   * makes brute-force attacks expensive but is slow per call.
   *
   * @param raw_key passphrase or raw key material used for key derivation
   * @param plaintext data to encrypt
   * @return authenticated ciphertext (magic | salt | nonce | ciphertext | tag),
   * or empty on failure
   */
  static auto Encrypt(const GpgFrontend::GFBuffer& raw_key,
                      const GpgFrontend::GFBuffer& plaintext) -> GFBufferOrNone;

  /**
   * @brief Decrypt a buffer produced by Encrypt().
   *
   * @param raw_key passphrase or raw key material used during encryption
   * @param encrypted ciphertext buffer including magic, salt, nonce, and tag
   * @return decrypted plaintext, or empty if authentication fails or input is
   * malformed
   */
  static auto Decrypt(const GpgFrontend::GFBuffer& raw_key,
                      const GpgFrontend::GFBuffer& encrypted) -> GFBufferOrNone;

  /**
   * @brief Encrypt plaintext using XChaCha20-Poly1305 with fast BLAKE2b key
   * derivation.
   *
   * Intended for already-high-entropy keys (e.g. output of a KDF). Much faster
   * than Encrypt() but provides no resistance against brute-force on weak keys.
   *
   * @param raw_key high-entropy raw key used for key derivation
   * @param plaintext data to encrypt
   * @return authenticated ciphertext (magic | salt | nonce | ciphertext | tag),
   * or empty on failure
   */
  static auto EncryptLite(const GpgFrontend::GFBuffer& raw_key,
                          const GpgFrontend::GFBuffer& plaintext)
      -> GFBufferOrNone;

  /**
   * @brief Decrypt a buffer produced by EncryptLite().
   *
   * @param raw_key high-entropy raw key used during encryption
   * @param encrypted ciphertext buffer including magic, salt, nonce, and tag
   * @return decrypted plaintext, or empty if authentication fails or input is
   * malformed
   */
  static auto DecryptLite(const GpgFrontend::GFBuffer& raw_key,
                          const GpgFrontend::GFBuffer& encrypted)
      -> GFBufferOrNone;

  /**
   * @brief Test whether a buffer looks like output of Encrypt()/EncryptLite().
   *
   * Checks the magic prefix and the minimum container length. This makes an
   * encrypted file self-describing: callers can tell an encrypted key file from
   * a plaintext one without keeping a separate marker that could fall out of
   * sync with the file it describes.
   *
   * Note this cannot distinguish Encrypt() from EncryptLite() output; both use
   * the same container.
   *
   * @param buffer candidate buffer
   * @return true if @p buffer carries the container magic and is long enough
   */
  static auto IsEncryptedBuffer(const GpgFrontend::GFBuffer& buffer) -> bool;

  /**
   * @brief Derive a key from a passphrase using Argon2id.
   *
   * Exposed separately from Encrypt() for callers that need the derived key
   * itself rather than a ciphertext.
   *
   * @param passphrase input key material
   * @param salt salt, which must be exactly crypto_pwhash_SALTBYTES long
   * @param key_len desired output length in bytes
   * @param t_cost time cost (Argon2 opslimit)
   * @param m_cost memory cost in KiB
   * @param parallelism ignored; libsodium's crypto_pwhash does not expose it,
   * and a value other than 1 only produces a warning
   * @return derived key, or empty on invalid parameters or derivation failure
   */
  static auto DeriveKeyArgon2(const GpgFrontend::GFBuffer& passphrase,
                              const GpgFrontend::GFBuffer& salt,
                              int key_len = 32, int t_cost = 3,
                              int m_cost = 65536, int parallelism = 4)
      -> GFBufferOrNone;
};
}  // namespace GpgFrontend
