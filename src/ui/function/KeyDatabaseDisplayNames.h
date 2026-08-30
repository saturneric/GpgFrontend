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

#include "core/model/KeyDatabaseInfo.h"

namespace GpgFrontend::UI {

/**
 * @brief What a key database is, in the words the key list already uses.
 *
 * Shared rather than spelled at each site for the reason SecurityDisplayNames.h
 * is shared: two names for one thing is worse than none, and the database menu
 * and the About dialog are read within a click of each other.
 *
 * @param kind the database's kind
 * @param packed_directory whether its directory is one a package carries
 * @return a translated, user-facing phrase
 */
auto GF_UI_EXPORT KeyDatabaseKindDisplayName(KeyDatabaseKind kind,
                                             bool packed_directory) -> QString;

/**
 * @brief Whether a key database's directory is one a package carries.
 *
 * Deliberately not "is it inside the profile". A database under
 * `@profile/workspace` is inside the profile and still never travels, and the
 * DEFAULT database of a self-contained profile sits at `@profile/db` and does.
 * The kind cannot answer this: ClassifyKeyDatabase() settles DEFAULT on the
 * name alone, because the stored path of that one is replaced on every read.
 *
 * Asked of IsManagedKeyDatabasePath(), the same list the packer uses, so a
 * phrase here cannot promise something RewriteKeyDatabaseListForPacking() would
 * not actually pack.
 *
 * @param path the database's path, absolute or `@profile/...`
 * @param profile_root the root the path is read against
 * @return true when the directory travels with the profile
 */
auto GF_UI_EXPORT KeyDatabaseTravelsWithProfile(const QString& path,
                                                const QString& profile_root)
    -> bool;

}  // namespace GpgFrontend::UI
