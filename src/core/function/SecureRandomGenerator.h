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
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Singleton providing cryptographically secure random data via
 * libsodium.
 *
 * All generation methods use libsodium's randombytes_buf, which is seeded from
 * the OS entropy source. The class requires libsodium to be successfully
 * initialised before any Generate call; if initialisation fails the call
 * returns empty.
 */
class GF_CORE_EXPORT SecureRandomGenerator
    : public SingletonFunctionObject<SecureRandomGenerator> {
 public:
  /**
   * @brief Construct the generator for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit SecureRandomGenerator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Generate cryptographically secure random bytes.
   *
   * @param size number of bytes to generate
   * @return buffer containing @p size random bytes, or empty if libsodium
   *         initialisation failed
   */
  static auto Generate(size_t size) -> GFBufferOrNone;

  /**
   * @brief Generate a z-base-32 encoded random identifier.
   *
   * Generates 20 random bytes and encodes them with the z-base-32 alphabet,
   * producing a 31-character human-readable string.
   *
   * @return z-base-32 encoded random buffer (31 characters), or empty on
   * failure
   */
  static auto GenerateZBase32() -> GFBufferOrNone;

 private:
  OpenPGPContext& ctx_ = OpenPGPContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< OpenPGP context held to
                                               ///< ensure channel lifetime
};

}  // namespace GpgFrontend
