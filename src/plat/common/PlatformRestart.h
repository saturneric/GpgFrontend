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

/**
 * @file PlatformRestart.h
 * @brief Start the application again, as a new process.
 *
 * What opening another profile takes on a phone: a profile is chosen before
 * anything else is set up, and is not changed under a running core. The
 * caller has shut the core down first; on Android this process is then ended
 * from outside it, and the launcher's activity comes up in a new one.
 */

#include <QtGlobal>

namespace GpgFrontend {

/// Begin the restart; the caller quits once it returns true.
auto RestartApplication() -> bool;

/// Leave the screen, the application left running as it is, as Back from
/// the first screen does on Android (12 and later). False where there is no
/// such thing (a desktop window), and the caller closes instead.
auto SendToBackground() -> bool;

}  // namespace GpgFrontend
