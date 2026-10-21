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
 * @brief The PassphraseGenerator class
 *
 * This class is used to generate a passphrase.
 */
class GF_CORE_EXPORT PassphraseGenerator
    : public SingletonFunctionObject<PassphraseGenerator> {
 public:
  /**
   * @brief PassphraseGenerator constructor
   *
   * @param channel The channel to use
   */
  explicit PassphraseGenerator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief generate passphrase
   *
   * @param len length of the passphrase
   * @return QString passphrase
   */
  auto Generate(int len) -> GFBufferOrNone;

  /**
   * @brief generate passphrase
   *
   * @param len length of the passphrase
   * @return QString passphrase
   */
  auto GenerateBytes(int len) -> GFBufferOrNone;

  /**
   * @brief generate passphrase
   *
   * @param len length of the passphrase
   * @return QString passphrase
   */
  static auto GenerateBytesByOpenSSL(int len) -> GFBufferOrNone;

 private:
  /**
   * @brief Draw @p len random bytes from GnuPG, falling back to OpenSSL.
   */
  auto draw_random_bytes(int len) -> GFBufferOrNone;

  SecureRandomGenerator& rand_ =
      SecureRandomGenerator::GetInstance(SingletonFunctionObject::GetChannel());
};

}  // namespace GpgFrontend
