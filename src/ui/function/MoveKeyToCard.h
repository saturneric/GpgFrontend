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

#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief Drive the full "move a (sub)key onto a smart card" user flow.
 *
 * Shared by the key-details subkey tab and the Smart Card controller so both
 * behave identically: it warns about the destructive nature of `keytocard`,
 * offers to export a secret-key backup first, resolves the target card slot
 * (auto when the key has a single capability, otherwise asks), resolves the
 * target card serial (uses @p preselected_serial when non-empty, otherwise asks
 * from the inserted cards), performs the move via
 * GpgSmartCardManager::MoveKeyToCard, and refreshes the key database on
 * success.
 *
 * @param parent dialog parent for the prompts
 * @param channel key database / engine channel the key lives in
 * @param key the key whose (sub)key is being moved
 * @param subkey_index index into key->SubKeys() (0 is the primary)
 * @param preselected_serial target card serial, or empty to ask the user
 * @return true if the key was moved successfully
 */
auto GF_UI_EXPORT MoveKeyToCardInteractive(QWidget* parent, int channel,
                                           const GpgKeyPtr& key,
                                           int subkey_index,
                                           const QString& preselected_serial)
    -> bool;

}  // namespace GpgFrontend::UI
