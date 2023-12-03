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

#include "core/thread/CtxCheckTask.h"

#include "core/GpgCoreInit.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "thread/Task.h"

namespace GpgFrontend {

Thread::CoreInitTask::CoreInitTask() : Task("ctx_check_task") {
  connect(this, &CoreInitTask::SignalBadGnupgEnv,
          CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalBadGnupgEnv);
}

void Thread::CoreInitTask::Run() {
  // Init GpgFrontend Core
  InitGpgFrontendCore();

  // Create & Check Gnupg Context Status
  if (!GpgContext::GetInstance().Good()) {
    emit SignalBadGnupgEnv();
  }
  // Try flushing key cache
  else {
    GpgKeyGetter::GetInstance().FlushKeyCache();
  }

  SPDLOG_DEBUG("ctx check task runnable done");
}
}  // namespace GpgFrontend
