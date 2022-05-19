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
#include "easylogging++.h"

GpgFrontend::Thread::Task::Task() : uuid_(generate_uuid()) {
  LOG(TRACE) << "Task" << uuid_ << "created";
  init();
}

GpgFrontend::Thread::Task::Task(TaskCallback callback,
                                DataObjectPtr data_object)
    : uuid_(generate_uuid()),
      callback_(std::move(callback)),
      callback_thread_(QThread::currentThread()),
      data_object_(data_object) {
  LOG(TRACE) << "Task" << uuid_ << "created with callback"
             << "callback_thread_: " << callback_thread_;
  init();
}

GpgFrontend::Thread::Task::Task(TaskRunnable runnable, TaskCallback callback,
                                DataObjectPtr data_object)
    : uuid_(generate_uuid()),
      runnable_(std::move(runnable)),
      callback_(std::move(callback)),
      callback_thread_(QThread::currentThread()),
      data_object_(data_object) {
  init();
  LOG(TRACE) << "Task" << uuid_ << "created with runnable and callback"
             << "callback_thread_: " << callback_thread_;
}

GpgFrontend::Thread::Task::~Task() = default;

std::string GpgFrontend::Thread::Task::GetUUID() const { return uuid_; }

void GpgFrontend::Thread::Task::SetFinishAfterRun(bool finish_after_run) {
  this->finish_after_run_ = finish_after_run;
}

void GpgFrontend::Thread::Task::SetRTN(int rtn) { this->rtn_ = rtn; }

void GpgFrontend::Thread::Task::init() {
  connect(this, &Task::SignalTaskFinished, this, &Task::before_finish_task);
  connect(this, &Task::SignalTaskFinished, this, &Task::deleteLater);
}

void GpgFrontend::Thread::Task::before_finish_task() {
  LOG(TRACE) << "Task" << uuid_ << "finished";
  if (callback_) {
    bool if_invoke = QMetaObject::invokeMethod(
        callback_thread_,
        [callback = callback_, rtn = rtn_, data_object = data_object_]() {
          callback(rtn, data_object);
        });
    if (!if_invoke) {
      LOG(ERROR) << "failed to invoke callback";
    }
  }
}

void GpgFrontend::Thread::Task::run() {
  LOG(TRACE) << "Task" << uuid_ << "started";
  Run();
  if (finish_after_run_) emit SignalTaskFinished();
}

void GpgFrontend::Thread::Task::Run() {
  if (runnable_) {
    bool if_invoke = QMetaObject::invokeMethod(
        this, [=]() { return runnable_(data_object_); }, &rtn_);
    if (!if_invoke) {
      LOG(ERROR) << "Qt invokeMethod failed";
    }
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
    LOG(WARNING) << "data_objects_ is not empty"
                 << "address:" << this;
  while (!data_objects_.empty()) {
    free_heap_ptr(data_objects_.top());
    data_objects_.pop();
  }
}

size_t GpgFrontend::Thread::Task::DataObject::GetObjectSize() {
  return data_objects_.size();
}

void GpgFrontend::Thread::Task::DataObject::free_heap_ptr(Destructor *ptr) {
  DLOG(TRACE) << "p_obj: " << ptr->p_obj << "destructor: " << ptr->destroy
              << "DataObject:" << this;
  if (ptr->destroy != nullptr) {
    ptr->destroy(ptr->p_obj);
  }
  free((void *)ptr->p_obj);
  delete ptr;
}

std::string GpgFrontend::Thread::Task::generate_uuid() {
  return boost::uuids::to_string(boost::uuids::random_generator()());
}
