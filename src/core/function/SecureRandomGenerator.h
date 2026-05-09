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

class GF_CORE_EXPORT SecureRandomGenerator
    : public SingletonFunctionObject<SecureRandomGenerator> {
 public:
  explicit SecureRandomGenerator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Generate cryptographically secure random bytes.
   *
   * @param size number of bytes to generate
   * @return GFBufferOrNone
   */
  static auto Generate(size_t size) -> GFBufferOrNone;

  /**
   * @brief Generate a z-base-32 encoded random identifier.
   *
   * @return GFBufferOrNone
   */
  static auto GenerateZBase32() -> GFBufferOrNone;

 private:
  OpenPGPContext& ctx_ =
      OpenPGPContext::GetInstance(SingletonFunctionObject::GetChannel());
};

}  // namespace GpgFrontend