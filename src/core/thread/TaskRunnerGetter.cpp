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

#include "core/thread/TaskRunnerGetter.h"

#include "thread/TaskRunner.h"

namespace GpgFrontend::Thread {

TaskRunnerGetter::TaskRunnerGetter(int channel)
    : SingletonFunctionObject<TaskRunnerGetter>(channel) {}

TaskRunnerPtr GpgFrontend::Thread::TaskRunnerGetter::GetTaskRunner(
    TaskRunnerType runner_type) {
  while (true) {
    auto it = task_runners_.find(runner_type);
    if (it != task_runners_.end()) {
      return it->second;
    } else {
      auto runner = GpgFrontend::SecureCreateSharedObject<TaskRunner>();
      task_runners_[runner_type] = runner;
      runner->Start();
      continue;
    }
  }
}
}  // namespace GpgFrontend::Thread