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

#ifndef GPGFRONTEND_SDK_TASK_H
#define GPGFRONTEND_SDK_TASK_H

#include <string>

#include "GpgFrontendPluginSDK.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Plugin::SDK {

using STaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class GPGFRONTEND_PLUGIN_SDK_EXPORT STask : public Thread::Task {
  Q_OBJECT
 public:
  using STaskName = std::string;
  using SDataObjectPtr = std::shared_ptr<DataObject>;              ///<
  using STaskRunnable = std::function<int(SDataObjectPtr)>;        ///<
  using STaskCallback = std::function<void(int, SDataObjectPtr)>;  ///<

  /**
   * @brief Construct a new Task object
   *
   */
  STask(STaskName name = DEFAULT_TASK_NAME);

  /**
   * @brief Construct a new Task object
   *
   * @param callback The callback function to be executed.
   */
  explicit STask(STaskName name, STaskRunnable runnable,
                 SDataObjectPtr data_object = nullptr);

  /**
   * @brief Construct a new Task object
   *
   * @param runnable
   */
  explicit STask(
      STaskName name, STaskRunnable runnable, SDataObjectPtr data_object,
      STaskCallback callback = [](int, const SDataObjectPtr &) {});

  virtual ~STask() = default;
};

void PostTask(Thread::TaskRunner *, STask *);

}  // namespace GpgFrontend::Plugin::SDK

#endif  // GPGFRONTEND_SDK_TASK_H
