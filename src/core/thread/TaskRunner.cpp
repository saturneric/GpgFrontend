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

#include "core/thread/TaskRunner.h"

#include <qobjectdefs.h>
#include <qthread.h>
#include <qthreadpool.h>

#include <memory>

#include "core/thread/Task.h"
#include "spdlog/spdlog.h"

namespace GpgFrontend::Thread {

class TaskRunner::Impl : public QThread {
 public:
  void PostTask(Task* task) {
    if (task == nullptr) {
      SPDLOG_ERROR("task posted is null");
      return;
    }

    task->setParent(nullptr);

    if (task->GetSequency()) {
      SPDLOG_TRACE("post task: {}, sequency mode: {}", task->GetFullID(),
                   task->GetSequency());
      task->moveToThread(this);
    } else {
      if (pool_.tryStart(task)) {
        SPDLOG_TRACE("runner's pool starts concurrent task {} immediately",
                     task->GetFullID());
      } else {
        SPDLOG_TRACE("runner's pool will start concurrent task {} later",
                     task->GetFullID());
      }
    }
    emit task->SignalRun();
  }

  void PostScheduleTask(Task* task, size_t seconds) {
    if (task == nullptr) return;
    // TODO
  }

 private:
  QThreadPool pool_;
};

GpgFrontend::Thread::TaskRunner::TaskRunner() : p_(std::make_unique<Impl>()) {}

GpgFrontend::Thread::TaskRunner::~TaskRunner() = default;

void GpgFrontend::Thread::TaskRunner::PostTask(Task* task) {
  p_->PostTask(task);
}

void TaskRunner::PostScheduleTask(Task* task, size_t seconds) {
  p_->PostScheduleTask(task, seconds);
}

void TaskRunner::Start() { p_->start(); }

QThread* TaskRunner::GetThread() { return p_.get(); }

bool TaskRunner::IsRunning() { return p_->isRunning(); }

}  // namespace GpgFrontend::Thread
