/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "core/function/GlobalSettingStation.h"

/**
 * @brief Get the files of a given directory
 *
 * @param _path target directory
 * @return std::vector<std::filesystem::path>
 */
std::vector<std::filesystem::path> get_files_of_directory(
    const std::filesystem::path& _path) {
  namespace fs = std::filesystem;
  std::vector<fs::path> path_list;
  if (!_path.empty()) {
    fs::recursive_directory_iterator end;

    for (fs::recursive_directory_iterator i(_path); i != end; ++i) {
      const fs::path cp = (*i);
      path_list.push_back(cp);
    }
  }
  return path_list;
}