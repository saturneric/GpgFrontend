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

#include "plat/common/PlatformHardening.h"

#include <QtGlobal>

#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <sys/prctl.h>
#include <sys/resource.h>
#endif

namespace GpgFrontend {

auto ApplyProcessHardening() -> ProcessHardening {
  ProcessHardening done;
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
  // On Android, bionic's crash handler makes the process dumpable again
  // before it hands a crash to debuggerd, so tombstones are still written.
  done.not_dumpable = prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) == 0 &&
                      prctl(PR_GET_DUMPABLE, 0, 0, 0, 0) == 0;
  const rlimit none{0, 0};
  done.no_core_dumps = setrlimit(RLIMIT_CORE, &none) == 0;
#endif
  return done;
}

auto HardenProcess() -> ProcessHardening {
#ifdef QT_NO_DEBUG
  return ApplyProcessHardening();
#else
  return {};
#endif
}

}  // namespace GpgFrontend
