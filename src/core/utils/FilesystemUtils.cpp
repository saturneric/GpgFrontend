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

/**
 * @brief
 *
 */
auto GetFileSizeByPath(const QString &path,
                       const QString &filename_pattern) -> int64_t {
  auto dir = QDir(path);
  QFileInfoList const file_list =
      dir.entryInfoList(QStringList{filename_pattern}, QDir::Files);
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

}  // namespace GpgFrontend