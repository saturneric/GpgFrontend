/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "ArchiveDirectory.h"

#include "core/function/ArchiveFileOperator.h"

namespace GpgFrontend::UI {

auto PathPreCheck(QWidget* parent, const std::filesystem::path& path) -> bool {
  QFileInfo const file_info(path);
  QFileInfo const path_info(file_info.absolutePath());

  if (!path_info.exists()) {
    QMessageBox::critical(
        parent, _("Error"),
        QString(_("The path %1 does not exist.")).arg(path.c_str()));
    return false;
  }

  if (!path_info.isDir()) {
    QMessageBox::critical(
        parent, _("Error"),
        QString(_("The path %1 is not a directory.")).arg(path.c_str()));
    return false;
  }

  if (!file_info.isReadable()) {
    QMessageBox::critical(parent, _("Error"),
                          _("No permission to read this file."));
    return false;
  }
  return true;
}

ArchiveDirectory::ArchiveDirectory(QWidget* parent) : QWidget(parent) {}

auto ArchiveDirectory::Exec(const std::filesystem::path& target_directory)
    -> std::tuple<bool, GFBuffer> {
  if (!PathPreCheck(this, target_directory)) {
    return {false, {}};
  }

  try {
    auto base_path = target_directory.parent_path();
    auto target_path = target_directory;
    target_path = target_path.replace_extension("");

    GF_UI_LOG_DEBUG("archive directory, base path: {}, target path: {}",
                    base_path.string(), target_path.string());

    // ArchiveFileOperator::CreateArchive(base_path, target_path, 0, );

  } catch (...) {
    GF_UI_LOG_ERROR("archive caught exception error");
    return {false, {}};
  }
  return {true, {}};
}

}  // namespace GpgFrontend::UI