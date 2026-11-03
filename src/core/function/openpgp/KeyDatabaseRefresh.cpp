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

#include "core/function/openpgp/KeyDatabaseRefresh.h"

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/model/DataObject.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

void RefreshKeyDatabases(QObject* receiver, std::function<void()> done) {
  auto* refresh_task = new Thread::Task(
      [](DataObjectPtr) -> int {
        // Flush the key cache of every database in the Runtime in use, each
        // under a lease so a reload cannot reclaim it mid-flush. A retired
        // Runtime's channels are not refreshed: nothing new is read from them
        // any more.
        for (const auto& info : GetGpgKeyDatabaseInfos()) {
          auto lease = TryAcquireEngineLease(info.channel);
          if (!lease) continue;
          LOG_D() << "refreshing key database at channel: " << info.channel;
          AbstractKeyRepository::GetInstance(info.channel).FlushCache();
        }
        LOG_D() << "refreshing key database at all channel done";
        return 0;
      },
      "update_key_database_task");

  QObject::connect(refresh_task, &Thread::Task::SignalTaskEnd, receiver,
                   [done = std::move(done)]() {
                     if (done) done();
                   });

  LOG_D() << "sending key database refresh task to gpg task runner...";
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(refresh_task);
}

}  // namespace GpgFrontend
