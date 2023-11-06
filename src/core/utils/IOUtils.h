/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
 * @brief read file content using std struct
 *
 *
 * @param file_name file name
 * @param data data read from file
 * @return
 */
auto GPGFRONTEND_CORE_EXPORT ReadFileStd(const std::filesystem::path &file_name,
                                         std::string &data) -> bool;

/**
 * @brief write file content using std struct
 *
 * @param file_name file name
 * @param data data to write to file
 * @return
 */
auto GPGFRONTEND_CORE_EXPORT WriteFileStd(
    const std::filesystem::path &file_name, const std::string &data) -> bool;

/**
 * @brief read file content
 *
 * @param file_name file name
 * @param data data read from file
 * @return true if success
 * @return false if failed
 */
auto GPGFRONTEND_CORE_EXPORT ReadFile(const QString &file_name,
                                      QByteArray &data) -> bool;

/**
 * @brief write file content
 *
 * @param file_name file name
 * @param data data to write to file
 * @return true if success
 * @return false if failed
 */
auto GPGFRONTEND_CORE_EXPORT WriteFile(const QString &file_name,
                                       const QByteArray &data) -> bool;

/**
 * calculate the hash of a file
 * @param file_path
 * @return
 */
auto GPGFRONTEND_CORE_EXPORT
CalculateHash(const std::filesystem::path &file_path) -> std::string;

/**
 * @brief
 *
 * @param path
 * @return std::string
 */
auto GPGFRONTEND_CORE_EXPORT ReadAllDataInFile(const std::string &path)
    -> std::string;

/**
 * @brief
 *
 * @param path
 * @param out_buffer
 * @return true
 * @return false
 */
auto GPGFRONTEND_CORE_EXPORT WriteBufferToFile(const std::string &path,
                                               const std::string &out_buffer)
    -> bool;
}  // namespace GpgFrontend
