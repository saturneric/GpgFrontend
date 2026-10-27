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

#include "ui/struct/settings_object/KeyGenSO.h"

namespace GpgFrontend::UI {

/**
 * @brief The settings as they should be, from what is stored.
 *
 * Profiles never written to the settings come from the cache the old dialog
 * kept them in, and failing that from the defaults.
 *
 * @param stored the settings object's content
 * @param legacy the old cache's profile array; empty when there is none
 * @return the settings, with `profiles_stored` saying whether they changed
 */
auto GF_UI_EXPORT KeyGenSettingsFromStored(const QJsonObject& stored,
                                           const QJsonArray& legacy)
    -> KeyGenSO;

/**
 * @brief The key generation settings, migrating them on first use.
 */
auto GF_UI_EXPORT LoadKeyGenSettings() -> KeyGenSO;

/**
 * @brief Store the key generation settings.
 */
void GF_UI_EXPORT StoreKeyGenSettings(const KeyGenSO& settings);

/**
 * @brief Remember whether the key dialogs open in Expert Mode.
 *
 * Only that one value: a dialog holding an older copy of the settings must not
 * write the rest of it back.
 */
void GF_UI_EXPORT StoreKeyGenExpertMode(bool on);

/**
 * @brief The profiles a channel's engine can generate, in stored order.
 */
auto GF_UI_EXPORT OfferedKeyGenProfiles(int channel,
                                        const QList<KeyGenProfile>& profiles)
    -> QList<ResolvedKeyGenProfile>;

}  // namespace GpgFrontend::UI
