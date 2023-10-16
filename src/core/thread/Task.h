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

#ifndef GPGFRONTEND_TASK_H
#define GPGFRONTEND_TASK_H

#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <type_traits>
#include <utility>

#include "core/GpgFrontendCore.h"

namespace GpgFrontend::Thread {

class TaskRunner;

class GPGFRONTEND_CORE_EXPORT Task : public QObject, public QRunnable {
  Q_OBJECT
 public:
  class DataObject;
  using DataObjectPtr = std::shared_ptr<DataObject>;             ///<
  using TaskRunnable = std::function<int(DataObjectPtr)>;        ///<
  using TaskCallback = std::function<void(int, DataObjectPtr)>;  ///<

  static const std::string DEFAULT_TASK_NAME;

  friend class TaskRunner;

  /**
   * @brief Construct a new Task object
   *
   */
  Task(std::string name = DEFAULT_TASK_NAME);

  /**
   * @brief Construct a new Task object
   *
   * @param callback The callback function to be executed.
   */
  explicit Task(TaskRunnable runnable, std::string name = DEFAULT_TASK_NAME,
                DataObjectPtr data_object = nullptr, bool sequency = true);

  /**
   * @brief Construct a new Task object
   *
   * @param runnable
   */
  explicit Task(
      TaskRunnable runnable, std::string name, DataObjectPtr data,
      TaskCallback callback = [](int, const std::shared_ptr<DataObject> &) {},
      bool sequency = true);

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

  /**
   * @brief
   *
   * @return std::string
   */
  std::string GetUUID() const;

  /**
   * @brief
   *
   * @return std::string
   */
  std::string GetFullID() const;

  /**
   * @brief
   *
   * @return std::string
   */
  bool GetSequency() const;

 public slots:

  /**
   * @brief
   *
   */
  void SlotRun();

 signals:
  /**
   * @brief announce runnable finished
   *
   */
  void SignalTaskRunnableEnd(int rtn);

  /**
   * @brief runnable and callabck all finished
   *
   */
  void SignalTaskEnd();

 protected:
  /**
   * @brief Set the Finish After Run object
   *
   * @param finish_after_run
   */
  void HoldOnLifeCycle(bool hold_on);

  /**
   * @brief
   *
   * @param rtn
   */
  void SetRTN(int rtn);

 private:
 class Impl;
 std::unique_ptr<Impl>;
  const std::string uuid_;
  const std::string name_;
  const bool sequency_ = true;  ///< must run in the same thread
  TaskCallback callback_;       ///<
  TaskRunnable runnable_;       ///<
  bool run_callback_after_runnable_finished_ = true;  ///<
  int rtn_ = 0;                                       ///<
  QThread *callback_thread_ = nullptr;                ///<
  DataObjectPtr data_object_ = nullptr;               ///<

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

  /**
   * @brief
   *
   * @return std::string
   */
  static std::string generate_uuid();

 private slots:
  /**
   * @brief
   *
   */
  void slot_task_run_callback(int rtn);
};
}  // namespace GpgFrontend::Thread

#endif  // GPGFRONTEND_TASK_H