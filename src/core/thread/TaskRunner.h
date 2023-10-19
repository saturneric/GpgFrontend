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

#include <vector>

#include "core/GpgFrontendCore.h"

namespace GpgFrontend::Thread {

class Task;

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
  virtual ~TaskRunner() override;

  void Start();

  QThread* GetThread();

  bool IsRunning();

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
  std::unique_ptr<Impl> p_;
};
}  // namespace GpgFrontend::Thread

#endif  // GPGFRONTEND_TASKRUNNER_H