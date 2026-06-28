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
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

/**
 * @brief Read the entire contents of a file into a GFBuffer.
 *
 * @param file_name path to the file to read
 * @return tuple of (success flag, buffer containing file contents)
 */
auto GF_CORE_EXPORT ReadFileGFBuffer(const QString& file_name)
    -> std::tuple<bool, GFBuffer>;

/**
 * @brief Write a GFBuffer to a file, overwriting any existing content.
 *
 * @param file_name destination file path
 * @param data binary data to write
 * @return true on success, false if the file could not be opened or written
 */
auto GF_CORE_EXPORT WriteFileGFBuffer(const QString& file_name, GFBuffer data)
    -> bool;

/**
 * @brief Read the entire contents of a file into a QByteArray.
 *
 * @param file_name path to the file to read
 * @param data output parameter filled with the file contents on success
 * @return true on success, false if the file could not be opened
 */
auto GF_CORE_EXPORT ReadFile(const QString& file_name, QByteArray& data)
    -> bool;

/**
 * @brief Write a QByteArray to a file, overwriting any existing content.
 *
 * @param file_name destination file path
 * @param data data to write
 * @return true on success, false if the file could not be opened or written
 */
auto GF_CORE_EXPORT WriteFile(const QString& file_name, const QByteArray& data)
    -> bool;

/**
 * @brief Calculate the SHA-256 hash of a file and return it as a hex string.
 *
 * @param file_path path to the file to hash
 * @return hex-encoded SHA-256 digest string
 */
auto GF_CORE_EXPORT CalculateHash(const QString& file_path) -> QString;

/**
 * @brief Compute the file hash report as structured (key, value) field pairs.
 *
 * This is the source of truth for the hash report: the file is read and hashed
 * exactly once here. Both the human-readable text (FormatFileHashInfo) and the
 * UI card are derived from these fields, so nothing has to parse a formatted,
 * localized string back into data.
 *
 * @param file_path path to the file to hash
 * @return ordered list of (key, value) pairs, empty if the file is unreadable
 */
auto GF_CORE_EXPORT CalculateFileHashInfo(const QString& file_path)
    -> QContainer<QPair<QString, QString>>;

/**
 * @brief Render the structured fields from CalculateFileHashInfo() into the
 * human-readable report text shown in the raw details view.
 *
 * @param fields field pairs produced by CalculateFileHashInfo()
 * @return formatted report text
 */
auto GF_CORE_EXPORT FormatFileHashInfo(
    const QContainer<QPair<QString, QString>>& fields) -> QString;

/**
 * @brief Calculate a checksum of a binary file and return it as a hex string.
 *
 * @param path path to the binary file
 * @return hex-encoded checksum string
 */
auto GF_CORE_EXPORT CalculateBinaryChacksum(const QString& path) -> QString;

/**
 * @brief Write a string buffer to a file, overwriting any existing content.
 *
 * @param path destination file path
 * @param out_buffer string content to write
 * @return true on success, false if the file could not be opened or written
 */
auto GF_CORE_EXPORT WriteBufferToFile(const QString& path,
                                      const QString& out_buffer) -> bool;

/**
 * @brief Return the path to a new temporary file in the system temp directory.
 *
 * @return absolute path to a temporary file (not yet created)
 */
auto GF_CORE_EXPORT GetTempFilePath() -> QString;

/**
 * @brief Create a temporary file, write string @p data to it, and return its
 * path.
 *
 * @param data string content to write
 * @return absolute path to the created temporary file, or empty on failure
 */
auto GF_CORE_EXPORT CreateTempFileAndWriteData(const QString& data) -> QString;

/**
 * @brief Create a temporary file, write binary @p data to it, and return its
 * path.
 *
 * @param data binary content to write
 * @return absolute path to the created temporary file, or empty on failure
 */
auto GF_CORE_EXPORT CreateTempFileAndWriteData(const GFBuffer& data) -> QString;

/**
 * @brief Check whether a file at @p path can be read from or written to.
 *
 * @param path file path to check
 * @param read true to check readability, false to check writability
 * @return tuple of (ok flag, error message string); error string is empty on
 * success
 */
auto GF_CORE_EXPORT TargetFilePreCheck(const QString& path, bool read)
    -> std::tuple<bool, QString>;

/**
 * @brief Return the full compound extension of a file path (e.g. ".tar.gz").
 *
 * Unlike QFileInfo::suffix(), this includes all dot-separated suffix
 * components starting from the first dot in the file name.
 *
 * @param path file path
 * @return full extension string including all dots, or empty if none
 */
auto GF_CORE_EXPORT GetFullExtension(QString path) -> QString;

}  // namespace GpgFrontend
