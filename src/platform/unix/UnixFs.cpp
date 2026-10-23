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

#ifdef Q_OS_UNIX

#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QFile>

#include "core/profile/ProfileStoragePlan.h"

namespace GpgFrontend::Platform {

auto StatRegularFile(const QString &path) -> RegularFileStamp {
  RegularFileStamp stamp;

  struct stat st{};
  if (::stat(QFile::encodeName(path).constData(), &st) != 0 ||
      !S_ISREG(st.st_mode)) {
    return stamp;
  }
  stamp.size = static_cast<qint64>(st.st_size);
#if defined(Q_OS_MACOS)
  stamp.modified_ms = static_cast<qint64>(st.st_mtimespec.tv_sec) * 1000 +
                      st.st_mtimespec.tv_nsec / 1000000;
#else
  stamp.modified_ms = static_cast<qint64>(st.st_mtim.tv_sec) * 1000 +
                      st.st_mtim.tv_nsec / 1000000;
#endif
  stamp.file_id = static_cast<quint64>(st.st_ino);
  return stamp;
}

void FlushToDisk(int fd) { ::fsync(fd); }

auto LockedMemoryLimit() -> qint64 {
  struct rlimit limit{};
  if (getrlimit(RLIMIT_MEMLOCK, &limit) == 0 &&
      limit.rlim_cur != RLIM_INFINITY) {
    return static_cast<qint64>(limit.rlim_cur);
  }
  return -1;
}

auto DuplicateFd(int fd) -> int { return ::dup(fd); }

auto CloseFd(int fd) -> int { return ::close(fd); }

auto EffectiveUserId() -> uint { return static_cast<uint>(geteuid()); }

auto ClaimPrivateDirectory(const QString &path, QString &reason) -> bool {
  if (mkdir(path.toLocal8Bit().constData(), 0700) == 0) return true;

  struct stat info{};
  if (lstat(path.toLocal8Bit().constData(), &info) != 0) {
    // Not there and not creatable. Usually the parent is root-owned and this
    // login has no per-user runtime directory at all, which is ordinary inside
    // a container and worth saying rather than reporting as a mystery.
    reason = "this folder is not there and could not be created";
    return false;
  }

  if (!IsAcceptableOwnership(info.st_uid, info.st_mode,
                             S_ISLNK(info.st_mode) != 0, geteuid())) {
    // /dev/shm is world-writable, so something else may already be sitting at
    // the name this build would pick. Adopting it would hand another local
    // account a live view of somebody's keys.
    reason = "a folder of this name is already there and is not private to you";
    return false;
  }

  return S_ISDIR(info.st_mode) != 0;
}

}  // namespace GpgFrontend::Platform

#endif  // Q_OS_UNIX
