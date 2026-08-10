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

#include "platform/LibSecretSearchPath.h"

#include <QDir>

namespace GpgFrontend {

auto LibSecretSearchPaths(const QString& app_dir, const QString& override_path)
    -> QStringList {
  QStringList candidates;
  const auto add = [&candidates](const QString& path) {
    if (!path.isEmpty() && !candidates.contains(path)) candidates << path;
  };

  // The escape hatch: the only way to try a fix on someone else's machine
  // without building them a new bundle first.
  add(override_path.trimmed());

  // Keyed on $APPDIR rather than $APPIMAGE on purpose: $APPIMAGE is unset once
  // the image has been unpacked with --appimage-extract and started through
  // AppRun, which is exactly how this gets debugged.
  if (QDir::isAbsolutePath(app_dir)) {
    add(QDir::cleanPath(app_dir + QStringLiteral("/usr/lib/") +
                        QLatin1String(kLibSecretSoname)));
  }

  // Every ordinary install, and the fallback when a bundled copy is the one at
  // fault. Left to the loader rather than spelled out as /usr/lib/<triplet>:
  // ld.so.cache already covers those, and Qt's name for an architecture is not
  // the same as Debian's.
  add(QLatin1String(kLibSecretSoname));

  return candidates;
}

}  // namespace GpgFrontend
