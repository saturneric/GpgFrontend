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

#include "ui/function/KeyDatabaseDisplayNames.h"

#include "core/profile/ProfileAreaTraits.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::UI {

auto KeyDatabaseKindDisplayName(KeyDatabaseKind kind, bool packed_directory)
    -> QString {
  switch (kind) {
    case KeyDatabaseKind::kDEFAULT:
      // The kind alone would say "this computer's" for both, and for a
      // self-contained profile that is the opposite of true: its default
      // database is @profile/db, it travels, and nothing outside the profile
      // can see it.
      return packed_directory
                 ? QObject::tr("This profile's own default key database")
                 : QObject::tr("This computer's default key database");
    case KeyDatabaseKind::kMANAGED:
      return QObject::tr("Kept inside your profile");
    case KeyDatabaseKind::kEXTERNAL:
      return QObject::tr("On this computer only");
  }
  return {};
}

auto KeyDatabaseTravelsWithProfile(const QString& path,
                                   const QString& profile_root) -> bool {
  if (path.isEmpty() || profile_root.isEmpty()) return false;

  // Tokenised first so the long spelling and the `@profile` one are recognised
  // as the same directory; which of them reached the settings file is an
  // accident of which build wrote it.
  const auto relative = ToProfileRelativeKeyDatabasePath(path, profile_root);
  const auto prefix = QLatin1String(kProfilePathToken) + QLatin1String("/");
  if (!relative.startsWith(prefix)) return false;

  return IsManagedKeyDatabasePath(relative.mid(prefix.size()));
}

}  // namespace GpgFrontend::UI
