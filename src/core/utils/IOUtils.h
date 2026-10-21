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
 * @param file_name
 * @return GFBuffer
 */
auto GF_CORE_EXPORT ReadFileGFBuffer(const QString &file_name)
    -> std::tuple<bool, GFBuffer>;

/**
 * @brief
 *
 * @param file_name
 * @param data
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT WriteFileGFBuffer(const QString &file_name, GFBuffer data)
    -> bool;

/**
 * @brief read file content
 *
 * @param file_name file name
 * @param data data read from file
 * @return true if success
 * @return false if failed
 */
auto GF_CORE_EXPORT ReadFile(const QString &file_name, QByteArray &data)
    -> bool;

/**
 * @brief write file content
 *
 * @param file_name file name
 * @param data data to write to file
 * @return true if success
 * @return false if failed
 */
auto GF_CORE_EXPORT WriteFile(const QString &file_name, const QByteArray &data)
    -> bool;

/**
 * calculate the hash of a file
 * @param file_path
 * @return
 */
auto GF_CORE_EXPORT CalculateHash(const QString &file_path) -> QString;

/**
 * @brief
 *
 * @param path
 * @return QString
 */
auto GF_CORE_EXPORT CalculateBinaryChacksum(const QString &path) -> QString;

/**
 * @brief
 *
 * @param path
 * @param out_buffer
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT WriteBufferToFile(const QString &path,
                                      const QString &out_buffer) -> bool;

/**
 * @brief
 *
 * @return QString
 */
auto GF_CORE_EXPORT GetTempFilePath() -> QString;

/**
 * @brief
 *
 * @param data
 * @return QString
 */
auto GF_CORE_EXPORT CreateTempFileAndWriteData(const QString &data) -> QString;

/**
 * @brief
 *
 * @param data
 * @return QString
 */
auto GF_CORE_EXPORT CreateTempFileAndWriteData(const GFBuffer &data) -> QString;

/**
 * @brief
 *
 * @param path
 * @param read
 * @return std::tuple<bool, QString>
 */
auto GF_CORE_EXPORT TargetFilePreCheck(const QString &path, bool read)
    -> std::tuple<bool, QString>;

/**
 * @brief
 *
 * @param path
 * @return QString
 */
auto GF_CORE_EXPORT GetFullExtension(QString path) -> QString;

/**
 * @brief Return a staging path for an output that will end up at @p final_path.
 *
 * The staged file is placed in a fresh work directory next to the final file,
 * not beside it. The directory is private (0700) and its name unpredictable,
 * so nothing can be planted where the output is staged. The directory an
 * operation writes into is often the one the user is browsing, and that
 * directory is watched: writing every temporary output there makes the file
 * browser react to each one while the operation is still running. Staying on
 * the same volume keeps the later commit a plain rename.
 *
 * The work directory is created as a side effect. If it cannot be created, a
 * path beside @p final_path is returned instead, so an unwritable work
 * directory degrades rather than fails.
 *
 * @param final_path path the output is destined for
 * @return absolute path to write the output to (not yet created)
 */
auto GF_CORE_EXPORT MakeSafeOutputTempPath(const QString& final_path)
    -> QString;

/**
 * @brief Move a staged output file to its final path.
 *
 * An existing final file is renamed aside first and only removed once the new
 * one is in place, so a failed rename leaves the original untouched.
 *
 * @param temp_path staged output produced by MakeSafeOutputTempPath()
 * @param final_path path the output belongs at
 * @return true if the output is in place, false if nothing was changed
 */
auto GF_CORE_EXPORT CommitSafeOutputFile(const QString& temp_path,
                                         const QString& final_path) -> bool;

/**
 * @brief Remove a staged output that will not be committed.
 *
 * Removes a staged file, or a staged directory (an archive extraction) with
 * everything in it, so a failed or cancelled operation leaves no plaintext
 * behind. A symlink is removed, never followed.
 *
 * @param temp_path staged output produced by MakeSafeOutputTempPath()
 * @return true if nothing remains at @p temp_path
 */
auto GF_CORE_EXPORT DiscardSafeOutput(const QString& temp_path) -> bool;

/**
 * @brief Remove the staging directory holding @p temp_path.
 *
 * Does nothing unless @p temp_path lives in a work directory created by
 * MakeSafeOutputTempPath() for this process, and the directory is empty — a
 * leftover file keeps the directory, and the evidence, around.
 *
 * @param temp_path staged output path whose work directory should be dropped
 */
void GF_CORE_EXPORT RemoveSafeOutputWorkDir(const QString& temp_path);

}  // namespace GpgFrontend
