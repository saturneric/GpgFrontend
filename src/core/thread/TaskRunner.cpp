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

#include <QtConcurrent>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <utility>

#include "core/thread/Task.h"

namespace GpgFrontend::Thread {

class TaskRunner::Impl : public QThread {
 public:
  Impl() : QThread(nullptr) {}

  void PostTask(Task* task) {
    if (task == nullptr) {
      SPDLOG_ERROR("task posted is null");
      return;
    }

    task->setParent(nullptr);
    task->moveToThread(this);

    SPDLOG_TRACE("runner starts task: {} at thread: {}", task->GetFullID(),
                 this->currentThreadId());
    task->SafelyRun();
  }

  static void PostTask(const Task::TaskRunnable& runner,
                       const Task::TaskCallback& cb, DataObjectPtr p_obj) {
    auto* callback_thread = QThread::currentThread();
    auto data_object = std::move(p_obj);
    const auto task_uuid = generate_uuid();

    QtConcurrent::run(runner, data_object).then([=](int rtn) {
      if (!cb) {
        SPDLOG_TRACE("task {} doesn't have a callback function", task_uuid);
        return;
      }

      if (callback_thread == QThread::currentThread()) {
        SPDLOG_TRACE("for task {}, the callback thread is the same thread: {}",
                     task_uuid, static_cast<void*>(callback_thread));

        cb(rtn, data_object);

        // raise signal, announcing this task comes to an end
        SPDLOG_TRACE(
            "for task {}, its life comes to an end in the same thread "
            "after its callback executed.",
            task_uuid);
      } else {
        SPDLOG_TRACE("for task {}, callback thread is a different thread: {}",
                     task_uuid, static_cast<void*>(callback_thread));
        if (!QMetaObject::invokeMethod(callback_thread, [=]() {
              SPDLOG_TRACE("calling callback of task {}", task_uuid);
              try {
                cb(rtn, data_object);
              } catch (...) {
                SPDLOG_ERROR(
                    "unknown exception was caught when execute "
                    "callback of task {}",
                    task_uuid);
              }
              // raise signal, announcing this task comes to an end
              SPDLOG_TRACE(
                  "for task {}, its life comes to an end whether its "
                  "callback function fails or not.",
                  task_uuid);
            })) {
          SPDLOG_ERROR(
              "task {} had failed to invoke the callback function to "
              "target thread",
              task_uuid);
          SPDLOG_TRACE(
              "for task {}, its life must come to an end now, although it "
              "has something not done yet.",
              task_uuid);
        }
      }
    });
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

 private:
  static auto generate_uuid() -> std::string {
    return boost::uuids::to_string(boost::uuids::random_generator()());
  }
};

TaskRunner::TaskRunner() : p_(SecureCreateUniqueObject<Impl>()) {}

TaskRunner::~TaskRunner() {
  if (p_->isRunning()) {
    Stop();
  }
}

void TaskRunner::PostTask(Task* task) { p_->PostTask(task); }

void TaskRunner::PostTask(const Task::TaskRunnable& runner,
                          const Task::TaskCallback& cb, DataObjectPtr p_obj) {
  p_->PostTask(runner, cb, p_obj);
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

}  // namespace GpgFrontend::Thread
