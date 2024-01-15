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

#include "AsyncUtils.h"

#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CommonUtils.h"
#include "model/DataObject.h"

namespace GpgFrontend {

auto RunGpgOperaAsync(GpgOperaRunnable runnable, GpgOperationCallback callback,
                      const QString& operation, const QString& minial_version)
    -> Thread::Task::TaskHandler {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", minial_version);
  GF_CORE_LOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

  if (CompareSoftwareVersion(gnupg_version, "2.0.15") < 0) {
    GF_CORE_LOG_ERROR("operator not support");
    callback(GPG_ERR_NOT_SUPPORTED, TransferParams());
    return Thread::Task::TaskHandler(nullptr);
  }

  auto handler =
      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
          ->RegisterTask(
              operation,
              [=](const DataObjectPtr& data_object) -> int {
                auto custom_data_object = TransferParams();
                GpgError err = runnable(custom_data_object);

                data_object->Swap({err, custom_data_object});
                return 0;
              },
              [=](int, const DataObjectPtr& data_object) {
                callback(ExtractParams<GpgError>(data_object, 0),
                         ExtractParams<DataObjectPtr>(data_object, 1));
              },
              TransferParams());
  handler.Start();
  return handler;
}

auto RunIOOperaAsync(OperaRunnable runnable, OperationCallback callback,
                     const QString& operation) -> Thread::Task::TaskHandler {
  auto handler =
      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_IO)
          ->RegisterTask(
              operation,
              [=](const DataObjectPtr& data_object) -> int {
                auto custom_data_object = TransferParams();
                GpgError err = runnable(custom_data_object);

                data_object->Swap({err, custom_data_object});
                return 0;
              },
              [=](int, const DataObjectPtr& data_object) {
                callback(ExtractParams<GFError>(data_object, 0),
                         ExtractParams<DataObjectPtr>(data_object, 1));
              },
              TransferParams());
  handler.Start();
  return handler;
}
}  // namespace GpgFrontend