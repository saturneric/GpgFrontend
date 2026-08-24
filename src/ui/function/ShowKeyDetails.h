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
 * @brief Open the details dialog that matches what @p key actually is.
 *
 * A key list holds ordinary keys and key groups side by side, and each needs a
 * different dialog. Dispatching here rather than at every double-click and
 * context menu keeps the two dialog headers -- and their transitive includes --
 * out of every widget that can show a key.
 *
 * @param parent dialog parent
 * @param channel key database / engine channel the key lives in
 * @param key the key or key group to show; a null key reports an error
 */
void GF_UI_EXPORT ShowKeyDetails(QWidget* parent, int channel,
                                 const GpgAbstractKeyPtr& key);

}  // namespace GpgFrontend::UI
