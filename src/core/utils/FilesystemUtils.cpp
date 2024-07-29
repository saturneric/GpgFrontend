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

#include "FilesystemUtils.h"

namespace GpgFrontend {

auto GetOnlyFileNameWithPath(const QString &path) -> QString {
  // Create a path object from given string
  QFileInfo file_info(path);
  // Check if file name in the path object has extension
  if (!file_info.fileName().isEmpty()) {
    // Fetch the extension from path object and return
    return file_info.path() + "/" + file_info.baseName();
  }
  // In case of no extension return empty string
  return {};
}

auto GetFileExtension(const QString &path) -> QString {
  return QFileInfo(path).suffix();
}

/**
 * @brief
 *
 */
auto GetFileSizeByPath(const QString &path,
                       const QString &filename_pattern) -> int64_t {
  auto dir = QDir(path);
  QFileInfoList const file_list =
      dir.entryInfoList(QStringList() << filename_pattern, QDir::Files);
  qint64 total_size = 0;

  for (const QFileInfo &file_info : file_list) {
    total_size += file_info.size();
  }
  return total_size;
}

/**
 * @brief
 *
 */
auto GetHumanFriendlyFileSize(int64_t size) -> QString {
  auto num = static_cast<double>(size);
  QStringList list;
  list << "KB" << "MB" << "GB" << "TB";

  QStringListIterator i(list);
  QString unit("bytes");

  while (num >= 1024.0 && i.hasNext()) {
    unit = i.next();
    num /= 1024.0;
  }
  return (QString().setNum(num, 'f', 2) + " " + unit);
}

/**
 * @brief
 *
 */
void DeleteAllFilesByPattern(const QString &path,
                             const QString &filename_pattern) {
  auto dir = QDir(path);

  QStringList const log_files =
      dir.entryList(QStringList() << filename_pattern, QDir::Files);

  for (const auto &file : log_files) {
    QFile::remove(dir.absoluteFilePath(file));
  }
}

}  // namespace GpgFrontend