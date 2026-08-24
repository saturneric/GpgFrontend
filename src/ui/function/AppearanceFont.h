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
 * @brief The font a text surface should use for a stored appearance setting.
 *
 * Starts from the system's fixed-pitch font and only takes @p family over it
 * when that family is actually installed: a font that was uninstalled since it
 * was chosen must fall back to something readable rather than let Qt
 * substitute an arbitrary family.
 *
 * @param family stored family name, empty to keep the system fixed-pitch font
 * @param point_size point size to apply
 * @return the resolved font
 */
auto GF_UI_EXPORT ResolveAppearanceFont(const QString& family, int point_size)
    -> QFont;

/**
 * @brief Whether @p family is a monospaced family.
 *
 * Wraps the version split in QFontDatabase: Qt 6 made its query functions
 * static, while on Qt 5 they are members that need an instance.
 *
 * @param family family name to query
 * @return true when the family is fixed pitch
 */
auto GF_UI_EXPORT IsFixedPitchFontFamily(const QString& family) -> bool;

}  // namespace GpgFrontend::UI
