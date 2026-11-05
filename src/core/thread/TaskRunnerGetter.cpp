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

#include "core/thread/TaskRunnerGetter.h"

#include <mutex>

#include "core/GFConstants.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Thread {

TaskRunnerGetter::TaskRunnerGetter(int)
    : SingletonFunctionObject<TaskRunnerGetter>(kGpgFrontendDefaultChannel) {}

auto TaskRunnerGetter::GetTaskRunner(TaskRunnerType runner_type)
    -> TaskRunnerPtr {
  std::lock_guard<std::mutex> lock_guard(task_runners_map_lock_);
  while (true) {
    auto it = task_runners_.find(runner_type);
    if (it != task_runners_.end()) {
      return it->second;
    }

    auto runner = GpgFrontend::SecureCreateSharedObject<TaskRunner>();
    task_runners_[runner_type] = runner;
    runner->Start();
  }
}

auto TaskRunnerGetter::StopAllTeakRunner(int timeout_ms) -> bool {
  // Copied under the lock and stopped outside it: a task finishing up may
  // still ask for a runner, and would wait on this lock while the stop waits
  // on it.
  std::map<TaskRunnerType, TaskRunnerPtr> runners;
  {
    std::lock_guard<std::mutex> lock_guard(task_runners_map_lock_);
    runners = task_runners_;
  }

  const QDeadlineTimer deadline(timeout_ms);
  bool all_stopped = true;
  for (const auto& [key, value] : runners) {
    LOG_D() << "Stopping task runner: " << static_cast<int>(key);
    if (!value->IsRunning()) continue;
    if (!value->Stop(static_cast<int>(deadline.remainingTime()))) {
      LOG_W() << "task runner" << static_cast<int>(key)
              << "did not stop: a task on it is still running";
      all_stopped = false;
    }
  }
  return all_stopped;
}

}  // namespace GpgFrontend::Thread