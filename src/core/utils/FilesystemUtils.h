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

#include "core/GpgFrontendCoreExport.h"

namespace GpgFrontend {

/**
 * @brief Get the file extension object
 *
 * @param path
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetFileExtension(const QString& path) -> QString;

/**
 * @brief Get the only file name with path object
 *
 * @param path
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetOnlyFileNameWithPath(const QString& path)
    -> QString;

/**
 * @brief Get the File Size By Path object
 *
 * @param path The path of the file
 * @param filename_pattern The pattern of the file name, e.g. "*.txt"
 * @return int64_t
 */
auto GPGFRONTEND_CORE_EXPORT GetFileSizeByPath(
    const QString& path, const QString& filename_pattern) -> int64_t;

/**
 * @brief Get the Human Readable File Size object
 *
 * @param size
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetHumanFriendlyFileSize(int64_t size) -> QString;

/**
 * @brief
 *
 * @param path
 * @param filename_pattern
 */
void GPGFRONTEND_CORE_EXPORT
DeleteAllFilesByPattern(const QString& path, const QString& filename_pattern);

}  // namespace GpgFrontend