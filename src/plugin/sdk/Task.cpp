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
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "Task.h"

#include "core/thread/Task.h"

namespace GpgFrontend::Plugin::SDK {
/**
 * @brief Construct a new Task object
 *
 */
STask::STask(STaskName name) : Thread::Task(name) {}

/**
 * @brief Construct a new Task object
 *
 * @param callback The callback function to be executed.
 */
STask::STask(STaskName name, STaskRunnable runnable, SDataObjectPtr data_object)
    : Thread::Task(runnable, name, data_object, true) {}

/**
 * @brief Construct a new Task object
 *
 * @param runnable
 */
STask::STask(STaskName name, STaskRunnable runnable, SDataObjectPtr data_object,
             STaskCallback callback)
    : Thread::Task(runnable, name, data_object, callback, true) {}

void PostTask(Thread::TaskRunner *runner, STask *task) {
  if (runner == nullptr || task == nullptr) return;
  runner->PostTask(task);
}

}  // namespace GpgFrontend::Plugin::SDK