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

#include <csetjmp>

#include "GpgFrontend.h"

#ifdef FREEBSD
extern sigjmp_buf recover_env;
#else
extern jmp_buf recover_env;
#endif

/**
 * @brief handle the signal caught.
 *
 * @param sig signal number
 */
void handle_signal(int sig) {
  static int _repeat_handle_num = 1, last_sig = sig;
  // SPDLOG_DEBUG("signal caught {}", sig);
  std::cout << "signal caught" << sig;

  if (last_sig == sig)
    _repeat_handle_num++;
  else
    _repeat_handle_num = 1, last_sig = sig;

  if (_repeat_handle_num > 3) {
      std::cout << "The same signal appears three times,"
      << "execute the termination operation." << sig;
    exit(-1);
  }

#ifndef WINDOWS
  siglongjmp(recover_env, 1);
#else
  longjmp(recover_env, 1);
#endif
}
