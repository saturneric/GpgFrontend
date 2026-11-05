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

#include "GFShellExport.h"

namespace GpgFrontend::Shell {

/**
 * @brief What is known about a file operation's output once it ended.
 */
struct GF_SHELL_EXPORT FileOutputFacts {
  bool cancelled = false;
  /// GpgOpResultInfo::OutputReleasable(): making the output worked, whatever
  /// a signature checked on the way said.
  bool releasable = false;
  bool exists = false;  ///< the output file is there
  qint64 size = 0;      ///< its size in bytes
  /// Decrypting an empty file rightly writes nothing; encrypting or signing
  /// always writes something.
  bool may_be_empty = false;
};

/**
 * @brief Whether a file operation's output is kept and offered.
 *
 * Decided by how making the output went, not by the operation's overall
 * status: a file whose signature fails still decrypted, and the person gets
 * it with the failure reported, as on the desktop.
 *
 * @param facts what is known about the output
 * @return true when the output is kept
 */
auto GF_SHELL_EXPORT KeepsFileOutput(const FileOutputFacts& facts) -> bool;

}  // namespace GpgFrontend::Shell
