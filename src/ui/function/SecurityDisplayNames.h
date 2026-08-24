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

#include "core/profile/ProfileSecureKeyManager.h"

namespace GpgFrontend::UI {

/**
 * @brief Human name of a memory-hardening secure level.
 *
 * Shared by the Advanced tab and the About dialog so the two cannot drift
 * apart, which is how their labels came to disagree in the first place.
 *
 * @param level the GFSecureLevel value, 0..3
 * @return a translated, user-facing name
 */
auto GF_UI_EXPORT SecureLevelDisplayName(int level) -> QString;

/**
 * @brief Human name of an application key protection mode.
 *
 * @param protection resolved protection, as stored in GFAppKeyProtection
 * @return a translated, user-facing name
 */
auto GF_UI_EXPORT AppKeyProtectionDisplayName(AppKeyProtection protection)
    -> QString;

}  // namespace GpgFrontend::UI
