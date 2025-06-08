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

#include "core/thread/Task.h"

#include <qscopedpointer.h>

#include "utils/MemoryUtils.h"

namespace GpgFrontend::Thread {

class Task::Impl {
 public:
  Impl(Task *parent, QString name)
      : parent_(parent), uuid_(generate_uuid()), name_(std::move(name)) {
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, QString name,
       DataObjectPtr data_object)
      : parent_(parent),
        uuid_(generate_uuid()),
        name_(std::move(name)),
        runnable_(std::move(runnable)),
        callback_([](int, const DataObjectPtr &) {}),
        callback_thread_(QThread::currentThread()),
        data_object_(std::move(data_object)) {
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, QString name,
       DataObjectPtr data_object, TaskCallback callback)
      : parent_(parent),
        uuid_(generate_uuid()),
        name_(std::move(name)),
        runnable_(std::move(runnable)),
        callback_(std::move(callback)),
        callback_thread_(QThread::currentThread()),
        data_object_(std::move(data_object)) {
    init();
  }

  ~Impl() = default;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetFullID() const -> QString {
    return uuid_ + "/" + name_;
  }

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetUUID() const -> QString { return uuid_; }

  /**
   * @brief
   *
   * @return int
   */
  auto Run() -> int {
    if (runnable_) return runnable_(data_object_);
    return 0;
  }

  /**
   * @brief Set the Finish After Run object
   *
   * @param finish_after_run
   */
  void HoldOnLifeCycle(bool hold_on) { parent_->setAutoDelete(!hold_on); }

  /**
   * @brief
   *
   * @param rtn
   */
  void SetRTN(int rtn) { this->rtn_ = rtn; }

  /**
   * @brief
   *
   * @return auto
   */
  [[nodiscard]] auto GetRTN() const { return this->rtn_; }

 private:
  Task *const parent_;
  const QString uuid_;
  const QString name_;
  TaskRunnable runnable_;                ///<
  TaskCallback callback_;                ///<
  int rtn_ = Task::kInitialRTN;          ///<
  QThread *callback_thread_ = nullptr;   ///<
  DataObjectPtr data_object_ = nullptr;  ///<

  void init() {
    //
    HoldOnLifeCycle(false);

    //
    connect(parent_, &Task::SignalRun, parent_, &Task::slot_exception_safe_run);

    auto *callback_thread = callback_thread_ != nullptr
                                ? callback_thread_
                                : QCoreApplication::instance()->thread();
    //
    connect(parent_, &Task::SignalTaskShouldEnd, callback_thread,
            [this](int rtn) {
              // set task returning code
              SetRTN(rtn);

#ifdef NDEBUG
              try {
                if (callback_) {
                  callback_(rtn_, data_object_);
                }
              } catch (...) {
                LOG_W() << "task: {}, " << GetFullID()
                        << "callback caught exception, rtn: " << rtn;
              }
#else
              if (callback_) {
                callback_(rtn_, data_object_);
              }
#endif

              LOG_D() << "task" << this->name_
                      << "sending task end signal, rtn:" << rtn;
              emit parent_->SignalTaskEnd();
            });

    //
    connect(parent_, &Task::SignalTaskEnd, parent_, &Task::deleteLater);
  }

  /**
   * @brief
   *
   * @return QString
   */
  static auto generate_uuid() -> QString {
    return QUuid::createUuid().toString();
  }
};

Task::Task(QString name)
    : p_(SecureCreateUniqueObject<Task::Impl>(this, name)) {}

Task::Task(TaskRunnable runnable, QString name, DataObjectPtr data_object)
    : p_(SecureCreateUniqueObject<Impl>(this, runnable, name, data_object)) {}

Task::Task(TaskRunnable runnable, QString name, DataObjectPtr data_object,
           TaskCallback callback)
    : p_(SecureCreateUniqueObject<Impl>(this, runnable, name, data_object,
                                        callback)) {}

Task::~Task() = default;

/**
 * @brief
 *
 * @return QString
 */
QString Task::GetFullID() const { return p_->GetFullID(); }

QString Task::GetUUID() const { return p_->GetUUID(); }

void Task::HoldOnLifeCycle(bool hold_on) { p_->HoldOnLifeCycle(hold_on); }

void Task::setRTN(int rtn) { p_->SetRTN(rtn); }

void Task::SafelyRun() { emit SignalRun(); }

int Task::Run() { return p_->Run(); }

void Task::run() { this->SafelyRun(); }

Task::TaskHandler::TaskHandler(Task *task) : task_(task) {}

void Task::TaskHandler::Start() {
  if (task_ != nullptr) task_->SafelyRun();
}

void Task::TaskHandler::Cancel() {
  if (task_ != nullptr) emit task_->SignalTaskEnd();
}

auto Task::TaskHandler::GetTask() -> Task * {
  if (task_ != nullptr) return task_;
  return nullptr;
}

void Task::slot_exception_safe_run() noexcept {
  auto rtn = p_->GetRTN();
  try {
    // Run() will set rtn by itself
    rtn = this->Run();

  } catch (...) {
    LOG_W() << "exception was caught at task: {}" << GetFullID();
  }

  // raise signal to anounce after runnable returned
  if (this->autoDelete()) emit this->SignalTaskShouldEnd(rtn);
}

auto Task::GetRTN() -> int { return p_->GetRTN(); }
}  // namespace GpgFrontend::Thread