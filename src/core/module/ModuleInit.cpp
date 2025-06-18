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

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <QCoreApplication>
#include <QDir>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace {

auto SearchModuleFromPath(const QString& mods_path, bool integrated)
    -> QMap<QString, bool> {
  QMap<QString, bool> modules;

  QDir dir(mods_path);
  if (!dir.exists()) return modules;

  const auto entries = dir.entryInfoList(
      QStringList() << "*.so" << "*.dll" << "*.dylib", QDir::Files);

  const QRegularExpression rx(QStringLiteral("^libgf_mod_.+$"));

  for (const auto& info : entries) {
    if (rx.match(info.fileName()).hasMatch()) {
      modules.insert(info.absoluteFilePath(), integrated);
    }
  }

  return modules;
}

auto LoadIntegratedMods() -> QMap<QString, bool> {
  const auto module_path = GpgFrontend::GlobalSettingStation::GetInstance()
                               .GetIntegratedModulePath();
  LOG_I() << "loading integrated modules from path:" << module_path;

  if (!QDir(module_path).exists()) {
    LOG_W() << "integrated modules at path: " << module_path
            << " not found, abort...";
    return {};
  }

  return SearchModuleFromPath(module_path, true);
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

}  // namespace

#if defined(Q_OS_WINDOWS)

#include <softpub.h>
#include <windows.h>
#include <wintrust.h>

namespace {

auto ValidateModule(const QString& path) -> bool {
  LONG l_status;
  GUID wvt_policy_guid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

  WINTRUST_FILE_INFO file_data = {0};
  file_data.cbStruct = sizeof(WINTRUST_FILE_INFO);
  file_data.pcwszFilePath = (LPCWSTR)path.utf16();
  file_data.hFile = NULL;
  file_data.pgKnownSubject = NULL;

  WINTRUST_DATA win_trust_data = {0};
  win_trust_data.cbStruct = sizeof(WINTRUST_DATA);
  win_trust_data.dwUIChoice = WTD_UI_NONE;
  win_trust_data.fdwRevocationChecks = WTD_REVOKE_NONE;
  win_trust_data.dwUnionChoice = WTD_CHOICE_FILE;
  win_trust_data.pFile = &file_data;
  win_trust_data.dwStateAction = 0;
  win_trust_data.hWVTStateData = NULL;
  win_trust_data.pwszURLReference = NULL;
  win_trust_data.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;

  l_status = WinVerifyTrust(NULL, &wvt_policy_guid, &win_trust_data);

  return (l_status == ERROR_SUCCESS);
}

}  // namespace

#else

namespace {

auto ValidateModule(const QString& path) -> bool { return true; };

}  // namespace

#endif

namespace GpgFrontend::Module {

void LoadGpgFrontendModules(ModuleInitArgs) {
  // give user ability to give up all modules
  auto disable_loading_all_modules =
      GetSettings().value("basic/disable_loading_all_modules", false).toBool();
  if (disable_loading_all_modules) return;

  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
      ->PostTask(new Thread::Task(
          [](const DataObjectPtr&) -> int {
            QMap<QString, bool> modules;

            // if do self checking
            const auto self_check = qApp->property("GFSelfCheck").toBool();

            // only check integrated modules at first
            QMap<QString, bool> integrated_modules = LoadIntegratedMods();
            for (auto it = integrated_modules.keyValueBegin();
                 it != integrated_modules.keyValueEnd(); ++it) {
              if (self_check && it->second && !ValidateModule(it->first)) {
                LOG_W() << "refuse to load integrated module: " << it->first;
                continue;
              }
              modules.insert(it->first, it->second);
            }

            modules.insert(LoadExternalMods());

            auto& manager = ModuleManager::GetInstance();
            manager.SetNeedRegisterModulesNum(static_cast<int>(modules.size()));

            for (auto it = modules.keyValueBegin(); it != modules.keyValueEnd();
                 ++it) {
              manager.LoadModule(it->first, it->second);
            }

            LOG_D() << "all modules are loaded into memory: " << modules.size();
            return 0;
          },
          "modules_system_init_task"));

  LOG_D() << "are all modules registered? answer: "
          << ModuleManager::GetInstance().IsAllModulesRegistered();
}

void ShutdownGpgFrontendModules() {}

}  // namespace GpgFrontend::Module