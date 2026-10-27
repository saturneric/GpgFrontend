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

#pragma once

#include "core/profile/ProfileRegistry.h"

namespace GpgFrontend::UI {

/**
 * @brief What the Status column says about a profile.
 */
enum class ProfileRowStatus {
  kNone,
  kThisWindow,     ///< the profile this process is running
  kOpenElsewhere,  ///< another process holds its lock
  kNotCreated,     ///< registered, but its folder does not exist yet
};

/**
 * @brief One row of the profile list, as the dialog shows it.
 *
 * Kept free of widgets so the rules for what a row says and allows can be
 * tested without a dialog. The dialog probes the filesystem and the lock and
 * hands the answers in; nothing here touches the disk.
 */
struct GF_UI_EXPORT ProfileRowView {
  QString name;
  QString type;
  QString last_opened;
  QString status;
  ProfileRowStatus status_kind = ProfileRowStatus::kNone;

  bool is_current = false;
  bool can_open = false;
  bool can_reveal = false;
  bool can_delete = false;
};

/**
 * @brief Describe one registry entry as a row.
 *
 * @param entry the entry
 * @param current_id the id of the profile this process is running
 * @param root_exists whether the entry's folder exists
 * @param held_elsewhere whether another process holds the entry's lock
 * @return the row
 */
auto GF_UI_EXPORT ProfileRowViewFor(const ProfileRegistryEntry& entry,
                                    const QString& current_id, bool root_exists,
                                    bool held_elsewhere) -> ProfileRowView;

/**
 * @brief A stamped last-opened time as a table cell shows it.
 *
 * The registry keeps ISO-8601 in UTC; a reader wants their own clock and
 * their own locale's order of day and month.
 *
 * @param iso the stamp, possibly empty
 * @param locale the locale to format in
 * @return the local time, or "Never" when there is no usable stamp
 */
auto GF_UI_EXPORT ProfileLastOpenedText(const QString& iso,
                                        const QLocale& locale = QLocale())
    -> QString;

}  // namespace GpgFrontend::UI
