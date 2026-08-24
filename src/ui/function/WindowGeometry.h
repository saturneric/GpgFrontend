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

namespace GpgFrontend::UI {

/**
 * @brief Shrink and nudge @p rect until it sits inside @p available.
 *
 * A restored window geometry is only as trustworthy as the display layout it
 * was saved under: a monitor that was unplugged, or a resolution that shrank,
 * leaves a stored rect that is partly or wholly off screen. Clamped to 95% of
 * the available area rather than 100% so a maximised-looking window still
 * shows its edges.
 *
 * Pure, so every clamp and nudge is assertable without a screen.
 *
 * @param rect the geometry to correct
 * @param available the screen area it must fit inside
 * @return a rect no larger than 95% of @p available and fully inside it
 */
auto GF_UI_EXPORT ClampRectToAvailableGeometry(QRect rect,
                                               const QRect& available) -> QRect;

}  // namespace GpgFrontend::UI
