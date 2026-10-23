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

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)

#include <sys/statfs.h>

#include "core/profile/ProfileStoragePlan.h"

namespace GpgFrontend::Platform {

void ExcludeFromIndexAndBackup(const QString &path) {
  // Nothing platform-wide to mark here: the CACHEDIR.TAG and .nobackup the
  // caller writes are what Linux backup tools read.
  Q_UNUSED(path)
}

auto IsRamBackedPath(const QString &path, QString &reason) -> bool {
  struct statfs info{};
  if (statfs(path.toLocal8Bit().constData(), &info) != 0) {
    reason = "this folder could not be examined";
    return false;
  }

#ifdef Q_OS_FREEBSD
  // No magic number here; the name is what there is.
  if (QLatin1String(info.f_fstypename) != QLatin1String("tmpfs")) {
    reason = QString("not a memory-backed filesystem (%1)")
                 .arg(QLatin1String(info.f_fstypename));
    return false;
  }
#else
  if (!IsRamBackedMagic(static_cast<quint64>(info.f_type))) {
    reason = "not a memory-backed filesystem";
    return false;
  }
#endif

  return true;
}

#ifdef Q_OS_LINUX

auto IsFscryptCapableOrigin(const QString &path, QString &reason) -> bool {
  const auto origin = NearestExistingAncestor(path);
  if (origin.isEmpty()) {
    reason = "this folder could not be examined";
    return false;
  }

  struct statfs info{};
  if (statfs(origin.toLocal8Bit().constData(), &info) != 0) {
    reason = "this folder could not be examined";
    return false;
  }

  if (!IsFscryptCapableMagic(static_cast<quint64>(info.f_type))) {
    reason = "this filesystem cannot encrypt folders";
    return false;
  }

  return true;
}

#endif  // Q_OS_LINUX

}  // namespace GpgFrontend::Platform

#endif  // Q_OS_LINUX || Q_OS_FREEBSD
