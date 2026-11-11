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

namespace GpgFrontend {
class GpgAbstractKey;
}

namespace GpgFrontend::Shell {

/**
 * @brief Whether @p key may be added to the key group @p group_id.
 *
 * A key group is a set of recipients, so only a key that can encrypt joins
 * one; beyond that the repository's own rules (not the group itself, not a
 * member already, no group nested back into itself) decide.
 *
 * @param channel the key database the group is in
 * @param group_id the group
 * @param key the key or group to add; nullptr is refused
 * @return true when it may be added
 */
auto GF_SHELL_EXPORT CanJoinKeyGroup(int channel, const QString& group_id,
                                     const GpgAbstractKey* key) -> bool;

/**
 * @brief Delete the key group @p group_id, and take it out of every category
 * it was put in, so a later group never inherits them by chance. Its keys,
 * and the groups it was a member of, stay.
 * @return whether the group was deleted
 */
auto GF_SHELL_EXPORT DeleteKeyGroup(int channel, const QString& group_id)
    -> bool;

/// The names of the key groups @p group_id is a member of, which lose it
/// when it is deleted (a group gone from the list by its id).
auto GF_SHELL_EXPORT KeyGroupParentNames(int channel, const QString& group_id)
    -> QStringList;

}  // namespace GpgFrontend::Shell
