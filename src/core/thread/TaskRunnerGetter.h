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

#ifndef GPGFRONTEND_TASKRUNNERGETTER_H
#define GPGFRONTEND_TASKRUNNERGETTER_H

#include "core/GpgFrontendCore.h"
#include "core/GpgFunctionObject.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Thread {

class GPGFRONTEND_CORE_EXPORT TaskRunnerGetter
    : public GpgFrontend::SingletonFunctionObject<TaskRunnerGetter> {
 public:
  enum TaskRunnerType {
    kTaskRunnerType_Default,
    kTaskRunnerType_GPG,
    kTaskRunnerType_IO,
    kTaskRunnerType_Network,
    kTaskRunnerType_External_Process,
  };

  TaskRunnerGetter(int channel = SingletonFunctionObject::GetDefaultChannel());

  TaskRunner *GetTaskRunner(
      TaskRunnerType runner_type = kTaskRunnerType_Default);

 private:
  std::map<TaskRunnerType, TaskRunner *> task_runners_;
};

}  // namespace GpgFrontend::Thread

#endif  // GPGFRONTEND_TASKRUNNERGETTER_H