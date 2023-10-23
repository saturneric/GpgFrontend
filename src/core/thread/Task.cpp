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

#include "core/thread/Task.h"

#include <qobjectdefs.h>

#include <boost/stacktrace.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>

#include "spdlog/spdlog.h"

namespace GpgFrontend::Thread {

class Task::Impl : public QObject {
  Q_OBJECT

 public:
  Impl(Task *parent, std::string name)
      : QObject(parent), parent_(parent), uuid_(generate_uuid()), name_(name) {
    SPDLOG_TRACE("task {} created", GetFullID());
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, std::string name,
       DataObjectPtr data_object)
      : QObject(parent),
        parent_(parent),
        uuid_(generate_uuid()),
        name_(name),
        runnable_(std::move(runnable)),
        callback_(std::move([](int, const DataObjectPtr &) {})),
        callback_thread_(QThread::currentThread()),
        data_object_(data_object) {
    SPDLOG_TRACE("task {} created with runnable, callback_thread_: {}",
                 GetFullID(), static_cast<void *>(callback_thread_));
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, std::string name,
       DataObjectPtr data_object, TaskCallback callback)
      : QObject(parent),
        parent_(parent),
        uuid_(generate_uuid()),
        name_(name),
        runnable_(std::move(runnable)),
        callback_(std::move(callback)),
        callback_thread_(QThread::currentThread()),
        data_object_(data_object) {
    SPDLOG_TRACE(
        "task {} created with runnable and callback, callback_thread_: {}",
        GetFullID(), static_cast<void *>(callback_thread_));
    init();
  }

  ~Impl() { SPDLOG_TRACE("task {} destroyed", GetFullID()); }

  /**
   * @brief
   *
   * @return std::string
   */
  std::string GetFullID() const { return uuid_ + "/" + name_; }

  std::string GetUUID() const { return uuid_; }

  void Run() {
    SPDLOG_DEBUG("task {} is using default runnable and callback mode",
                 GetFullID());
    if (runnable_) {
      SetRTN(runnable_(data_object_));
    } else {
      SPDLOG_WARN("no runnable in task, do callback operation");
    }
  }

  /**
   * @brief Set the Finish After Run object
   *
   * @param finish_after_run
   */
  void HoldOnLifeCycle(bool hold_on) { parent_->setAutoDelete(!hold_on); }

  /**
   * @brief
   *
   * @param rtn
   */
  void SetRTN(int rtn) { this->rtn_ = rtn; }

 private slots:

  /**
   * @brief
   *
   */
  void slot_run() {
    try {
      SPDLOG_TRACE("task {} is starting...", GetFullID());
      // Run() will set rtn by itself
      parent_->Run();
      SPDLOG_TRACE("task {} was end.", GetFullID());
    } catch (std::exception &e) {
      SPDLOG_ERROR("exception was caught at task: {}", e.what());
      SPDLOG_ERROR(
          "stacktrace of the exception: {}",
          boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
    }
    // raise signal to anounce after runnable returned
    if (parent_->autoDelete()) emit parent_->SignalTaskShouldEnd(rtn_);
  }

 private:
  Task *const parent_;
  const std::string uuid_;
  const std::string name_;
  TaskCallback callback_;                ///<
  TaskRunnable runnable_;                ///<
  int rtn_ = 0;                          ///<
  QThread *callback_thread_ = nullptr;   ///<
  DataObjectPtr data_object_ = nullptr;  ///<

  void init() {
    SPDLOG_TRACE("task {} created, parent: {}, impl: {}", name_,
                 (void *)parent_, (void *)this);

    //
    HoldOnLifeCycle(false);

    //
    connect(parent_, &Task::SignalRun, this, &Task::Impl::slot_run);

    //
    connect(parent_, &Task::SignalTaskShouldEnd, this,
            &Impl::slot_task_should_end);

    //
    connect(parent_, &Task::SignalTaskEnd, parent_, &Task::deleteLater);
  }

  /**
   * @brief
   *
   * @return std::string
   */
  std::string generate_uuid() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
  }

 private slots:

  /**
   * @brief
   *
   */
  void slot_task_should_end(int rtn) {
    SPDLOG_TRACE("task runnable {} finished, rtn: {}", GetFullID(), rtn);
    // set return value
    this->SetRTN(rtn);

    try {
      if (callback_) {
        if (callback_thread_ == QThread::currentThread()) {
          SPDLOG_DEBUG("for task {}, the callback thread is the same thread",
                       GetFullID(), callback_thread_->currentThreadId());
          callback_(rtn, data_object_);
        } else {
          SPDLOG_DEBUG("for task {}, callback thread is a different thread: {}",
                       GetFullID(), callback_thread_->currentThreadId());
          if (!QMetaObject::invokeMethod(callback_thread_,
                                         [callback = callback_, rtn = rtn_,
                                          data_object = data_object_]() {
                                           callback(rtn, data_object);
                                         })) {
            SPDLOG_ERROR("task {} had failed to invoke callback", GetFullID());
          }
        }
      }
    } catch (std::exception &e) {
      SPDLOG_ERROR("exception was caught at task callback: {}", e.what());
      SPDLOG_ERROR(
          "stacktrace of the exception: {}",
          boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
    } catch (...) {
      SPDLOG_ERROR("unknown exception was caught");
      SPDLOG_ERROR(
          "stacktrace of the exception: {}",
          boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
    }

    // raise signal, announcing this task come to an end
    SPDLOG_DEBUG("for task {}, its life comes to an end.", GetFullID());
    emit parent_->SignalTaskEnd();
  }
};

Task::Task(std::string name) : p_(new Impl(this, name)) {}

Task::Task(TaskRunnable runnable, std::string name, DataObjectPtr data_object)
    : p_(new Impl(this, runnable, name, data_object)) {}

Task::Task(TaskRunnable runnable, std::string name, DataObjectPtr data_object,
           TaskCallback callback)
    : p_(new Impl(this, runnable, name, data_object, callback)) {}

Task::~Task() = default;

/**
 * @brief
 *
 * @return std::string
 */
std::string Task::GetFullID() const { return p_->GetFullID(); }

std::string Task::GetUUID() const { return p_->GetUUID(); }

void Task::HoldOnLifeCycle(bool hold_on) { p_->HoldOnLifeCycle(hold_on); }

void Task::SetRTN(int rtn) { p_->SetRTN(rtn); }

void Task::SafelyRun() { emit SignalRun(); }

void Task::Run() { p_->Run(); }

void Task::run() {
  SPDLOG_DEBUG("interface run() of task {} was called by thread: {}",
               GetFullID(), QThread::currentThread()->currentThreadId());
  this->SafelyRun();
}

}  // namespace GpgFrontend::Thread

#include "Task.moc"