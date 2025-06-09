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
 * @brief
 *
 * @param pin
 * @param key
 * @return GFBuffer
 */
auto CalculateKeyId(const GFBuffer &pin, const GFBuffer &key) -> GFBuffer;

/**
 * @brief
 *
 * @param passphrase
 * @param salt
 * @param key_len
 * @param t_cost
 * @param m_cost
 * @param parallelism
 * @return GFBufferOrNone
 */
auto DeriveKeyArgon2(const GFBuffer &passphrase, const GFBuffer &salt,
                     int key_len = 32, int t_cost = 3, int m_cost = 65536,
                     int parallelism = 4) -> GFBufferOrNone;

/**
 * @brief
 *
 * @param pin
 * @return GFBuffer
 */
auto FetchTimeRelatedAppSecureKey(const GFBuffer &pin) -> GFBuffer;

/**
 * @brief
 *
 * @param pin
 * @return GFBuffer
 */
auto NewLegacyAppSecureKey(const GFBuffer &pin) -> GFBuffer;

/**
 * @brief
 *
 * @param pin
 * @return true
 * @return false
 */
auto InitLegacyAppSecureKey(const GFBuffer &pin) -> bool;

/**
 * @brief
 *
 * @param pin
 * @return true
 * @return false
 */
auto InitAppSecureKey(const GFBuffer &pin) -> bool;
}  // namespace GpgFrontend