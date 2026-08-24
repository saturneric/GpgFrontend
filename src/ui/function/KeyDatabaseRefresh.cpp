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

#include "ui/function/KeyDatabaseRefresh.h"

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {

namespace {

bool g_handler_installed = false;

}  // namespace

void InstallKeyDatabaseRefreshHandler() {
  if (g_handler_installed) return;
  g_handler_installed = true;

  QObject::connect(
      UISignalStation::GetInstance(),
      &UISignalStation::SignalKeyDatabaseRefresh,
      UISignalStation::GetInstance(), []() -> void {
        auto* refresh_task = new Thread::Task(
            [](DataObjectPtr) -> int {
              // flush key cache for all GpgKeyGetter Intances.
              for (const auto& channel_id : OpenPGPContext::GetAllChannelId()) {
                LOG_D() << "refreshing key database at channel: " << channel_id;
                AbstractKeyRepository::GetInstance(channel_id).FlushCache();
              }
              LOG_D() << "refreshing key database at all channel done";
              return 0;
            },
            "update_key_database_task");

        QObject::connect(refresh_task, &Thread::Task::SignalTaskEnd,
                         UISignalStation::GetInstance(),
                         &UISignalStation::SignalKeyDatabaseRefreshDone);

        // post the task to the default task runner
        LOG_D() << "sending key database refresh task to gpg task runner...";
        Thread::TaskRunnerGetter::GetInstance()
            .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
            ->PostTask(refresh_task);
      });
}

}  // namespace GpgFrontend::UI
