/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include <csetjmp>

#include "GpgFrontend.h"

extern jmp_buf recover_env;

void handle_signal(int sig) {
  static int _repeat_handle_num = 1, last_sig = sig;
  LOG(INFO) << "signal caught" << sig;

  if (last_sig == sig)
    _repeat_handle_num++;
  else
    _repeat_handle_num = 1, last_sig = sig;

  if (_repeat_handle_num > 3) {
    LOG(INFO) << "The same signal appears three times, execute the termination "
                 "operation. ";
    exit(-1);
  }

#ifndef WINDOWS
  siglongjmp(recover_env, 1);
#else
  longjmp(recover_env, 1);
#endif
}
