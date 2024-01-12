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

#pragma once

#include "core/GpgFrontendCore.h"
#include "core/function/SecureMemoryAllocator.h"
#include "core/thread/Task.h"

namespace GpgFrontend::Thread {

class GPGFRONTEND_CORE_EXPORT TaskRunner : public QObject {
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
  ~TaskRunner() override;

  /**
   * @brief
   *
   */
  void Start();

  /**
   * @brief
   *
   */
  void Stop();

  /**
   * @brief Get the Thread object
   *
   * @return QThread*
   */
  auto GetThread() -> QThread*;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto IsRunning() -> bool;

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
   * @param runner
   * @param cb
   */
  void PostTask(const QString&, const Task::TaskRunnable&,
                const Task::TaskCallback&, DataObjectPtr);

  /**
   * @brief
   *
   * @return std::tuple<QPointer<Task>, TaskTrigger>
   */
  std::tuple<QPointer<Task>, Task::TaskTrigger> RegisterTask(
      const QString&, const Task::TaskRunnable&, const Task::TaskCallback&,
      DataObjectPtr);

  /**
   * @brief
   *
   * @param task
   */
  void PostConcurrentTask(Task* task);

  /**
   * @brief
   *
   * @param task
   * @param seconds
   */
  void PostScheduleTask(Task* task, size_t seconds);

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};
}  // namespace GpgFrontend::Thread
