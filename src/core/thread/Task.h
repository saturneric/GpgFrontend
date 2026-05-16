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

#include "core/model/DataObject.h"

namespace GpgFrontend::Thread {

class TaskRunner;

/**
 * @brief Unit of work that runs on a TaskRunner's thread and optionally
 * delivers a result via callback to the construction thread.
 *
 * A Task wraps a TaskRunnable (executed on the runner thread) and an optional
 * TaskCallback (invoked on the thread that constructed the task). Subclasses
 * may override Run() instead of supplying a runnable. The return code from
 * Run() is stored and passed to the callback. Tasks are identified by a UUID
 * and a human-readable name.
 */
class GF_CORE_EXPORT Task : public QObject, public QRunnable {
  Q_OBJECT
 public:
  friend class TaskRunner;

  // Called on the runner thread; receives the data object and returns an int
  // return code.
  using TaskRunnable = std::function<int(DataObjectPtr)>;
  // Called on the construction thread after Run() completes; receives the
  // return code and data object.
  using TaskCallback = std::function<void(int, DataObjectPtr)>;

  // Sentinel return code indicating the task has not yet completed.
  static const int kInitialRTN = -99;

  /**
   * @brief RAII handle for a Task that allows deferred start or cancellation.
   */
  class TaskHandler {
   public:
    /**
     * @brief Wrap the given raw task pointer.
     *
     * @param task raw pointer to the task; must outlive the handler unless
     *             HoldOnLifeCycle(true) is set
     */
    explicit TaskHandler(Task* task);

    /**
     * @brief Schedule the task for execution on its runner thread.
     */
    void Start();

    /**
     * @brief Cancel the task before it starts, if possible.
     */
    void Cancel();

    /**
     * @brief Return the raw task pointer held by this handler.
     *
     * @return raw task pointer, or nullptr if the task has been destroyed
     */
    auto GetTask() -> Task*;

   private:
    QPointer<Task> task_;
  };

  /**
   * @brief Construct a named task with no runnable; subclasses override Run().
   *
   * @param name human-readable name for logging and identification
   */
  explicit Task(QString name);

  /**
   * @brief Construct a task with a runnable and no callback.
   *
   * @param runnable function executed on the runner thread
   * @param name human-readable name
   * @param data_object data passed to the runnable (default: nullptr)
   */
  explicit Task(TaskRunnable runnable, QString name,
                DataObjectPtr data_object = nullptr);

  /**
   * @brief Construct a task with a runnable and a result callback.
   *
   * @param runnable function executed on the runner thread
   * @param name human-readable name
   * @param data data object passed to the runnable and callback
   * @param callback function invoked on the construction thread after
   * completion
   */
  explicit Task(TaskRunnable runnable, QString name, DataObjectPtr data,
                TaskCallback callback);

  ~Task() override;

  /**
   * @brief Return the task's unique UUID string.
   *
   * @return UUID string
   */
  [[nodiscard]] auto GetUUID() const -> QString;

  /**
   * @brief Return the full identifier in the form "UUID/name".
   *
   * @return full identifier string
   */
  [[nodiscard]] auto GetFullID() const -> QString;

  /**
   * @brief Control whether QRunnable auto-deletes the task after execution.
   *
   * Call with @p hold_on = true when the task's lifetime must extend beyond
   * the QRunnable::run() call (e.g. for event-loop-based chunked work).
   *
   * @param hold_on if true, disables auto-deletion; if false, enables it
   */
  void HoldOnLifeCycle(bool hold_on);

  /**
   * @brief Execute the task's work; may be overridden by subclasses.
   *
   * The default implementation calls the TaskRunnable if one was supplied.
   * Subclasses that override this should call setRTN() with the result.
   *
   * @return return code stored as the task's result (0 = success)
   */
  virtual auto Run() -> int;

  /**
   * @brief Return the return code produced by the most recent Run() call.
   *
   * Returns kInitialRTN if Run() has not yet been called.
   *
   * @return task return code
   */
  [[nodiscard]] auto GetRTN() -> int;

 public slots:
  /**
   * @brief Exception-safe entry point that invokes Run() and handles cleanup.
   *
   * Called by the TaskRunner; should not be overridden by subclasses.
   */
  void SafelyRun();

 signals:
  /**
   * @brief Emitted when the task begins executing on the runner thread.
   */
  void SignalRun();

  /**
   * @brief Emitted to request task termination with the given return code.
   *
   * @param rtn return code to deliver to the callback
   */
  void SignalTaskShouldEnd(int rtn);

  /**
   * @brief Emitted after the task's callback has been invoked and the lifecycle
   * is complete.
   */
  void SignalTaskEnd();

 protected:
  /**
   * @brief Set the return code that will be passed to the callback.
   *
   * @param rtn return code
   */
  void setRTN(int rtn);

 private slots:
  /**
   * @brief QRunnable-level execution slot; catches all exceptions and emits
   * SignalTaskShouldEnd.
   */
  void slot_exception_safe_run() noexcept;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;

  // QRunnable::run() override — invokes SafelyRun() via the event loop.
  void run() override;
};
}  // namespace GpgFrontend::Thread
