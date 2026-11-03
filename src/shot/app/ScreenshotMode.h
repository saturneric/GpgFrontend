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

#include <QCommandLineParser>
#include <optional>

#include "GpgFrontendContext.h"
#include "core/profile/Profile.h"

/**
 * @file ScreenshotMode.h
 * @brief The hidden screenshot mode's three hooks into main().
 *
 * Only compiled with GPGFRONTEND_BUILD_SCREENSHOT_MODE. Every option it adds
 * is hidden from --help.
 */

namespace GpgFrontend::Screenshot {

/**
 * @brief Before the QApplication exists: pin scale, DPI and timezone when a
 *        screenshot run is asked for, and answer --screenshot-list.
 *
 * @return an exit code when the process should end here
 */
auto PrepareScreenshotEnvironment(int argc, char** argv) -> std::optional<int>;

/**
 * @brief Register the hidden options on the application's parser.
 */
void AddScreenshotOptions(QCommandLineParser& parser);

/**
 * @brief The profile to mount: MakeProfile(@p selection), except that the
 *        temporary profile of a render or probe gets a fixed anchor, so the
 *        paths the application shows (a key database's location, say) are
 *        the same on every run.
 */
auto MakeScreenshotProfile(const ProfileSelection& selection,
                           const QCommandLineParser& parser)
    -> QSharedPointer<Profile>;

/**
 * @brief Whether the parsed command line asks for a render or a probe.
 */
auto IsScreenshotRun(const QCommandLineParser& parser) -> bool;

/**
 * @brief Run one render or environment probe to completion.
 *
 * Called after the profile is open, in place of the normal startup.
 */
auto RunScreenshotMode(const GFCxtSPtr& ctx, const QCommandLineParser& parser)
    -> int;

}  // namespace GpgFrontend::Screenshot
