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

#include "GpgFrontendModuleInit.h"

#include <core/module/ModuleManager.h>

#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

// integrated modules
#include "integrated/gnupg_info_gathering_module/GnuPGInfoGatheringModule.h"
#include "integrated/version_checking_module/VersionCheckingModule.h"
#include "spdlog/common.h"

namespace GpgFrontend::Module {

void LoadGpgFrontendModules(ModuleInitArgs args) {
  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance().GetTaskRunner()->PostTask(
      new Thread::Task(
          [args](const DataObjectPtr&) -> int {
            MODULE_LOG_INFO("loading integrated module...");

            // VersionCheckingModule
            RegisterAndActivateModule<
                Integrated::VersionCheckingModule::VersionCheckingModule>();

            // VersionCheckingModule
            RegisterAndActivateModule<Integrated::GnuPGInfoGatheringModule::
                                          GnuPGInfoGatheringModule>();

            MODULE_LOG_INFO("load integrated module done.");
            return 0;
          },
          "modules_system_init_task"));
}

void ShutdownGpgFrontendModules() {}

}  // namespace GpgFrontend::Module