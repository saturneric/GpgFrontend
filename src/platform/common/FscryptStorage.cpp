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

#include "platform/common/FscryptStorage.h"

namespace GpgFrontend {

#ifndef GF_HAS_FSCRYPT

// No fscrypt backend in this build: every platform but Linux, and a Linux
// built against kernel headers without the v2 policy ioctls. The real
// implementation is platform/linux/FscryptStorage.cpp.

auto FscryptAvailable() -> bool { return false; }

auto FscryptDirectoryIsUsable(const QString &path, QString &reason) -> bool {
  Q_UNUSED(path)
  reason = "this build has no filesystem encryption support";
  return false;
}

auto FscryptProvisionDirectory(const QString &dir, QByteArray &identifier,
                               QString &reason) -> bool {
  Q_UNUSED(dir)
  identifier.clear();
  reason = "this build has no filesystem encryption support";
  return false;
}

auto FscryptRemoveKey(const QString &any_path_on_fs,
                      const QByteArray &identifier, QString &reason) -> bool {
  Q_UNUSED(any_path_on_fs)
  Q_UNUSED(identifier)

  // Nothing to remove rather than a failure to remove it, for the same reason
  // the Linux backend answers ENOKEY and ENOTTY with success: a build without
  // the entry points never minted a key and can never evict one, so a caller
  // asking whether anything is left to clean up is owed "no".
  reason.clear();
  return true;
}

#endif  // GF_HAS_FSCRYPT

auto FscryptIdentifierToHex(const QByteArray &id) -> QString {
  if (id.size() != kFscryptKeyIdentifierSize) return {};
  return QString::fromLatin1(id.toHex());
}

auto FscryptIdentifierFromHex(const QString &hex) -> QByteArray {
  if (hex.size() != kFscryptKeyIdentifierSize * 2) return {};

  // Every character checked, rather than trusting the length of what comes
  // back. fromHex() *skips* what it cannot read and pads an odd count, so a
  // string of the right length with one bad character in it still decodes to a
  // full-length identifier -- one naming a key nobody minted. This parses a
  // file on disk and what is done with the result is an eviction, so it has to
  // refuse rather than approximate.
  for (const auto c : hex) {
    const auto ch = c.toLatin1();
    const auto is_hex_digit = (ch >= '0' && ch <= '9') ||
                              (ch >= 'a' && ch <= 'f') ||
                              (ch >= 'A' && ch <= 'F');
    if (!is_hex_digit) return {};
  }

  const auto raw = QByteArray::fromHex(hex.toLatin1());
  if (raw.size() != kFscryptKeyIdentifierSize) return {};

  return raw;
}

}  // namespace GpgFrontend
