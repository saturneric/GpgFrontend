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

#ifdef Q_OS_WINDOWS

#include <io.h>
#include <windows.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace GpgFrontend::Platform {

auto StatRegularFile(const QString &path) -> RegularFileStamp {
  RegularFileStamp stamp;

  const QFileInfo info(path);
  if (!info.exists() || !info.isFile()) return stamp;
  stamp.size = info.size();
  stamp.modified_ms =
      info.fileTime(QFileDevice::FileModificationTime).toMSecsSinceEpoch();
  return stamp;
}

void ExcludeFromIndexAndBackup(const QString &path) {
  // POSIX owner-only permissions mean nothing on NTFS, so the attributes carry
  // what they can and the ACL is left to the directory's inherited default.
  const auto native = QDir::toNativeSeparators(path).toStdWString();
  auto attributes = GetFileAttributesW(native.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) return;

  // TEMPORARY is a hint to the cache manager to avoid flushing, not a promise.
  attributes |= FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
  SetFileAttributesW(native.c_str(), attributes);
}

void FlushToDisk(int fd) {
  // Not flushed, as it never has been here: FlushFileBuffers() on the handle
  // behind the descriptor would be the equivalent of fsync().
  Q_UNUSED(fd)
}

auto LockedMemoryLimit() -> qint64 { return -1; }

auto DuplicateFd(int fd) -> int { return _dup(fd); }

auto CloseFd(int fd) -> int { return _close(fd); }

auto EncryptDirectory(const QString &path, QString &reason) -> bool {
  const auto native = QDir::toNativeSeparators(path).toStdWString();

  if (EncryptFileW(native.c_str()) == 0) {
    reason = "this volume or edition does not support file encryption";
    return false;
  }

  const auto attributes = GetFileAttributesW(native.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & FILE_ATTRIBUTE_ENCRYPTED) == 0) {
    reason = "file encryption was accepted but did not take effect";
    return false;
  }

  // The directory's own attribute says only that files created here should
  // inherit encryption. It is not a statement about any file, and the claim
  // this makes to the user -- that what lands here is unreadable to anyone
  // else -- is entirely about the files. So one is created and asked.
  //
  // The case this catches is not hypothetical: inheritance is what fails when
  // the account has no usable EFS certificate, and the directory keeps its
  // attribute either way. Without this, that machine would be told its keys
  // were in an encrypted folder while every file in it was plaintext.
  const auto probe = path + "/.gf-encryption-probe";

  QFile file(probe);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    reason = "this folder could not be written to";
    return false;
  }
  file.write("probe");
  file.close();

  const auto probe_native = QDir::toNativeSeparators(probe).toStdWString();
  const auto probe_attributes = GetFileAttributesW(probe_native.c_str());

  // Removed whatever the answer: it has served its purpose, and leaving a file
  // behind in a directory whose emptiness the caller relies on is its own bug.
  QFile::remove(probe);

  if (probe_attributes == INVALID_FILE_ATTRIBUTES ||
      (probe_attributes & FILE_ATTRIBUTE_ENCRYPTED) == 0) {
    reason = "files created in this folder are not encrypted";
    return false;
  }

  return true;
}

}  // namespace GpgFrontend::Platform

#endif  // Q_OS_WINDOWS
