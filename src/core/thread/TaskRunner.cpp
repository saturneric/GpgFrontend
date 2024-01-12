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

#include "core/thread/Task.h"

namespace GpgFrontend::Thread {

class TaskRunner::Impl : public QThread {
 public:
  Impl() : QThread(nullptr) {}

  void PostTask(Task* task) {
    if (task == nullptr) {
      GF_CORE_LOG_ERROR("task posted is null");
      return;
    }

    task->setParent(nullptr);
    task->moveToThread(this);

    GF_CORE_LOG_TRACE("runner starts task: {} at thread: {}", task->GetFullID(),
                      this->currentThreadId());
    task->SafelyRun();
  }

  std::tuple<QPointer<Task>, Task::TaskTrigger> RegisterTask(
      const QString& name, const Task::TaskRunnable& runnerable,
      const Task::TaskCallback& cb, DataObjectPtr params) {
    auto raw_task = SecureCreateQSharedObject<Task>(runnerable, name,
                                                    std::move(params), cb);
    raw_task->setParent(nullptr);
    raw_task->moveToThread(this);

    connect(raw_task.get(), &Task::SignalRun, this, [this, raw_task]() {
      pending_tasks_[raw_task->GetFullID()] = raw_task;
    });

    connect(raw_task.get(), &Task::SignalTaskEnd, this, [this, raw_task]() {
      pending_tasks_.remove(raw_task->GetFullID());
    });

    GF_CORE_LOG_TRACE("runner starts task: {} at thread: {}",
                      raw_task->GetFullID(), this->currentThreadId());

    QPointer<Task> const task = raw_task.get();
    return {nullptr, [task]() {
              if (task != nullptr) task->SafelyRun();
            }};
  }

  void PostTask(const QString& name, const Task::TaskRunnable& runnerable,
                const Task::TaskCallback& cb, DataObjectPtr params) {
    PostTask(new Task(runnerable, name, std::move(params), cb));
  }

  void PostConcurrentTask(Task* task) {
    if (task == nullptr) {
      GF_CORE_LOG_ERROR("task posted is null");
      return;
    }

    auto* concurrent_thread = new QThread(this);

    task->setParent(nullptr);
    task->moveToThread(concurrent_thread);

    connect(task, &Task::SignalTaskEnd, concurrent_thread, &QThread::quit);
    connect(concurrent_thread, &QThread::finished, concurrent_thread,
            &QThread::deleteLater);

    concurrent_thread->start();

    GF_CORE_LOG_TRACE("runner starts task concurrenctly: {}",
                      task->GetFullID());
    task->SafelyRun();
  }

  void PostScheduleTask(Task* task, size_t seconds) {
    if (task == nullptr) return;
    // TODO
  }

 private:
  QMap<QString, QSharedPointer<Task>> pending_tasks_;
};

TaskRunner::TaskRunner() : p_(SecureCreateUniqueObject<Impl>()) {}

TaskRunner::~TaskRunner() {
  if (p_->isRunning()) {
    Stop();
  }
}

void TaskRunner::PostTask(Task* task) { p_->PostTask(task); }

void TaskRunner::PostTask(const QString& name, const Task::TaskRunnable& runner,
                          const Task::TaskCallback& cb, DataObjectPtr params) {
  p_->PostTask(name, runner, cb, std::move(params));
}

void TaskRunner::PostConcurrentTask(Task* task) {
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

std::tuple<QPointer<Task>, Task::TaskTrigger> TaskRunner::RegisterTask(
    const QString& name, const Task::TaskRunnable& runnable,
    const Task::TaskCallback& cb, DataObjectPtr p_pbj) {
  return p_->RegisterTask(name, runnable, cb, p_pbj);
}
}  // namespace GpgFrontend::Thread
