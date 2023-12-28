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

#pragma once

#include "core/GpgFrontendCore.h"
#include "core/typedef/CoreTypedef.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT ArchiveFileOperator {
 public:
  /**
   * @brief
   *
   * @param archive_path
   */
  static void ListArchive(const std::filesystem::path &archive_path);

  /**
   * @brief Create a Archive object
   *
   * @param base_path
   * @param archive_path
   * @param compress
   * @param files
   */
  static void NewArchive2Fd(const std::filesystem::path &target_directory,
                            int fd, const OperationCallback &cb);

  /**
   * @brief
   *
   * @param archive_path
   * @param base_path
   */
  static void ExtractArchiveFromFd(int fd,
                                   const std::filesystem::path &target_path,
                                   const OperationCallback &cb);
};
}  // namespace GpgFrontend
