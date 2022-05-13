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

#ifndef GPGFRONTEND_TASK_H
#define GPGFRONTEND_TASK_H

#include <functional>

#include "core/GpgFrontendCore.h"

namespace GpgFrontend::Thread {

class TaskRunner;

class GPGFRONTEND_CORE_EXPORT Task : public QObject, public QRunnable {
  Q_OBJECT
 public:
  using TaskRunnable = std::function<int()>;      ///<
  using TaskCallback = std::function<void(int)>;  ///<
  friend class TaskRunner;

  /**
   * @brief Construct a new Task object
   *
   */
  Task();

  /**
   * @brief Construct a new Task object
   *
   * @param callback The callback function to be executed.
   *                 callback must not be nullptr, and not tp opreate UI object.
   */
  Task(TaskCallback callback);

  /**
   * @brief Construct a new Task object
   *
   * @param runnable
   */
  Task(
      TaskRunnable runnable, TaskCallback callback = [](int) {});

  /**
   * @brief Destroy the Task object
   *
   */
  virtual ~Task() override;

  /**
   * @brief Run - run the task
   *
   */
  virtual void Run();

 signals:
  void SignalTaskFinished();

 protected:
  void SetFinishAfterRun(bool finish_after_run);

  void SetRTN(int rtn);

 private:
  TaskCallback callback_;         ///<
  TaskRunnable runnable_;         ///<
  bool finish_after_run_ = true;  ///<
  int rtn_ = 0;                   ///<

  /**
   * @brief
   *
   */
  void before_finish_task();

  /**
   * @brief
   *
   */
  void init();

  /**
   * @brief
   *
   */
  virtual void run() override;
};
}  // namespace GpgFrontend::Thread

#endif  // GPGFRONTEND_TASK_H