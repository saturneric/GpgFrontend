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

#include <exception>

#include "core/thread/Task.h"
#include "easylogging++.h"

GpgFrontend::Thread::TaskRunner::TaskRunner() = default;

GpgFrontend::Thread::TaskRunner::~TaskRunner() = default;

void GpgFrontend::Thread::TaskRunner::PostTask(Task* task) {
  std::string uuid = task->GetUUID();
  LOG(TRACE) << "Post Task" << uuid;

  if (task == nullptr) return;
  task->setParent(nullptr);
  task->moveToThread(this);

  connect(task, &Task::SignalTaskPostFinishedDone, this, [&, uuid]() {
    auto it = pending_tasks_.find(uuid);
    if (it == pending_tasks_.end()) {
      return;
    } else {
      it->second->deleteLater();
      pending_tasks_.erase(it);
    }
  });
  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks.push(task);
  }
  quit();
}

void GpgFrontend::Thread::TaskRunner::run() {
  LOG(TRACE) << "called"
             << "thread id:" << QThread::currentThreadId();
  while (true) {
    LOG(TRACE) << "TaskRunner: A new cycle start";
    if (tasks.empty()) {
      LOG(TRACE) << "TaskRunner: No tasks to run, trapping into event loop...";
      exec();
    } else {
      LOG(TRACE) << "TaskRunner: Task queue size:" << tasks.size();

      Task* task = nullptr;
      {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        task = std::move(tasks.front());
        tasks.pop();
        pending_tasks_.insert({task->GetUUID(), task});
      }

      if (task != nullptr) {
        // Run the task
        LOG(TRACE) << "TaskRunner: Running Task" << task->GetUUID();
        try {
          task->run();
        } catch (const std::exception& e) {
          LOG(ERROR) << "TaskRunner: Exception in Task" << task->GetUUID()
                     << "Exception: " << e.what();

          // destroy the task, remove the task from the pending tasks
          task->deleteLater();
          pending_tasks_.erase(task->GetUUID());
        } catch (...) {
          LOG(ERROR) << "TaskRunner: Unknwon Exception in Task"
                     << task->GetUUID();

          // destroy the task, remove the task from the pending tasks
          task->deleteLater();
          pending_tasks_.erase(task->GetUUID());
        }
      }
    }
  }
}
