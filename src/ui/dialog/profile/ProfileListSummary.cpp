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

#include "ui/dialog/profile/ProfileListSummary.h"

#include "ui/function/DateTimeText.h"
#include "ui/function/ProfileController.h"

namespace GpgFrontend::UI {

auto ProfileRowViewFor(const ProfileRegistryEntry& entry,
                       const QString& current_id, bool root_exists,
                       bool held_elsewhere) -> ProfileRowView {
  ProfileRowView row;
  row.name = entry.name;
  row.type = ProfileKindDisplayName(entry.kind);
  row.last_opened = ProfileLastOpenedText(entry.last_opened);
  row.is_current = entry.id == current_id;

  // "in use elsewhere" is worth showing here rather than only on a failed
  // switch: it explains the refusal before the user runs into it
  if (row.is_current) {
    row.status_kind = ProfileRowStatus::kThisWindow;
    row.status = QObject::tr("This window");
  } else if (!root_exists) {
    row.status_kind = ProfileRowStatus::kNotCreated;
    row.status = QObject::tr("Not created yet");
  } else if (held_elsewhere) {
    row.status_kind = ProfileRowStatus::kOpenElsewhere;
    row.status = QObject::tr("Open in another window");
  }

  row.can_open = !row.is_current;
  row.can_reveal = root_exists;

  // Classic and portable are resolved by the application itself, so there is
  // nothing coherent for "delete" to mean; the current profile cannot be
  // deleted out from under the process that is using it.
  row.can_delete = !entry.implicit && !row.is_current;
  return row;
}

auto ProfileLastOpenedText(const QString& iso, const QLocale& locale)
    -> QString {
  const auto stamp = QDateTime::fromString(iso, Qt::ISODate);
  if (!stamp.isValid()) return QObject::tr("Never");
  return ShortDateTimeText(stamp.toLocalTime(), locale);
}

}  // namespace GpgFrontend::UI
