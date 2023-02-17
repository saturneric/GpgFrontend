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
#include "spdlog/spdlog.h"

GpgFrontend::Thread::TaskRunner::TaskRunner() = default;

GpgFrontend::Thread::TaskRunner::~TaskRunner() = default;

void GpgFrontend::Thread::TaskRunner::PostTask(Task* task) {
  if (task == nullptr) {
    SPDLOG_ERROR("task posted is null");
    return;
  }

  SPDLOG_TRACE("post task: {}", task->GetFullID());

  task->setParent(nullptr);
  task->moveToThread(this);

  connect(task, &Task::SignalTaskPostFinishedDone, this, [&, this, task]() {
    SPDLOG_DEBUG("cleaning task {}", task->GetFullID());
    auto pending_task = pending_tasks_.find(task->GetUUID());
    if (pending_task == pending_tasks_.end()) {
      SPDLOG_ERROR("cannot find task in pending list: {}", task->GetFullID());
      return;
    } else {
      std::lock_guard<std::mutex> lock(tasks_mutex_);
      pending_tasks_.erase(pending_task);
    }
    task->deleteLater();
  });

  {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    tasks.push(task);
  }
  quit();
}

void GpgFrontend::Thread::TaskRunner::PostScheduleTask(Task* task,
                                                       size_t seconds) {
  if (task == nullptr) return;
  // TODO
}

[[noreturn]] void GpgFrontend::Thread::TaskRunner::run() {
  SPDLOG_TRACE("run, thread id: {}", QThread::currentThreadId());
  while (true) {
    SPDLOG_TRACE("task runner: a new cycle start");
    if (tasks.empty()) {
      SPDLOG_TRACE("task runner: no tasks to run, trapping into event loop...");
      exec();
    } else {
      SPDLOG_TRACE("task runner: task queue size:", tasks.size());

      Task* task = nullptr;
      {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        task = std::move(tasks.front());
        tasks.pop();
        pending_tasks_.insert({task->GetUUID(), task});
      }

      if (task != nullptr) {
        try {
          // Run the task
          SPDLOG_TRACE("task runner: running task {}, sequency: {}",
                       task->GetFullID(), task->GetSequency());
          if (task->GetSequency()) {
            // run sequently
            task->run();
          } else {
            // run concurrently
            thread_pool_.start(task);
          }
        } catch (const std::exception& e) {
          SPDLOG_ERROR("task runner: exception in task {}, exception: {}",
                       task->GetFullID(), e.what());
          // destroy the task, remove the task from the pending tasks
          pending_tasks_.erase(task->GetUUID());
          task->deleteLater();
        } catch (...) {
          SPDLOG_ERROR("task runner: unknown exception in task: {}",
                       task->GetFullID());
          // destroy the task, remove the task from the pending tasks
          pending_tasks_.erase(task->GetUUID());
          task->deleteLater();
        }
      }
    }
  }
}
