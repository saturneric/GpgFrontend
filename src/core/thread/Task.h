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
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
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

  friend class TaskRunner;

  /**
   * @brief DataObject to be passed to the callback function.
   *
   */
  class GPGFRONTEND_CORE_EXPORT DataObject {
   public:
    struct Destructor {
      const void *p_obj;
      void (*destroy)(const void *);
    };

    /**
     * @brief Get the Objects Size
     *
     * @return size_t
     */
    size_t GetObjectSize();

    /**
     * @brief
     *
     * @tparam T
     * @param ptr
     */
    template <typename T>
    void AppendObject(T &&obj) {
      DLOG(TRACE) << "called:" << this;
      auto *obj_dstr = this->get_heap_ptr(sizeof(T));
      new ((void *)obj_dstr->p_obj) T(std::forward<T>(obj));

      if (std::is_class_v<T>) {
        auto destructor = [](const void *x) {
          static_cast<const T *>(x)->~T();
        };
        obj_dstr->destroy = destructor;
      } else {
        obj_dstr->destroy = nullptr;
      }

      data_objects_.push(obj_dstr);
    }

    /**
     * @brief
     *
     * @tparam T
     * @param ptr
     */
    template <typename T>
    void AppendObject(T *obj) {
      DLOG(TRACE) << "called:" << this;
      auto *obj_dstr = this->get_heap_ptr(sizeof(T));
      auto *ptr_heap = new ((void *)obj_dstr->p_obj) T(std::move(*obj));
      if (std::is_class_v<T>) {
        LOG(TRACE) << "is class";
        auto destructor = [](const void *x) {
          static_cast<const T *>(x)->~T();
        };
        obj_dstr->destroy = destructor;
      } else {
        obj_dstr->destroy = nullptr;
      }
      data_objects_.push(std::move(obj_dstr));
    }

    /**
     * @brief
     *
     * @tparam T
     * @return std::shared_ptr<T>
     */
    template <typename T>
    T PopObject() {
      DLOG(TRACE) << "called:" << this;
      if (data_objects_.empty()) throw std::runtime_error("No object to pop");
      auto *obj_dstr = data_objects_.top();
      auto *heap_ptr = (T *)obj_dstr->p_obj;
      auto obj = std::move(*(T *)(heap_ptr));
      this->free_heap_ptr(obj_dstr);
      data_objects_.pop();
      return obj;
    }

    /**
     * @brief Destroy the Data Object object
     *
     */
    ~DataObject();

   private:
    std::stack<Destructor *> data_objects_;  ///<

    /**
     * @brief Get the heap ptr object
     *
     * @param bytes_size
     * @return void*
     */
    Destructor *get_heap_ptr(size_t bytes_size);

    /**
     * @brief
     *
     * @param heap_ptr
     */
    void free_heap_ptr(Destructor *);
  };

  /**
   * @brief Construct a new Task object
   *
   */
  Task();

  /**
   * @brief Construct a new Task object
   *
   * @param callback The callback function to be executed.
   */
  explicit Task(TaskCallback callback, DataObjectPtr data_object = nullptr);

  /**
   * @brief Construct a new Task object
   *
   * @param runnable
   */
  explicit Task(
      TaskRunnable runnable,
      TaskCallback callback = [](int, const std::shared_ptr<DataObject> &) {},
      DataObjectPtr data = nullptr);

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

 signals:
  /**
   * @brief
   *
   */
  void SignalTaskFinished();

  /**
   * @brief
   *
   */
  void SignalTaskPostFinishedDone();

 protected:
  /**
   * @brief Set the Finish After Run object
   *
   * @param finish_after_run
   */
  void SetFinishAfterRun(bool finish_after_run);

  /**
   * @brief
   *
   * @param rtn
   */
  void SetRTN(int rtn);

 private:
  const std::string uuid_;
  TaskCallback callback_;                ///<
  TaskRunnable runnable_;                ///<
  bool finish_after_run_ = true;         ///<
  int rtn_ = 0;                          ///<
  QThread *callback_thread_ = nullptr;   ///<
  DataObjectPtr data_object_ = nullptr;  ///<

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

  /**
   * @brief
   *
   * @return std::string
   */
  static std::string generate_uuid();
};
}  // namespace GpgFrontend::Thread

#endif  // GPGFRONTEND_TASK_H