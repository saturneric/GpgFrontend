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

#ifndef GPGFRONTEND_TASKRUNNER_H
#define GPGFRONTEND_TASKRUNNER_H

#include <cstddef>
#include <mutex>
#include <queue>

#include "core/GpgFrontendCore.h"

namespace GpgFrontend::Thread {

class Task;

class GPGFRONTEND_CORE_EXPORT TaskRunner : public QThread {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Task Runner object
   *
   */
  TaskRunner();

  /**
   * @brief Destroy the Task Runner object
   *
   */
  virtual ~TaskRunner() override;

  /**
   * @brief
   *
   */
  [[noreturn]] void run() override;

 public slots:

  /**
   * @brief
   *
   * @param task
   */
  void PostTask(Task* task);

  /**
   * @brief
   *
   * @param task
   * @param seconds
   */
  void PostScheduleTask(Task* task, size_t seconds);

 private:
  std::queue<Task*> tasks;                      ///< The task queue
  std::map<std::string, Task*> pending_tasks_;  ///< The pending tasks
  std::mutex tasks_mutex_;                      ///< The task queue mutex
  QThreadPool thread_pool_{this};               ///< run non-sequency task

  /**
   * @brief
   *
   */
  void unregister_finished_task(std::string);
};
}  // namespace GpgFrontend::Thread

#endif  // GPGFRONTEND_TASKRUNNER_H