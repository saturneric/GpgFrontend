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
 * @brief Where the file panel starts, and where user-file dialogs default to.
 *
 * Note the deliberate distance from `basic/default_workspace_as`, which decides
 * which *view* opens at startup and has nothing to do with any of this. The
 * user-facing name for the directory below is "Profile Workspace".
 */
enum class FilePanelDefaultPathMode {
  kWORKSPACE,  ///< the profile's workspace directory
  kHOME,       ///< the user's home directory
  kCWD,        ///< the process working directory
};

auto GF_UI_EXPORT
FilePanelDefaultPathModeToString(FilePanelDefaultPathMode mode) -> QString;

auto GF_UI_EXPORT FilePanelDefaultPathModeFromString(const QString& s)
    -> FilePanelDefaultPathMode;

/**
 * @brief Resolve the mode to an actual directory.
 *
 * Pure, so all three modes are assertable without a profile or a home
 * directory.
 *
 * @param mode which directory to use
 * @param workspace_path the profile workspace
 * @param home_path the user's home directory
 * @param cwd_path the process working directory
 * @return the directory to open
 */
auto GF_UI_EXPORT ResolveFilePanelDefaultPath(FilePanelDefaultPathMode mode,
                                              const QString& workspace_path,
                                              const QString& home_path,
                                              const QString& cwd_path)
    -> QString;

/**
 * @brief The directory a file dialog for *user* files should start in.
 *
 * There is no other shared helper for this: every QFileDialog call site used to
 * hardcode a home directory, a bare filename, or nothing at all, so none of
 * them agreed with the file panel or with each other.
 *
 * Only for user files. Dialogs that pick a system location -- a GnuPG
 * installation directory, a key database -- are asking a different question and
 * should not be redirected into the workspace.
 *
 * @return absolute path
 */
auto GF_UI_EXPORT GetDefaultUserFilePath() -> QString;

}  // namespace GpgFrontend::UI
