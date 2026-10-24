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

#include <QJsonObject>
#include <QList>
#include <QString>

#include "screenshot/ScreenshotEnv.h"

/**
 * @file ScreenshotSeed.h
 * @brief The demo state a render starts from, beyond the built-in keyring:
 *        extra key databases, a key group and a key category.
 *
 * Every id and timestamp here is fixed, because the key list shows them.
 */

namespace GpgFrontend::Screenshot {

/// The id an extra demo database is stored under: a name-based UUID.
auto ScreenshotDatabaseId(const QString& demo_id) -> QString;

/// Where an extra demo database lives, relative to the profile's app data.
auto ScreenshotDatabaseDir(const DemoDatabase& db) -> QString;

/**
 * @brief The key database list to store before startup: the built-in
 *        database first and the default, then each of @p requested as a
 *        managed database inside the profile.
 */
auto BuildKeyDatabaseList(const QList<DemoDatabase>& requested) -> QJsonObject;

/// The demo key group. Its id is shown in the key list, so it is fixed.
constexpr auto kDemoGroupId = "#&5C3EE45A0B0001";
constexpr auto kDemoGroupName = "Project Team";
constexpr auto kDemoCategoryName = "Work Contacts";
constexpr auto kDemoCategoryColor = "#2E86DE";

/**
 * @brief Store the key database list. Before core startup, which opens what
 *        it lists.
 */
auto StoreKeyDatabaseList(const QList<DemoDatabase>& requested) -> bool;

/**
 * @brief Import an rPGP demo database's armored keys into its channel.
 *
 * @return empty on success, otherwise what went wrong
 */
auto ImportDemoKeys(int channel, const QString& directory,
                    const DemoDatabase& db) -> QString;

/**
 * @brief Give the built-in database the demo key group (Alice and Bob) and
 *        a custom category (Bob and Erin).
 *
 * @return empty on success, otherwise what went wrong
 */
auto SeedGroupsAndCategories(int channel, const DemoData& demo,
                             const QDateTime& created) -> QString;

}  // namespace GpgFrontend::Screenshot
