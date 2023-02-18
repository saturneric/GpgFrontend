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

#include "core/thread/Task.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <string>
#include <utility>

#include "core/thread/TaskRunner.h"

const std::string GpgFrontend::Thread::Task::DEFAULT_TASK_NAME = "default-task";

GpgFrontend::Thread::Task::Task(std::string name)
    : uuid_(generate_uuid()), name_(name) {
  SPDLOG_TRACE("task {}/ created", GetFullID());
  init();
}

GpgFrontend::Thread::Task::Task(TaskRunnable runnable, std::string name,
                                DataObjectPtr data_object, bool sequency)
    : uuid_(generate_uuid()),
      name_(name),
      runnable_(std::move(runnable)),
      callback_(std::move([](int, const std::shared_ptr<DataObject> &) {})),
      callback_thread_(QThread::currentThread()),
      data_object_(data_object),
      sequency_(sequency) {
  SPDLOG_TRACE("task {} created with runnable, callback_thread_: {}",
               GetFullID(), static_cast<void *>(callback_thread_));
  init();
}

GpgFrontend::Thread::Task::Task(TaskRunnable runnable, std::string name,
                                DataObjectPtr data_object,
                                TaskCallback callback, bool sequency)
    : uuid_(generate_uuid()),
      name_(name),
      runnable_(std::move(runnable)),
      callback_(std::move(callback)),
      callback_thread_(QThread::currentThread()),
      data_object_(data_object),
      sequency_(sequency) {
  init();
  SPDLOG_TRACE(
      "task {} created with runnable and callback, callback_thread_: {}",
      GetFullID(), static_cast<void *>(callback_thread_));
}

GpgFrontend::Thread::Task::~Task() {
  SPDLOG_TRACE("task {} destroyed", GetFullID());
}

/**
 * @brief
 *
 * @return std::string
 */
std::string GpgFrontend::Thread::Task::GetFullID() const {
  return uuid_ + "/" + name_;
}

std::string GpgFrontend::Thread::Task::GetUUID() const { return uuid_; }

bool GpgFrontend::Thread::Task::GetSequency() const { return sequency_; }

void GpgFrontend::Thread::Task::SetFinishAfterRun(
    bool run_callback_after_runnable_finished) {
  this->run_callback_after_runnable_finished_ =
      run_callback_after_runnable_finished;
}

void GpgFrontend::Thread::Task::SetRTN(int rtn) { this->rtn_ = rtn; }

void GpgFrontend::Thread::Task::init() {
  // after runnable finished, running callback
  connect(this, &Task::SignalTaskRunnableEnd, this,
          &Task::slot_task_run_callback);
}

void GpgFrontend::Thread::Task::slot_task_run_callback(int rtn) {
  SPDLOG_TRACE("task runnable {} finished, rtn: {}", GetFullID(), rtn);
  // set return value
  this->SetRTN(rtn);

  try {
    if (callback_) {
      if (callback_thread_ == QThread::currentThread()) {
        SPDLOG_DEBUG("callback thread is the same thread");
        if (!QMetaObject::invokeMethod(callback_thread_,
                                       [callback = callback_, rtn = rtn_,
                                        data_object = data_object_, this]() {
                                         callback(rtn, data_object);
                                         // do cleaning work
                                         emit SignalTaskEnd();
                                       })) {
          SPDLOG_ERROR("failed to invoke callback");
        }
        // just finished, let callack thread to raise SignalTaskEnd
        return;
      } else {
        // waiting for callback to finish
        if (!QMetaObject::invokeMethod(
                callback_thread_,
                [callback = callback_, rtn = rtn_,
                 data_object = data_object_]() { callback(rtn, data_object); },
                Qt::BlockingQueuedConnection)) {
          SPDLOG_ERROR("failed to invoke callback");
        }
      }
    }
  } catch (std::exception &e) {
    SPDLOG_ERROR("exception caught: {}", e.what());
  } catch (...) {
    SPDLOG_ERROR("unknown exception caught");
  }

  // raise signal, announcing this task come to an end
  SPDLOG_DEBUG("task {}, starting calling signal SignalTaskEnd", GetFullID());
  emit SignalTaskEnd();
}

void GpgFrontend::Thread::Task::run() {
  SPDLOG_TRACE("task {} starting", GetFullID());

  // build runnable package for running
  auto runnable_package = [=, id = GetFullID()]() {
    SPDLOG_DEBUG("task {} runnable start runing", id);
    // Run() will set rtn by itself
    Run();
    // raise signal to anounce after runnable returned
    if (run_callback_after_runnable_finished_) emit SignalTaskRunnableEnd(rtn_);
  };

  if (thread() != QThread::currentThread()) {
    SPDLOG_DEBUG("task running thread is not object living thread");
    // running in another thread, blocking until returned
    if (!QMetaObject::invokeMethod(thread(), runnable_package,
                                   Qt::BlockingQueuedConnection)) {
      SPDLOG_ERROR("qt invoke method failed");
    }
  } else {
    if (!QMetaObject::invokeMethod(this, runnable_package)) {
      SPDLOG_ERROR("qt invoke method failed");
    }
  }
}

void GpgFrontend::Thread::Task::SlotRun() { run(); }

void GpgFrontend::Thread::Task::Run() {
  if (runnable_) {
    SetRTN(runnable_(data_object_));
  } else {
    SPDLOG_WARN("no runnable in task, do callback operation");
  }
}

GpgFrontend::Thread::Task::DataObject::Destructor *
GpgFrontend::Thread::Task::DataObject::get_heap_ptr(size_t bytes_size) {
  Destructor *dstr_ptr = new Destructor();
  dstr_ptr->p_obj = malloc(bytes_size);
  return dstr_ptr;
}

GpgFrontend::Thread::Task::DataObject::~DataObject() {
  if (!data_objects_.empty())
    SPDLOG_WARN("data_objects_ is not empty",
                "address:", static_cast<void *>(this));
  while (!data_objects_.empty()) {
    free_heap_ptr(data_objects_.top());
    data_objects_.pop();
  }
}

size_t GpgFrontend::Thread::Task::DataObject::GetObjectSize() {
  return data_objects_.size();
}

void GpgFrontend::Thread::Task::DataObject::free_heap_ptr(Destructor *ptr) {
  SPDLOG_TRACE("p_obj: {} data object: {}",
               static_cast<const void *>(ptr->p_obj),
               static_cast<void *>(this));
  if (ptr->destroy != nullptr) {
    ptr->destroy(ptr->p_obj);
  }
  free((void *)ptr->p_obj);
  delete ptr;
}

std::string GpgFrontend::Thread::Task::generate_uuid() {
  return boost::uuids::to_string(boost::uuids::random_generator()());
}
