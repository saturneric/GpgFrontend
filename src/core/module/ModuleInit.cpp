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

#include "ModuleInit.h"

#include <QCoreApplication>
#include <QDir>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend::Module {

auto SearchModuleFromPath(const QString& mods_path,
                          bool integrated) -> QMap<QString, bool> {
  QMap<QString, bool> m;
  for (const auto& module_library_name : QDir(mods_path).entryList(
           QStringList() << "*.so" << "*.dll" << "*.dylib", QDir::Files)) {
    m[mods_path + "/" + module_library_name] = integrated;
  }
  return m;
}

auto LoadIntegratedMods() -> QMap<QString, bool> {
  const auto exec_binary_path = QCoreApplication::applicationDirPath();
  QString mods_path = exec_binary_path + "/modules";

#ifdef NDEBUG

#if defined(__APPLE__) && defined(__MACH__)
  // App Bundle
  mods_path = exec_binary_path + "/../Modules";
#elif defined(__linux__)
  // AppImage
  if (!qEnvironmentVariable("APPIMAGE").isEmpty()) {
    mods_path = qEnvironmentVariable("APPDIR") + "/usr/modules";
  }
  // Flatpak
  if (!qEnvironmentVariable("container").isEmpty()) {
    mods_path = "/app/modules";
  }
#endif

#endif

  if (!QDir(mods_path).exists()) {
    LOG_W() << "integrated module directory at path: " << mods_path
            << " not found, abort...";
    return {};
  }

  return SearchModuleFromPath(mods_path, true);
}

auto LoadExternalMods() -> QMap<QString, bool> {
  auto mods_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetModulesDir();

  if (!QDir(mods_path).exists()) {
    LOG_W() << "external module directory at path " << mods_path
            << " not found, abort...";
    return {};
  }

  return SearchModuleFromPath(mods_path, false);
}

void LoadGpgFrontendModules(ModuleInitArgs) {
  // give user ability to give up all modules
  auto disable_loading_all_modules =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("basic/disable_loading_all_modules", false)
          .toBool();
  if (disable_loading_all_modules) return;

  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
      ->PostTask(new Thread::Task(
          [](const DataObjectPtr&) -> int {
            QMap<QString, bool> modules = LoadIntegratedMods();
            modules.insert(LoadExternalMods());

            auto& manager = ModuleManager::GetInstance();
            manager.SetNeedRegisterModulesNum(static_cast<int>(modules.size()));

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
            for (const auto& m : modules.asKeyValueRange()) {
              manager.LoadModule(m.first, m.second);
            }
#else
            for (auto it = modules.keyValueBegin(); it != modules.keyValueEnd();
                 ++it) {
              manager.LoadModule(it->first, it->second);
            }
#endif

            LOG_D() << "all modules are loaded into memory.";
            return 0;
          },
          "modules_system_init_task"));

  LOG_D() << "dear module manager, is all module registered? answer: "
          << ModuleManager::GetInstance().IsAllModulesRegistered();
}

void ShutdownGpgFrontendModules() {}

}  // namespace GpgFrontend::Module