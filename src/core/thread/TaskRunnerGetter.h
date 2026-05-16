/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#pragma once

#include <mutex>

#include "core/function/basic/GpgFunctionObject.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Thread {

using TaskRunnerPtr = QSharedPointer<TaskRunner>;

/**
 * @brief Singleton registry that provides a dedicated TaskRunner per purpose.
 *
 * Each TaskRunnerType maps to a lazily-created, auto-started TaskRunner backed
 * by its own thread. Callers retrieve runners by type and post tasks to them.
 * StopAllTeakRunner() stops every runner at application shutdown.
 */
class GF_CORE_EXPORT TaskRunnerGetter
    : public GpgFrontend::SingletonFunctionObject<TaskRunnerGetter> {
 public:
  /**
   * @brief Identifies the purpose of a TaskRunner, each backed by its own thread.
   */
  enum TaskRunnerType {
    kTaskRunnerType_Default,           ///< General-purpose tasks
    kTaskRunnerType_GPG,               ///< GPG/OpenPGP operations
    kTaskRunnerType_IO,                ///< File and disk I/O
    kTaskRunnerType_Network,           ///< Network requests
    kTaskRunnerType_Module,            ///< Module registration and lifecycle
    kTaskRunnerType_External_Process,  ///< External process execution
  };

  /**
   * @brief Construct the getter for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit TaskRunnerGetter(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Return the TaskRunner for the given type, creating and starting it if needed.
   *
   * Thread-safe; lazily creates a new TaskRunner on first access for each type.
   *
   * @param runner_type type of task runner to retrieve (default: kTaskRunnerType_Default)
   * @return shared pointer to the TaskRunner; always valid
   */
  auto GetTaskRunner(TaskRunnerType runner_type = kTaskRunnerType_Default)
      -> TaskRunnerPtr;

  /**
   * @brief Stop all running TaskRunners.
   *
   * Should be called during application shutdown before destroying the singleton.
   */
  void StopAllTeakRunner();

 private:
  // Map from runner type to its TaskRunner instance.
  std::map<TaskRunnerType, TaskRunnerPtr> task_runners_;
  // Guards task_runners_ for thread-safe lazy creation.
  std::mutex task_runners_map_lock_;
};

}  // namespace GpgFrontend::Thread
