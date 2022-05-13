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

#include <functional>

#include "core/thread/TaskRunner.h"

GpgFrontend::Thread::Task::Task() { init(); }

GpgFrontend::Thread::Task::Task(TaskCallback callback)
    : callback_(std::move(callback)) {
  init();
}

GpgFrontend::Thread::Task::Task(TaskRunnable runnable, TaskCallback callback)
    : runnable_(runnable), callback_(std::move(callback)) {
  init();
}

GpgFrontend::Thread::Task::~Task() = default;

void GpgFrontend::Thread::Task::SetFinishAfterRun(bool finish_after_run) {
  this->finish_after_run_ = finish_after_run;
}

void GpgFrontend::Thread::Task::SetRTN(int rtn) { this->rtn_ = rtn; }

void GpgFrontend::Thread::Task::init() {
  LOG(INFO) << "called";
  connect(this, &Task::SignalTaskFinished, this, &Task::before_finish_task);
  connect(this, &Task::SignalTaskFinished, this, &Task::deleteLater);
}

void GpgFrontend::Thread::Task::before_finish_task() {
  LOG(INFO) << "called";
  if (callback_) callback_(rtn_);
}

void GpgFrontend::Thread::Task::run() {
  LOG(INFO) << "called";
  Run();
  if (finish_after_run_) emit SignalTaskFinished();
}

void GpgFrontend::Thread::Task::Run() {
  if (runnable_) {
    rtn_ = runnable_();
  }
}
