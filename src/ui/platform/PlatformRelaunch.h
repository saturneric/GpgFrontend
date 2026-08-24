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

#ifdef Q_OS_MACOS
/**
 * @brief Start another instance of this application bundle.
 *
 * macOS only, and implemented against NSWorkspace rather than by running the
 * executable: that is what gives the new instance its own Dock entry and its
 * own activation. Both the deep restart and "open a second window" come back
 * through here.
 *
 * It lives in the UI library rather than beside the application's other
 * platform sources because gf_ui itself calls it, and a library cannot resolve
 * a symbol that only the executable defines.
 *
 * @param arguments arguments for the new instance, argv[0] excluded
 * @return whether the launch was handed to the workspace
 */
auto GF_UI_EXPORT RelaunchApplication(const QStringList& arguments) -> bool;
#endif

}  // namespace GpgFrontend::UI
