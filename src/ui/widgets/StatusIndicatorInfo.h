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

#include "core/profile/Profile.h"
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief What one segment of the main window's status strip says.
 *
 * Kept apart from the widget that paints it: the wording is the part worth
 * asserting, and a test cannot build a widget on the thread it runs on.
 */
struct StatusIndicatorInfo {
  QString caption;  ///< dimmed prefix, empty when the value speaks for itself
  QString value;    ///< the thing the user reads at a glance
  QString tooltip;  ///< the detail behind it, ending in what a click opens
};

/**
 * @brief Describe the profile this window is working in.
 *
 * A profile opened from a file is said to be temporary right in the value: it
 * looks exactly like any other profile from inside the window, and a user who
 * does not know it is a copy has no reason to expect the question on closing.
 *
 * @param kind what sort of profile the session resolved to
 * @param display_name its name, as CurrentProfileDisplayName() gives it
 * @param root_path where the profile lives on this computer
 * @param package_path the .gfp it came out of, for kPACKAGED only
 * @param clickable whether this build offers profiles at all; false drops the
 * "click to manage" hint, which is the only part of the segment that promises
 * something to click, and would otherwise promise it where nothing happens
 * @param key_in_memory whether the profile's own key is held in memory rather
 * than written to that storage. Said separately from storage_label and kept
 * narrow: it is one thing inside the session, and folding the two together
 * would suggest the OpenPGP keys are in memory too, which they are not
 * @param storage_label how the session's storage describes itself, from
 * ProfileAccessor::Label(); empty for a profile that is simply kept here. A
 * packaged session may have fallen back from memory to an ordinary folder, and
 * that is a materially weaker guarantee — showing it is what keeps the fallback
 * a compromise rather than a lie
 * @return caption, value and tooltip for the profile segment
 */
auto GF_UI_EXPORT
DescribeProfileIndicator(ProfileKind kind, const QString& display_name,
                         const QString& root_path, const QString& package_path,
                         bool clickable, const QString& storage_label = {},
                         bool key_in_memory = false) -> StatusIndicatorInfo;

/**
 * @brief Describe the OpenPGP backend behind every operation in this window.
 *
 * @param engine the engine the current channel resolved to
 * @param version its version, empty while the engine is still coming up
 * @param key_db_name name of the key database in use
 * @param key_db_path where that key database lives
 * @return caption, value and tooltip for the engine segment
 */
auto GF_UI_EXPORT DescribeEngineIndicator(OpenPGPEngine engine,
                                          const QString& version,
                                          const QString& key_db_name,
                                          const QString& key_db_path)
    -> StatusIndicatorInfo;

/**
 * @brief Describe whether this session runs portable or installed.
 *
 * Takes the runtime answer, not the build flag: a `--profile` session on a
 * portable build is not a portable session.
 *
 * @param portable_mode whether the session sits on the portable root
 * @param self_contained whether the profile keeps its keys to itself
 * @return caption, value and tooltip for the deployment segment
 */
auto GF_UI_EXPORT DescribeDeploymentIndicator(bool portable_mode,
                                              bool self_contained)
    -> StatusIndicatorInfo;

}  // namespace GpgFrontend::UI
