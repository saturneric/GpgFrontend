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

#include "platform/common/PlatformFs.h"

#ifdef Q_OS_MACOS

#include <sys/xattr.h>

#include <QFile>

namespace GpgFrontend::Platform {

void ExcludeFromIndexAndBackup(const QString &path) {
  // Spotlight reads this one; Time Machine reads the extended attribute. Not
  // CSBackupSetItemExcluded(), which would pull in CoreServices for no gain.
  QFile never_index(path + "/.metadata_never_index");
  if (never_index.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    never_index.close();
  }

  const auto native = path.toLocal8Bit();
  const char kExcluded = 1;
  setxattr(native.constData(),
           "com.apple.metadata:com_apple_backup_excludeItem", &kExcluded,
           sizeof(kExcluded), 0, 0);
}

}  // namespace GpgFrontend::Platform

#endif  // Q_OS_MACOS
