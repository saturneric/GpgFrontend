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

namespace GpgFrontend::UI {

/**
 * @brief Why a file or folder name cannot be used.
 *
 * Creating and renaming apply the very same rules, so they share this one
 * verdict rather than each growing their own half of it.
 */
enum class FileSystemItemNameStatus : uint8_t {
  kOK,              ///< the name can be used
  kEMPTY,           ///< nothing, or only whitespace
  kDOT_NAME,        ///< "." or ".."
  kPATH_SEPARATOR,  ///< a name, not a path: no "/" and no "\"
  kOS_RESERVED,     ///< a device name Windows refuses to hand out
  kUNCHANGED,       ///< rename only: identical to the original name
  kALREADY_EXISTS,  ///< something else already sits at that path
};

/**
 * @brief Whether the reserved-name rule applies to this platform.
 *
 * Only Windows refuses these names, but the check itself is worth asserting
 * everywhere, so it reaches ValidateFileSystemItemName() as an argument
 * instead of hiding behind an #ifdef inside it.
 */
constexpr bool kEnforceOSReservedNames =
#ifdef Q_OS_WIN
    true;
#else
    false;
#endif

/**
 * @brief Whether Windows reserves this name for a device.
 *
 * The extension is irrelevant to Windows: "CON" and "CON.txt" are equally
 * refused.
 *
 * @param name the bare item name
 * @return true if the name is reserved
 */
auto GF_UI_EXPORT IsOSReservedName(const QString& name) -> bool;

/**
 * @brief Check a name a user typed for a file or folder.
 *
 * Pure, so every rule is assertable without touching a disk. The caller looks
 * up whether the target path is taken and passes the answer in.
 *
 * The order of the checks is deliberate: kUNCHANGED comes before
 * kALREADY_EXISTS, because in a rename the untouched name always exists.
 *
 * @param name the trimmed name the user typed
 * @param original_name the name being replaced, empty when creating
 * @param target_exists whether something already sits at the resulting path
 * @param enforce_os_reserved_names whether the Windows device names apply
 * @return why the name cannot be used, or kOK
 */
auto GF_UI_EXPORT ValidateFileSystemItemName(
    const QString& name, const QString& original_name, bool target_exists,
    bool enforce_os_reserved_names = kEnforceOSReservedNames)
    -> FileSystemItemNameStatus;

/**
 * @brief Whether a move would put an item inside itself.
 *
 * Covers both the item dropped onto itself and the item dropped into one of
 * its own descendants. Pure: the caller reports whether the source is a
 * directory, since only a directory can contain anything.
 *
 * @param source_path the item being dragged
 * @param target_dir the directory it was dropped on
 * @param source_is_dir whether the source is a directory
 * @return true if the move is impossible
 */
auto GF_UI_EXPORT IsMoveIntoItselfOrChild(const QString& source_path,
                                          const QString& target_dir,
                                          bool source_is_dir) -> bool;

/**
 * @brief Whether every source already lives in the target directory.
 *
 * Such a drop asks for nothing to happen, which usually means the user let go
 * of the drag rather than aimed it.
 *
 * @param source_paths the items being dragged
 * @param target_dir the directory they were dropped on
 * @return true if the operation would be a no-op
 */
auto GF_UI_EXPORT IsSameDirectoryOperation(const QStringList& source_paths,
                                           const QString& target_dir) -> bool;

}  // namespace GpgFrontend::UI
