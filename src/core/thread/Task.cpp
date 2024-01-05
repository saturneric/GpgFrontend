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

#include <qscopedpointer.h>

#include <boost/stacktrace.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <utility>

#include "utils/MemoryUtils.h"

namespace GpgFrontend::Thread {

class Task::Impl {
 public:
  Impl(Task *parent, std::string name)
      : parent_(parent), uuid_(generate_uuid()), name_(name) {
    GF_CORE_LOG_TRACE("task {} created", GetFullID());
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, std::string name,
       DataObjectPtr data_object)
      : parent_(parent),
        uuid_(generate_uuid()),
        name_(std::move(name)),
        runnable_(std::move(runnable)),
        callback_([](int, const DataObjectPtr &) {}),
        callback_thread_(QThread::currentThread()),
        data_object_(std::move(data_object)) {
    GF_CORE_LOG_TRACE("task {} created with runnable, callback_thread_: {}",
                      GetFullID(), static_cast<void *>(callback_thread_));
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, std::string name,
       DataObjectPtr data_object, TaskCallback callback)
      : parent_(parent),
        uuid_(generate_uuid()),
        name_(std::move(name)),
        runnable_(std::move(runnable)),
        callback_(std::move(callback)),
        callback_thread_(QThread::currentThread()),
        data_object_(std::move(data_object)) {
    GF_CORE_LOG_TRACE(
        "task {} created with runnable and callback, callback_thread_: {}",
        GetFullID(), static_cast<void *>(callback_thread_));
    init();
  }

  ~Impl() { GF_CORE_LOG_TRACE("task {} destroyed", GetFullID()); }

  /**
   * @brief
   *
   * @return std::string
   */
  std::string GetFullID() const { return uuid_ + "/" + name_; }

  std::string GetUUID() const { return uuid_; }

  void Run() {
    GF_CORE_LOG_TRACE("task {} is using default runnable and callback mode",
                      GetFullID());
    if (runnable_) {
      SetRTN(runnable_(data_object_));
    } else {
      GF_CORE_LOG_WARN("no runnable in task, do callback operation");
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

 private:
  Task *const parent_;
  const std::string uuid_;
  const std::string name_;
  TaskRunnable runnable_;                ///<
  TaskCallback callback_;                ///<
  int rtn_ = 0;                          ///<
  QThread *callback_thread_ = nullptr;   ///<
  DataObjectPtr data_object_ = nullptr;  ///<

  void init() {
    GF_CORE_LOG_TRACE("task {} created, parent: {}, impl: {}", name_,
                      (void *)parent_, (void *)this);

    //
    HoldOnLifeCycle(false);

    //
    connect(parent_, &Task::SignalRun, [=]() { inner_run(); });

    //
    connect(parent_, &Task::SignalTaskShouldEnd,
            [=](int rtn) { slot_task_should_end(rtn); });

    //
    connect(parent_, &Task::SignalTaskEnd, parent_, &Task::deleteLater);
  }

  /**
   * @brief
   *
   */
  void inner_run() {
    try {
      GF_CORE_LOG_TRACE("task {} is starting...", GetFullID());
      // Run() will set rtn by itself
      parent_->Run();
      GF_CORE_LOG_TRACE("task {} was end.", GetFullID());
    } catch (std::exception &e) {
      GF_CORE_LOG_ERROR("exception was caught at task: {}", e.what());
      GF_CORE_LOG_ERROR(
          "stacktrace of the exception: {}",
          boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
    }
    // raise signal to anounce after runnable returned
    if (parent_->autoDelete()) emit parent_->SignalTaskShouldEnd(rtn_);
  }

  /**
   * @brief
   *
   * @return std::string
   */
  static auto generate_uuid() -> std::string {
    return boost::uuids::to_string(boost::uuids::random_generator()());
  }

  /**
   * @brief
   *
   */
  void slot_task_should_end(int rtn) {
    GF_CORE_LOG_TRACE("task runnable {} finished, rtn: {}", GetFullID(), rtn);
    // set return value
    this->SetRTN(rtn);
#ifdef RELEASE
    try {
#endif
      if (callback_) {
        GF_CORE_LOG_TRACE("task {} has a callback function", GetFullID());
        if (callback_thread_ == QThread::currentThread()) {
          GF_CORE_LOG_TRACE(
              "for task {}, the callback thread is the same thread",
              GetFullID(), callback_thread_->currentThreadId());

          callback_(rtn, data_object_);

          // raise signal, announcing this task comes to an end
          GF_CORE_LOG_TRACE(
              "for task {}, its life comes to an end in the same thread after "
              "its callback executed.",
              parent_->GetFullID());
          emit parent_->SignalTaskEnd();
        } else {
          GF_CORE_LOG_TRACE(
              "for task {}, callback thread is a different thread: {}",
              GetFullID(), callback_thread_->currentThreadId());
          if (!QMetaObject::invokeMethod(
                  callback_thread_,
                  [callback = callback_, rtn = rtn_, data_object = data_object_,
                   parent_ = this->parent_]() {
                    GF_CORE_LOG_TRACE("calling callback of task {}",
                                      parent_->GetFullID());
                    try {
                      callback(rtn, data_object);
                    } catch (...) {
                      GF_CORE_LOG_ERROR(
                          "unknown exception was caught when execute "
                          "callback of task {}",
                          parent_->GetFullID());
                    }
                    // raise signal, announcing this task comes to an end
                    GF_CORE_LOG_TRACE(
                        "for task {}, its life comes to an end whether its "
                        "callback function fails or not.",
                        parent_->GetFullID());
                    emit parent_->SignalTaskEnd();
                  })) {
            GF_CORE_LOG_ERROR(
                "task {} had failed to invoke the callback function to target "
                "thread",
                GetFullID());
            GF_CORE_LOG_TRACE(
                "for task {}, its life must come to an end now, although it "
                "has something not done yet.",
                GetFullID());
            emit parent_->SignalTaskEnd();
          }
        }
      } else {
        // raise signal, announcing this task comes to an end
        GF_CORE_LOG_TRACE(
            "for task {}, its life comes to an end without callback "
            "peacefully.",
            GetFullID());
        emit parent_->SignalTaskEnd();
      }
#ifdef RELEASE
    } catch (std::exception &e) {
      GF_CORE_LOG_ERROR("exception was caught at task callback: {}", e.what());
      GF_CORE_LOG_ERROR(
          "stacktrace of the exception: {}",
          boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
      // raise signal, announcing this task comes to an end
      GF_CORE_LOG_TRACE("for task {}, its life comes to an end at chaos.",
                        GetFullID());
      emit parent_->SignalTaskEnd();
    } catch (...) {
      GF_CORE_LOG_ERROR("unknown exception was caught");
      GF_CORE_LOG_ERROR(
          "stacktrace of the exception: {}",
          boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
      // raise signal, announcing this task comes to an end
      GF_CORE_LOG_TRACE(
          "for task {}, its life comes to an end at unknown chaos.",
          GetFullID());
      emit parent_->SignalTaskEnd();
    }
#endif
  }
};

Task::Task(std::string name) : p_(new Impl(this, name)) {}

Task::Task(TaskRunnable runnable, std::string name, DataObjectPtr data_object)
    : p_(SecureCreateUniqueObject<Impl>(this, runnable, name, data_object)) {}

Task::Task(TaskRunnable runnable, std::string name, DataObjectPtr data_object,
           TaskCallback callback)
    : p_(SecureCreateUniqueObject<Impl>(this, runnable, name, data_object,
                                        callback)) {}

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
  GF_CORE_LOG_TRACE("interface run() of task {} was called by thread: {}",
                    GetFullID(), QThread::currentThread()->currentThreadId());
  this->SafelyRun();
}

}  // namespace GpgFrontend::Thread