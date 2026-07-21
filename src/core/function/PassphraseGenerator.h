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

#include "core/function/SecureRandomGenerator.h"
#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @brief Map uniformly random bytes onto the [0-9A-Za-z] alphabet.
 *
 * Uses rejection sampling: bytes at or above the largest multiple of 62 that
 * fits in an octet (248) are discarded rather than folded with a modulo, which
 * would over-represent the first eight characters of the alphabet. Callers
 * therefore consume more input than they produce and must top up @p src until
 * @p dst is full.
 *
 * Exposed as a free function so the byte-to-character mapping can be tested
 * against a known input; PassphraseGenerator::Generate() draws from a singleton
 * random source and offers no seam of its own.
 *
 * @param src random input bytes
 * @param src_size number of bytes available at @p src
 * @param dst output buffer receiving alphanumeric characters
 * @param dst_size capacity of @p dst
 * @return number of characters written to @p dst
 */
GF_CORE_EXPORT auto MapRandomBytesToAlphanum(const char *src, size_t src_size,
                                             char *dst, size_t dst_size)
    -> size_t;

/**
 * @brief Singleton that generates passphrases and random byte sequences.
 *
 * Wraps SecureRandomGenerator to produce alphanumeric passphrases (Generate)
 * or raw random bytes (GenerateBytes / GenerateBytesByOpenSSL). The
 * alphanumeric generator falls back to Qt's non-cryptographic random source if
 * the secure generator is unavailable.
 */
class GF_CORE_EXPORT PassphraseGenerator
    : public SingletonFunctionObject<PassphraseGenerator> {
 public:
  /**
   * @brief Construct the generator for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit PassphraseGenerator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Generate a random alphanumeric passphrase of @p len characters.
   *
   * Characters are drawn uniformly from [0-9A-Za-z] (62-character alphabet)
   * via MapRandomBytesToAlphanum(). Falls back to Qt's QRandomGenerator if
   * SecureRandomGenerator is unavailable or returns short; the fallback is not
   * cryptographically secure.
   *
   * @param len number of characters to generate
   * @return passphrase buffer of @p len bytes, or empty on failure
   */
  auto Generate(int len) -> GFBufferOrNone;

  /**
   * @brief Generate @p len raw random bytes using SecureRandomGenerator.
   *
   * Unlike Generate(), this returns unencoded binary data with no fallback;
   * returns empty if the secure generator fails.
   *
   * @param len number of bytes to generate
   * @return buffer of @p len random bytes, or empty on failure
   */
  auto GenerateBytes(int len) -> GFBufferOrNone;

  /**
   * @brief Generate @p len raw random bytes (delegates to
   * SecureRandomGenerator).
   *
   * Despite the name, this currently uses libsodium via SecureRandomGenerator
   * rather than OpenSSL directly. Behaviour is identical to GenerateBytes().
   *
   * @param len number of bytes to generate
   * @return buffer of @p len random bytes, or empty on failure
   */
  static auto GenerateBytesByOpenSSL(int len) -> GFBufferOrNone;

 private:
  SecureRandomGenerator &rand_ = SecureRandomGenerator::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< Secure random source for
                                               ///< this channel
};

}  // namespace GpgFrontend
