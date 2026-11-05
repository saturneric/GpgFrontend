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

#include "GFShellExport.h"
#include "core/model/GpgAbstractKey.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/typedef/GpgTypedef.h"

// The rules a key database's name follows.

namespace GpgFrontend {

/**
 * @brief Whether @p name uses only kKeyDatabaseNamePattern's characters.
 */
auto GF_SHELL_EXPORT IsKeyDatabaseNameText(const QString& name) -> bool;

/**
 * @brief Whether a database in @p infos already answers to @p name, by its
 * stored name or the one it is shown under, compared case-insensitively:
 * "Work" and "work" are one folder on some systems.
 */
auto GF_SHELL_EXPORT IsKeyDatabaseNameTaken(const QString& name,
                                            const QList<KeyDatabaseInfo>& infos)
    -> bool;

/**
 * @brief The first "<prefix> <n>" no database in @p infos is called, and, when
 * @p app_data_path is given, whose managed folder is not there already (a
 * removed database's, kept in the Recycle Bin).
 *
 * Not translated, deliberately: the name becomes a folder.
 */
auto GF_SHELL_EXPORT SuggestKeyDatabaseName(const QString& prefix,
                                            const QList<KeyDatabaseInfo>& infos,
                                            const QString& app_data_path = {})
    -> QString;

}  // namespace GpgFrontend
