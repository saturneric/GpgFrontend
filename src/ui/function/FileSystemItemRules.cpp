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

#include "ui/function/FileSystemItemRules.h"

#include <QDir>
#include <QFileInfo>

namespace GpgFrontend::UI {

namespace {

auto HasPathSeparator(const QString& text) -> bool {
  return text.contains("/") || text.contains("\\");
}

/**
 * @brief A path both cleaned and, on Windows, brought to one separator.
 *
 * The comparisons below are plain string comparisons, so the two sides have to
 * agree on how a path is spelled before they meet.
 */
auto NormalizedAbsolutePath(const QString& path) -> QString {
  auto normalized = QDir::cleanPath(QFileInfo(path).absoluteFilePath());

#ifdef Q_OS_WIN
  normalized.replace("\\", "/");
#endif

  return normalized;
}

}  // namespace

auto IsOSReservedName(const QString& name) -> bool {
  static const QSet<QString> kReservedNames = {
      "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
      "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
      "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
  };

  // baseName(), not completeBaseName(): Windows reads a device name up to the
  // first period, so "NUL.tar.gz" is the NUL device just as much as "NUL" is.
  return kReservedNames.contains(QFileInfo(name).baseName().toUpper());
}

auto ValidateFileSystemItemName(const QString& name,
                                const QString& original_name,
                                bool target_exists,
                                bool enforce_os_reserved_names)
    -> FileSystemItemNameStatus {
  if (name.trimmed().isEmpty()) return FileSystemItemNameStatus::kEMPTY;

  if (name == "." || name == "..") return FileSystemItemNameStatus::kDOT_NAME;

  if (HasPathSeparator(name)) {
    return FileSystemItemNameStatus::kPATH_SEPARATOR;
  }

  if (enforce_os_reserved_names && IsOSReservedName(name)) {
    return FileSystemItemNameStatus::kOS_RESERVED;
  }

  // before the existence check, deliberately: in a rename the original name is
  // taken by the very item being renamed.
  if (!original_name.isEmpty() && name == original_name) {
    return FileSystemItemNameStatus::kUNCHANGED;
  }

  if (target_exists) return FileSystemItemNameStatus::kALREADY_EXISTS;

  return FileSystemItemNameStatus::kOK;
}

auto IsMoveIntoItselfOrChild(const QString& source_path,
                             const QString& target_dir, bool source_is_dir)
    -> bool {
  const auto source = NormalizedAbsolutePath(source_path);
  const auto target = NormalizedAbsolutePath(target_dir);

  if (source == target) return true;

  if (!source_is_dir) return false;

  return target.startsWith(source + "/");
}

auto IsSameDirectoryOperation(const QStringList& source_paths,
                              const QString& target_dir) -> bool {
  const auto clean_target_dir = NormalizedAbsolutePath(target_dir);

  for (const auto& source_path : source_paths) {
    auto source_dir =
        QDir::cleanPath(QFileInfo(source_path).absoluteDir().absolutePath());

#ifdef Q_OS_WIN
    source_dir.replace("\\", "/");
#endif

    if (source_dir != clean_target_dir) return false;
  }

  return true;
}

}  // namespace GpgFrontend::UI
