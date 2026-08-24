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

#include "ui/function/FilePanelPath.h"

#include "core/function/GlobalSettingStation.h"
#include "core/profile/ProfileSession.h"

namespace GpgFrontend::UI {

auto FilePanelDefaultPathModeToString(FilePanelDefaultPathMode mode)
    -> QString {
  switch (mode) {
    case FilePanelDefaultPathMode::kWORKSPACE:
      return "workspace";
    case FilePanelDefaultPathMode::kHOME:
      return "home";
    case FilePanelDefaultPathMode::kCWD:
      return "cwd";
  }
  return "home";
}

auto FilePanelDefaultPathModeFromString(const QString& s)
    -> FilePanelDefaultPathMode {
  const auto v = s.trimmed().toLower();
  if (v == "workspace") return FilePanelDefaultPathMode::kWORKSPACE;
  if (v == "cwd") return FilePanelDefaultPathMode::kCWD;
  return FilePanelDefaultPathMode::kHOME;
}

auto ResolveFilePanelDefaultPath(FilePanelDefaultPathMode mode,
                                 const QString& workspace_path,
                                 const QString& home_path,
                                 const QString& cwd_path) -> QString {
  switch (mode) {
    case FilePanelDefaultPathMode::kWORKSPACE:
      // an unresolvable workspace falls back rather than opening nothing: the
      // file panel with no root at all is worse than the old default
      return workspace_path.isEmpty() ? home_path : workspace_path;
    case FilePanelDefaultPathMode::kCWD:
      return cwd_path;
    case FilePanelDefaultPathMode::kHOME:
      break;
  }
  return home_path;
}

auto GetDefaultUserFilePath() -> QString {
  const auto mode = FilePanelDefaultPathModeFromString(
      GetSettings().value("basic/file_panel_default_path_mode").toString());

  return ResolveFilePanelDefaultPath(mode,
                                     ProfileSession::Instance().WorkspacePath(),
                                     QDir::homePath(), QDir::currentPath());
}

}  // namespace GpgFrontend::UI
