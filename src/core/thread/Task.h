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
#include "core/model/DataObject.h"

namespace GpgFrontend::Thread {

class TaskRunner;

class GPGFRONTEND_CORE_EXPORT Task : public QObject, public QRunnable {
  Q_OBJECT
 public:
  friend class TaskRunner;

  using TaskRunnable = std::function<int(DataObjectPtr)>;        ///<
  using TaskCallback = std::function<void(int, DataObjectPtr)>;  ///<

  class TaskHandler {
   public:
    explicit TaskHandler(Task*);

    void Start();

    void Cancel();

    auto GetTask() -> Task*;

   private:
    QPointer<Task> task_;
  };

  /**
   * @brief Construct a new Task object
   *
   */
  explicit Task(QString name);

  /**
   * @brief Construct a new Task object
   *
   * @param callback The callback function to be executed.
   */
  explicit Task(TaskRunnable runnable, QString name,
                DataObjectPtr data_object = nullptr);

  /**
   * @brief Construct a new Task object
   *
   * @param runnable
   */
  explicit Task(TaskRunnable runnable, QString name, DataObjectPtr data,
                TaskCallback callback);

  /**
   * @brief Destroy the Task object
   *
   */
  ~Task() override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetUUID() const -> QString;

  /**
   * @brief Get the Full I D object
   *
   * @return QString
   */
  [[nodiscard]] auto GetFullID() const -> QString;

  /**
   * @brief
   *
   * @param hold_on
   */
  void HoldOnLifeCycle(bool hold_on);

  /**
   * @brief can be overwrite by subclass
   *
   * @return int
   */
  virtual auto Run() -> int;

  /**
   * @brief
   *
   * @return auto
   */
  [[nodiscard]] auto GetRTN();

 public slots:

  /**
   * @brief shouldn't be overwrite by subclass
   *
   */
  void SafelyRun();

 signals:

  /**
   * @brief
   *
   */
  void SignalRun();

  /**
   * @brief
   *
   */
  void SignalTaskShouldEnd(int);

  /**
   * @brief
   *
   */
  void SignalTaskEnd();

 protected:
  /**
   * @brief
   *
   * @param rtn
   */
  void setRTN(int rtn);

 private slots:

  /**
   * @brief
   *
   */
  void slot_exception_safe_run() noexcept;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;

  void run() override;
};
}  // namespace GpgFrontend::Thread
