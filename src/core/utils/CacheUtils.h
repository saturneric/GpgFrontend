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

namespace GpgFrontend {

/**
 * @brief Store a string value in the runtime cache under the given key.
 *
 * @param key cache key
 * @param value string value to store
 */
void GF_CORE_EXPORT SetCacheValue(const QString& key, QString value);

/**
 * @brief Retrieve and immediately remove a value from the runtime cache.
 *
 * The entry is erased on retrieval so each value can only be read once.
 *
 * @param key cache key
 * @return the cached string, or an empty string if the key is not found
 */
auto GF_CORE_EXPORT GetCacheValue(const QString& key) -> QString;

/**
 * @brief Remove a value from the runtime cache without returning it.
 *
 * @param key cache key to erase
 */
void GF_CORE_EXPORT ResetCacheValue(const QString& key);

}  // namespace GpgFrontend
