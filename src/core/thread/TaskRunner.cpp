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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "core/thread/TaskRunner.h"

#include "core/thread/Task.h"
#include "easylogging++.h"

GpgFrontend::Thread::TaskRunner::TaskRunner() = default;

GpgFrontend::Thread::TaskRunner::~TaskRunner() = default;

void GpgFrontend::Thread::TaskRunner::PostTask(Task* task) {
  LOG(INFO) << "called";
  if (task == nullptr) return;
  task->setParent(nullptr);
  task->moveToThread(this);
  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks.push(task);
  }
  quit();
}

void GpgFrontend::Thread::TaskRunner::run() {
  LOG(INFO) << "called";
  while (true) {
    if (tasks.empty()) {
      LOG(INFO) << "TaskRunner: No tasks to run";
      exec();
    } else {
      LOG(INFO) << "TaskRunner: Running task, queue size:" << tasks.size();

      Task* task = nullptr;
      {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        task = std::move(tasks.front());
        tasks.pop();
      }
      if (task != nullptr) task->run();
    }
  }
}
