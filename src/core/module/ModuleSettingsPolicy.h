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

#include <QString>

#include "GFCoreExport.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleSettingsPolicy.h
 * @brief Which application settings a module may read and write.
 *
 * A module used to be handed the application's QSettings object and could
 * read or overwrite anything in it -- another module's accounts, the Host's
 * own configuration. Now it names a key, and the Host decides where that key
 * lives: in the module's own group, or -- for the few Host settings a module
 * has a real reason to share -- in an explicit allowlist.
 */

/// Where a key is looked up.
enum class ModuleSettingScope {
  kMODULE = 0,  ///< the module's own group, invisible to other modules
  kHOST = 1,    ///< a Host setting on the allowlist, by its full name
};

/// What a module may do with one Host setting.
enum class HostSettingAccess { kNONE, kREAD, kREAD_WRITE };

/**
 * @brief The settings group a module's own keys live under.
 *
 * Two modules keep the group they always used, so their users' settings are
 * found where they already are; every other module is under
 * `modules/<module id>`.
 */
auto GF_CORE_EXPORT ModuleSettingsGroup(const QString& module_id) -> QString;

/// The access a module has to Host setting @p key; kNONE unless listed.
auto GF_CORE_EXPORT HostSettingAccessFor(const QString& key)
    -> HostSettingAccess;

/**
 * @brief Whether @p key is a well-formed relative key.
 *
 * No leading or trailing separator, no empty segment, no "..", and only
 * `[A-Za-z0-9_.-]` between separators: a module's key can name a place
 * inside its own group and nowhere else.
 */
auto GF_CORE_EXPORT IsValidModuleSettingKey(const QString& key) -> bool;

/**
 * @brief The full QSettings key for @p key in @p scope, or empty when the
 *        module may not use it at all.
 *
 * @param write whether the caller intends to write; a read-only Host
 *        setting resolves for a read only
 */
auto GF_CORE_EXPORT ResolveModuleSettingKey(const QString& module_id,
                                            ModuleSettingScope scope,
                                            const QString& key, bool write)
    -> QString;

}  // namespace GpgFrontend::Module
