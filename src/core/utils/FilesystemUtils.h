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
 * @brief Return the file extension of the given path (without the dot).
 *
 * @param path file path
 * @return extension string (e.g. "txt"), or empty if none
 */
auto GF_CORE_EXPORT GetFileExtension(const QString& path) -> QString;

/**
 * @brief Return the file name component of the given path, including any
 * extension.
 *
 * @param path file path
 * @return file name string without the directory prefix
 */
auto GF_CORE_EXPORT GetOnlyFileNameWithPath(const QString& path) -> QString;

/**
 * @brief Return the total size in bytes of all files matching the pattern under
 * @p path.
 *
 * @param path directory to search
 * @param filename_pattern glob pattern for file names (e.g. "*.log")
 * @return total size in bytes
 */
auto GF_CORE_EXPORT GetFileSizeByPath(const QString& path,
                                      const QString& filename_pattern)
    -> int64_t;

/**
 * @brief Format a byte count as a human-friendly size string (e.g. "4.2 MB").
 *
 * @param size size in bytes
 * @return formatted size string
 */
auto GF_CORE_EXPORT GetHumanFriendlyFileSize(int64_t size) -> QString;

/**
 * @brief Delete all files matching the pattern under the given directory.
 *
 * @param path directory to search
 * @param filename_pattern glob pattern for file names to delete (e.g. "*.log")
 */
void GF_CORE_EXPORT DeleteAllFilesByPattern(const QString& path,
                                            const QString& filename_pattern);

}  // namespace GpgFrontend
