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

#include <functional>

#include "core/function/PassphraseGenerator.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Singleton factory providing GFBuffer transformation and generation
 * utilities.
 *
 * Covers file I/O, Base64 encoding, SHA-256 and HMAC-SHA256 hashing,
 * authenticated encryption (Argon2id and BLAKE2b key derivation variants),
 * and several random passphrase generators. Compression is declared but not
 * yet implemented.
 */
class GF_CORE_EXPORT GFBufferFactory
    : public SingletonFunctionObject<GFBufferFactory> {
 public:
  /**
   * @brief Construct the factory for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit GFBufferFactory(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Read the contents of a file into a GFBuffer.
   *
   * @param path absolute or relative path to the file
   * @return buffer containing the file contents, or empty on read failure
   */
  static auto FromFile(const QString& path) -> GFBufferOrNone;

  /**
   * @brief Write a GFBuffer to a file, overwriting any existing content.
   *
   * @param path absolute or relative path to the destination file
   * @param buffer data to write
   * @return true on success, false on write failure
   */
  static auto ToFile(const QString& path, const GFBuffer& buffer) -> bool;

  /**
   * @brief Encode a GFBuffer to standard Base64.
   *
   * @param buffer binary data to encode
   * @return Base64-encoded buffer, or empty if @p buffer is empty or encoding
   * fails
   */
  static auto ToBase64(const GFBuffer& buffer) -> GFBufferOrNone;

  /**
   * @brief Decode a standard Base64 buffer back to binary.
   *
   * @param buffer Base64-encoded input
   * @return decoded binary buffer, or empty if @p buffer is empty or decoding
   * fails
   */
  static auto FromBase64(const GFBuffer& buffer) -> GFBufferOrNone;

  /**
   * @brief Compute the SHA-256 hash of a buffer.
   *
   * @param buffer data to hash
   * @return 32-byte SHA-256 digest, or empty if @p buffer is empty or hashing
   * fails
   */
  static auto ToSha256(const GFBuffer& buffer) -> GFBufferOrNone;

  /// Callback type passed to the streaming ToSha256 overload.
  using Sha256Chunk = std::function<void(const void*, size_t)>;

  /**
   * @brief Compute SHA-256 by streaming chunks through a feeder callable.
   *
   * The caller invokes @p feeder, which receives a @c Sha256Chunk. Each call
   * to that chunk appends bytes to the running hash state. This avoids
   * materializing the entire input in one allocation.
   *
   * @param feeder callable that drives data feeding via the chunk callback
   * @return 32-byte SHA-256 digest, or empty on initialisation failure
   */
  static auto ToSha256(
      const std::function<void(const Sha256Chunk&)>& feeder) -> GFBufferOrNone;

  /**
   * @brief Compute HMAC-SHA256 of @p data authenticated with @p key.
   *
   * If @p key is not exactly 32 bytes it is first hashed with SHA-256 to
   * produce a normalised key.
   *
   * @param key authentication key (any length; normalised to 32 bytes
   * internally)
   * @param data data to authenticate
   * @return 32-byte HMAC digest, or empty if either input is empty or the
   * operation fails
   */
  static auto ToHMACSha256(const GFBuffer& key, const GFBuffer& data)
      -> GFBufferOrNone;

  /**
   * @brief Compress a buffer.
   *
   * Not yet implemented; always returns empty.
   *
   * @param buffer data to compress
   * @return empty (unimplemented)
   */
  auto Compress(const GFBuffer& buffer) -> GFBufferOrNone;

  /**
   * @brief Decompress a buffer.
   *
   * Not yet implemented; always returns empty.
   *
   * @param buffer data to decompress
   * @return empty (unimplemented)
   */
  auto Decompress(const GFBuffer& buffer) -> GFBufferOrNone;

  /**
   * @brief Encrypt a buffer using XChaCha20-Poly1305 with Argon2id key
   * derivation.
   *
   * Delegates to AESCryptoHelper::Encrypt. Suitable for passphrases or
   * lower-entropy key material; computationally expensive per call.
   *
   * @param passphase key material or passphrase for encryption
   * @param buffer plaintext to encrypt
   * @return authenticated ciphertext, or empty if @p buffer is empty or
   * encryption fails
   */
  static auto Encrypt(const GFBuffer& passphase, const GFBuffer& buffer)
      -> GFBufferOrNone;

  /**
   * @brief Decrypt a buffer produced by Encrypt().
   *
   * @param passphase key material or passphrase used during encryption
   * @param buffer ciphertext to decrypt
   * @return plaintext, or empty if @p buffer is empty or authentication fails
   */
  static auto Decrypt(const GFBuffer& passphase, const GFBuffer& buffer)
      -> GFBufferOrNone;

  /**
   * @brief Encrypt a buffer using XChaCha20-Poly1305 with fast BLAKE2b key
   * derivation.
   *
   * Delegates to AESCryptoHelper::EncryptLite. Intended for
   * already-high-entropy keys; much faster than Encrypt().
   *
   * @param passphase high-entropy key material for encryption
   * @param buffer plaintext to encrypt
   * @return authenticated ciphertext, or empty if @p buffer is empty or
   * encryption fails
   */
  static auto EncryptLite(const GFBuffer& passphase, const GFBuffer& buffer)
      -> GFBufferOrNone;

  /**
   * @brief Decrypt a buffer produced by EncryptLite().
   *
   * @param passphase high-entropy key material used during encryption
   * @param buffer ciphertext to decrypt
   * @return plaintext, or empty if @p buffer is empty or authentication fails
   */
  static auto DecryptLite(const GFBuffer& passphase, const GFBuffer& buffer)
      -> GFBufferOrNone;

  /**
   * @brief Generate @p len random bytes using SecureRandomGenerator.
   *
   * Despite the "OpenSSL" name, this uses libsodium via SecureRandomGenerator.
   * Length is clamped to [16, 512].
   *
   * @param len desired number of bytes (clamped to [16, 512])
   * @return random byte buffer, or empty on failure
   */
  static auto RandomOpenSSLPassphase(int len) -> GFBufferOrNone;

  /**
   * @brief Generate @p len random bytes using
   * PassphraseGenerator::GenerateBytes.
   *
   * Length is clamped to [16, 512].
   *
   * @param len desired number of bytes (clamped to [16, 512])
   * @return random byte buffer, or empty on failure
   */
  auto RandomGpgPassphase(int len) -> GFBufferOrNone;

  /**
   * @brief Generate a z-base-32 encoded random identifier.
   *
   * The @p len parameter is ignored; the output length is fixed at 31
   * characters as produced by SecureRandomGenerator::GenerateZBase32().
   *
   * @param len ignored
   * @return z-base-32 random buffer (31 characters), or empty on failure
   */
  auto RandomGpgZBasePassphase(int len) -> GFBufferOrNone;

  /**
   * @brief Convert a GFBuffer to a QString.
   *
   * @param buffer source buffer
   * @return QString constructed from the buffer contents
   */
  auto QString(const GFBuffer& buffer) -> ::QString;

  /**
   * @brief Convert a GFBuffer to a QByteArray.
   *
   * @param buffer source buffer
   * @return QByteArray containing a copy of the buffer contents
   */
  auto QByteArray(const GFBuffer& buffer) -> ::QByteArray;

 private:
  PassphraseGenerator& ph_gen_ = PassphraseGenerator::GetInstance(
      GetChannel());  ///< Passphrase generator for this channel
};

}  // namespace GpgFrontend
