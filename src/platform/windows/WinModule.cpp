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

#include "platform/common/PlatformModule.h"

#ifdef Q_OS_WINDOWS

// <windows.h> before <imagehlp.h>: the latter is not self-contained.
#include <windows.h>
// clang-format off
#include <imagehlp.h>
// clang-format on

#include <QCryptographicHash>

#include "core/GFCoreLog.h"

namespace GpgFrontend::Platform {

namespace {

/// Receives the image bytes ImageGetDigestStream() decides are covered.
auto WINAPI PeDigestSink(DIGEST_HANDLE handle, PBYTE data, DWORD length)
    -> BOOL {
  auto *hash = reinterpret_cast<QCryptographicHash *>(handle);
  hash->addData(QByteArray(reinterpret_cast<const char *>(data),
                           static_cast<qsizetype>(length)));
  return TRUE;
}

}  // namespace

auto PeImageDigestSha256(const QString &path, QString &reason) -> QString {
  const auto handle = CreateFileW(
      reinterpret_cast<const wchar_t *>(path.utf16()), GENERIC_READ,
      FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    reason = QString("\"%1\" could not be opened to hash it").arg(path);
    return {};
  }

  QCryptographicHash hash(QCryptographicHash::Sha256);
  // DigestLevel 0: the Authenticode image digest. The CERT_PE_IMAGE_DIGEST_*
  // flags widen it to cover debug info, resources and import tables, none of
  // which Authenticode includes.
  const auto ok = ImageGetDigestStream(handle, 0, PeDigestSink, &hash);
  CloseHandle(handle);

  if (ok == FALSE) {
    reason = QString("\"%1\" is not a PE image this can hash").arg(path);
    return {};
  }
  return QString::fromLatin1(hash.result().toHex());
}

ScopedLibrarySearchDirectory::ScopedLibrarySearchDirectory(
    const QString &directory) {
  if (directory.isEmpty()) return;

  applied_ = SetDllDirectoryW(
                 reinterpret_cast<const wchar_t *>(directory.utf16())) != 0;
  if (!applied_) {
    LOG_W() << "cannot add module directory to the library search path: "
            << directory;
  }
}

ScopedLibrarySearchDirectory::~ScopedLibrarySearchDirectory() {
  // an empty string, unlike nullptr, restores the default search order
  // without putting the current working directory back into it
  if (applied_) SetDllDirectoryW(L"");
}

}  // namespace GpgFrontend::Platform

#endif  // Q_OS_WINDOWS
