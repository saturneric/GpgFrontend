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

#include <QDir>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace {

auto SearchModuleFromPath(const QString& mods_path, bool integrated,
                          bool packaged_only) -> QMap<QString, bool> {
  QMap<QString, bool> modules;

  QDir dir(mods_path);
  if (!dir.exists()) return modules;

  const auto entries =
      dir.entryInfoList(QStringList() << "*.so" << "*.dll" << "*.dylib"
                                      << "*.gfmodule",
                        QDir::Files);

  for (const auto& info : entries) {
    // the same rules the pre-load gate applies, so the scan cannot offer a
    // file that LoadModule() would then refuse
    if (GpgFrontend::Module::IsModulePackageFileName(info.fileName())) {
      modules.insert(info.absoluteFilePath(), integrated);
      continue;
    }
    if (packaged_only) continue;
    if (GpgFrontend::Module::IsModuleLibraryFileName(info.fileName())) {
      modules.insert(info.absoluteFilePath(), integrated);
    }
  }

  return modules;
}

auto LoadIntegratedMods(bool packaged_only) -> QMap<QString, bool> {
  const auto module_path = GpgFrontend::GlobalSettingStation::GetInstance()
                               .GetIntegratedModulePath();
  LOG_I() << "loading integrated modules from path:" << module_path;

  if (!QDir(module_path).exists()) {
    LOG_W() << "integrated modules at path: " << module_path
            << " not found, abort...";
    return {};
  }

  return SearchModuleFromPath(module_path, true, packaged_only);
}

auto LoadExternalMods(bool packaged_only) -> QMap<QString, bool> {
  auto mods_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetModulesDir();

  if (!QDir(mods_path).exists()) {
    LOG_W() << "external module directory at path " << mods_path
            << " not found, abort...";
    return {};
  }

  return SearchModuleFromPath(mods_path, false, packaged_only);
}

}  // namespace

namespace GpgFrontend::Module {

void LoadGpgFrontendModules(ModuleInitArgs) {
  const auto module_loading_policy =
      GetSettings()
          .value("basic/module_loading_policy", "only_integrated")
          .toString();

  if (module_loading_policy == "disable") {
    LOG_I() << "module loading is disabled by user settings, abort...";
    ModuleManager::GetInstance().SetNeedRegisterModulesNum(0);
    return;
  }

  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
      ->PostTask(new Thread::Task(
          [module_loading_policy](const DataObjectPtr&) -> int {
            // "packaged_only" is a level above "all", not beside it: it
            // loads everything, and refuses to consider a loose library that
            // nothing vouches for.
            //
            // This is the direction of travel, not a niche option: loose
            // module libraries are transitional, and a future version will
            // stop loading them. It is opt-in for now only because the four
            // in-tree modules still ship loose, and because a signature today
            // establishes that a package agrees with itself rather than who
            // built it -- so making it the default would cost users their own
            // builds and buy them less than it appears to.
            const auto packaged_only = module_loading_policy == "packaged_only";

            QMap<QString, bool> modules = LoadIntegratedMods(packaged_only);

            // if user want to load all modules, then check external modules
            if (module_loading_policy == "all" || packaged_only) {
              LOG_I() << "loading external modules as well since user settings "
                         "is set to load all modules";
              modules.insert(LoadExternalMods(packaged_only));
            }

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

void ShutdownGpgFrontendModules() {
  // The ordering here is the contract, and every step exists because skipping
  // it turns a tidy shutdown into a use-after-free. This function used to be
  // empty: nothing was deactivated, nothing was unregistered, no library was
  // ever unloaded, and nothing waited for module work to finish.
  auto& manager = ModuleManager::GetInstance();
  auto& gate = GlobalModuleDispatchGate();

  // 1. STOP NEW CALLS. From here the set of in-flight calls can only shrink.
  //    An event that arrives after this point is refused with
  //    kModuleUnloadingCode rather than being queued into a module that is
  //    about to go away.
  gate.Close();

  // 2. DEACTIVATE. Gives each module its chance to cancel its own in-flight
  //    work and drop the registrations it owns (settings pages, tab pages),
  //    which is why it runs before the wait rather than after it.
  const auto module_ids = manager.ListAllRegisteredModuleID();
  for (const auto& module_id : module_ids) {
    if (!manager.IsModuleActivated(module_id)) continue;
    manager.DeactivateModule(module_id);
  }

  // 3. QUIESCE. The step that did not exist. Without it, everything below
  //    races module code that is still running: unregistering state it is
  //    using, reclaiming handles it still holds, unmapping the code itself.
  //
  //    A timeout is REPORTED, not swallowed. Continuing to tear down after
  //    one would be exactly the hazard this ordering exists to prevent, so
  //    the later steps are skipped and the modules are left mapped -- leaking
  //    on the way out of a process that is exiting anyway is strictly better
  //    than freeing memory somebody is still reading.
  constexpr int kQuiesceTimeoutMs = 5000;
  if (!gate.WaitQuiescent(kQuiesceTimeoutMs)) {
    LOG_W() << "module system did not go quiet within" << kQuiesceTimeoutMs
            << "ms;" << gate.InFlight()
            << "call(s) still inside module code. Skipping teardown rather "
               "than freeing resources they may still be using.";
    return;
  }

  // 4. DESTROY MODULE-OWNED STATE. Only now is it safe: no module code runs.
  for (const auto& module_id : module_ids) {
    auto module = manager.SearchModule(module_id);
    if (module == nullptr) continue;
    module->UnRegister();
  }

  LOG_D() << "module system shut down cleanly, modules:" << module_ids.size();

  // Steps 5 (sweep outstanding SDK handles) and 6 (unload the libraries) are
  // deliberately not here yet. The sweep needs the SDK's per-module handle
  // ledger, which currently records no module id; unloading needs the module
  // objects to be destroyed first, and they are owned elsewhere. Both are
  // safe to add at this point in the sequence precisely because step 3 has
  // already established that nothing is running.
}

}  // namespace GpgFrontend::Module