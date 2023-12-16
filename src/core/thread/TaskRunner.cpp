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

#include <qobject.h>
#include <qobjectdefs.h>
#include <qthread.h>
#include <qthreadpool.h>

#include "core/thread/Task.h"

namespace GpgFrontend::Thread {

class TaskRunner::Impl : public QThread {
 public:
  Impl() {
    SPDLOG_TRACE("task runner created, thread id: {}",
                 QThread::currentThread()->currentThreadId());
  }

  void PostTask(Task* task) {
    if (task == nullptr) {
      SPDLOG_ERROR("task posted is null");
      return;
    }

    task->setParent(nullptr);
    task->moveToThread(this);

    SPDLOG_TRACE("runner starts task: {}", task->GetFullID());
    task->SafelyRun();
  }

  void PostConcurrentTask(Task* task) {
    if (task == nullptr) {
      SPDLOG_ERROR("task posted is null");
      return;
    }

    auto* concurrent_thread = new QThread(this);

    task->setParent(nullptr);
    task->moveToThread(concurrent_thread);

    connect(task, &Task::SignalTaskEnd, concurrent_thread, &QThread::quit);
    connect(concurrent_thread, &QThread::finished, concurrent_thread,
            &QThread::deleteLater);

    concurrent_thread->start();

    SPDLOG_TRACE("runner starts task concurrenctly: {}", task->GetFullID());
    task->SafelyRun();
  }

  void PostScheduleTask(Task* task, size_t seconds) {
    if (task == nullptr) return;
    // TODO
  }
};

GpgFrontend::Thread::TaskRunner::TaskRunner()
    : p_(SecureCreateUniqueObject<Impl>()) {}

GpgFrontend::Thread::TaskRunner::~TaskRunner() {
  if (p_->isRunning()) {
    Stop();
  }
}

void GpgFrontend::Thread::TaskRunner::PostTask(Task* task) {
  p_->PostTask(task);
}

void GpgFrontend::Thread::TaskRunner::PostConcurrentTask(Task* task) {
  p_->PostConcurrentTask(task);
}

void TaskRunner::PostScheduleTask(Task* task, size_t seconds) {
  p_->PostScheduleTask(task, seconds);
}

void TaskRunner::Start() { p_->start(); }

void TaskRunner::Stop() {
  p_->quit();
  p_->wait();
}

auto TaskRunner::GetThread() -> QThread* { return p_.get(); }

auto TaskRunner::IsRunning() -> bool { return p_->isRunning(); }

}  // namespace GpgFrontend::Thread
