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
 * @file PlatformHardening.h
 * @brief Keeping other processes out of this one's memory.
 *
 * Linux and Android only: a process that is not dumpable cannot be attached
 * to with ptrace, nor its /proc/<pid>/mem read, by another process of the
 * same user (root aside), and a crash writes no core file holding its keys
 * and passphrases. Elsewhere nothing is done.
 */

namespace GpgFrontend {

/// What hardening the process turned off.
struct ProcessHardening {
  /// No longer dumpable: no ptrace attach, no /proc/<pid>/mem, no core dump.
  bool not_dumpable = false;
  /// The core file size limit is 0, soft and hard.
  bool no_core_dumps = false;
};

/**
 * @brief Harden the process, in a release build only.
 *
 * A debug build is left as it is, so a debugger can still attach (Android
 * Studio attaches with ptrace). Call it first thing in main(), before any
 * secret is in memory.
 */
auto HardenProcess() -> ProcessHardening;

/// Harden the process whatever the build: what HardenProcess() does in a
/// release build. For tests, in a child process.
auto ApplyProcessHardening() -> ProcessHardening;

}  // namespace GpgFrontend
