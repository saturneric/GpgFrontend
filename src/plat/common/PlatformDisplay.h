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

#include <QtGlobal>

/**
 * @file PlatformDisplay.h
 * @brief What the system's display settings ask of the application's text.
 *
 * On Android Qt gives every application the same default font whatever the
 * system's font size, so the phone application scales its own type by the
 * system's font scale (Configuration.fontScale). Elsewhere it is 1.
 */

namespace GpgFrontend {

/// The system's font scale: 1 at the default text size, more for larger.
auto SystemFontScale() -> qreal;

/// Take away the launch screen the system shows until the application has
/// drawn (on Android, the activity's gf_splash, kept up over Qt's surface so
/// nothing white or black shows before the loading page). It fades into
/// what is drawn now. Nothing elsewhere.
void HideLaunchScreen();

}  // namespace GpgFrontend
