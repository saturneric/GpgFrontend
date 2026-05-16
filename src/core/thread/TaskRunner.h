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

#include "core/thread/Task.h"

namespace GpgFrontend::Thread {

/**
 * @brief Owns a dedicated QThread and serialises Task execution on it.
 *
 * Tasks posted via PostTask() are moved to the runner's thread and executed
 * sequentially through its event loop. PostConcurrentTask() submits to the
 * global QThreadPool instead. PostScheduleTask() delays execution by a given
 * number of seconds. RegisterTask() creates a task, tracks it, and returns a
 * TaskHandler for deferred start or cancellation.
 */
class GF_CORE_EXPORT TaskRunner : public QObject {
  Q_OBJECT
 public:
  /**
   * @brief Construct the runner. Call Start() to launch the worker thread.
   */
  TaskRunner();

  ~TaskRunner() override;

  /**
   * @brief Start the dedicated worker thread and its event loop.
   */
  void Start();

  /**
   * @brief Stop the worker thread and block until it exits.
   */
  void Stop();

  /**
   * @brief Return the underlying QThread owned by this runner.
   *
   * @return pointer to the internal QThread
   */
  auto GetThread() -> QThread*;

  /**
   * @brief Return whether the worker thread is currently running.
   *
   * @return true if the thread is active
   */
  auto IsRunning() -> bool;

  /**
   * @brief Move @p task to the runner thread and schedule it for execution.
   *
   * The task is executed via a queued connection on the thread's event loop.
   *
   * @param task heap-allocated task to execute; ownership is transferred
   */
  void PostTask(Task* task);

  /**
   * @brief Construct a Task from the given runnable and schedule it on the
   * runner thread.
   *
   * @param name human-readable task name
   * @param runnable function to execute on the runner thread
   * @param callback function invoked on the caller's thread after completion
   * @param data data object passed to runnable and callback
   */
  void PostTask(const QString& name, const Task::TaskRunnable& runnable,
                const Task::TaskCallback& callback, DataObjectPtr data);

  /**
   * @brief Construct a Task, register it in the pending-task map, and return a
   * handle.
   *
   * The task is not started until TaskHandler::Start() is called.
   *
   * @param name human-readable task name
   * @param runnable function to execute on the runner thread
   * @param callback function invoked on the caller's thread after completion
   * @param data data object passed to runnable and callback
   * @return TaskHandler wrapping the created task
   */
  auto RegisterTask(const QString& name, const Task::TaskRunnable& runnable,
                    const Task::TaskCallback& callback, DataObjectPtr data)
      -> Task::TaskHandler;

  /**
   * @brief Submit @p task to the global QThreadPool for concurrent execution.
   *
   * Unlike PostTask(), this does not use the runner's dedicated thread.
   *
   * @param task heap-allocated task to execute concurrently
   */
  void PostConcurrentTask(Task* task);

  /**
   * @brief Schedule @p task to run on the runner thread after @p seconds
   * seconds.
   *
   * @param task heap-allocated task to execute
   * @param seconds delay in seconds before execution
   */
  void PostScheduleTask(Task* task, size_t seconds);

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};
}  // namespace GpgFrontend::Thread
