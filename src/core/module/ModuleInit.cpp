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

#include "ModuleInit.h"

#include <QCoreApplication>
#include <QDir>

#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend::Module {

void LoadGpgFrontendModules(ModuleInitArgs) {
  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance().GetTaskRunner()->PostTask(
      new Thread::Task(
          [](const DataObjectPtr&) -> int {
            GF_CORE_LOG_INFO("loading modules...");

            auto exec_binary_path = QCoreApplication::applicationDirPath();
            auto mods_path = exec_binary_path + "/mods";

            if (!QDir(mods_path).exists()) {
              GF_CORE_LOG_INFO("module directory not found, abort...");
              return -1;
            }

            GF_CORE_LOG_INFO("the path of modules directory: {}", mods_path);

            for (const auto& module_library_name :
                 QDir(mods_path).entryList(QStringList() << "*.so"
                                                         << "*.dll"
                                                         << "*.dylib",
                                           QDir::Files)) {
              ModuleManager::GetInstance().LoadModule(mods_path + "/" +
                                                      module_library_name);
            }

            GF_CORE_LOG_INFO("load modules done.");
            return 0;
          },
          "modules_system_init_task"));
}

void ShutdownGpgFrontendModules() {}

}  // namespace GpgFrontend::Module