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

#include <atomic>
#include <thread>
#include <vector>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleLoadStats.h"
#include "core/module/ModuleStore.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "sdk/GFSDKModuleAttribution.h"

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

/**
 * @brief Run phase one for every discovered module, several at a time.
 *
 * One thread per module up to a cap, which is plenty: the work is I/O and
 * hashing, there are a handful of modules, and the existing concurrency
 * primitive in this tree is likewise a thread per task.
 *
 * Order is preserved so that what loads in phase two does not depend on which
 * verification happened to finish first -- a build whose module order changes
 * run to run is a build that is harder to reason about.
 */
auto PrepareModulesConcurrently(GpgFrontend::Module::ModuleManager& manager,
                                const QMap<QString, bool>& modules)
    -> QList<GpgFrontend::Module::ModuleLoadCandidate> {
  QList<QPair<QString, bool>> discovered;
  discovered.reserve(modules.size());
  for (auto it = modules.keyValueBegin(); it != modules.keyValueEnd(); ++it) {
    discovered.append({it->first, it->second});
  }

  QList<GpgFrontend::Module::ModuleLoadCandidate> prepared;
  prepared.resize(discovered.size());
  if (discovered.isEmpty()) return prepared;

  const auto hardware = static_cast<int>(std::thread::hardware_concurrency());
  const auto workers =
      std::max(1, std::min<int>(discovered.size(), hardware > 0 ? hardware : 1));

  std::atomic<int> next{0};
  std::vector<std::thread> pool;
  pool.reserve(static_cast<size_t>(workers));

  for (auto w = 0; w < workers; ++w) {
    pool.emplace_back([&]() {
      while (true) {
        const auto i = next.fetch_add(1);
        if (i >= discovered.size()) return;
        prepared[i] = manager.PrepareModule(discovered.at(i).first,
                                            discovered.at(i).second);
      }
    });
  }
  for (auto& t : pool) t.join();

  return prepared;
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

            ModuleLoadStats::GetInstance().Begin();

            QMap<QString, bool> modules = LoadIntegratedMods(packaged_only);

            // if user want to load all modules, then check external modules
            if (module_loading_policy == "all" || packaged_only) {
              LOG_I() << "loading external modules as well since user settings "
                         "is set to load all modules";
              modules.insert(LoadExternalMods(packaged_only));
            }

            auto& manager = ModuleManager::GetInstance();

            // PHASE ONE, concurrent: verify and install. This maps no image
            // and runs no module code, so several can run at once -- and it
            // is essentially the whole cost of loading.
            auto prepared = PrepareModulesConcurrently(manager, modules);

            // A package supersedes the loose library it carries. A build tree
            // holds both -- `mod_email.gfmodule` and
            // `libgf_mod_mod_email.so` -- and offering both loaded one module
            // identity twice, the second copy failing registration after
            // paying in full to be verified. The names do not correspond
            // (the package is named for the CMake target, the library for the
            // SDK prefix), so the package's own manifest is what says which
            // library it provides.
            QSet<QString> provided_by_package;
            for (const auto& candidate : prepared) {
              if (!candidate.ok || !candidate.packaged) continue;
              provided_by_package.insert(
                  QFileInfo(candidate.library_path).fileName());
            }

            QList<ModuleLoadCandidate> to_load;
            to_load.reserve(prepared.size());
            for (const auto& candidate : prepared) {
              if (!candidate.packaged &&
                  provided_by_package.contains(
                      QFileInfo(candidate.source_path).fileName())) {
                LOG_D() << "skipping loose module superseded by a package: "
                        << candidate.source_path;
                continue;
              }
              to_load.append(candidate);
            }

            // Counted after superseding, so the number the manager waits for
            // is the number that will actually be attempted.
            manager.SetNeedRegisterModulesNum(static_cast<int>(to_load.size()));

            // PHASE TWO, sequential: map each library and register it.
            // QLibrary::load() runs the module's own static initialisers, and
            // the host cannot establish that one module's are safe against
            // another's -- so this half stays one at a time, on purpose.
            for (const auto& candidate : to_load) {
              manager.LoadPreparedModule(candidate);
            }

            // After loading, not before: what is collected is whatever no
            // longer has anything pointing at it, and this start's installs
            // are what decide that. A crashed install leaves dot-prefixed
            // staging, and a version superseded twice can no longer be
            // reached even by a rollback.
            const auto store_root = ModuleStoreRoot(
                GlobalSettingStation::GetInstance().GetModulesDir());
            const auto collected = SweepModuleStore(store_root);
            if (collected > 0) {
              LOG_I() << "module store: removed" << collected
                      << "unreachable directories";
            }

            // Stated rather than left to be inferred from the gap between
            // two log lines, which is how this was got wrong twice.
            LOG_I() << "module loading finished:"
                    << ModuleLoadStats::GetInstance().Summary();
            return 0;
          },
          "modules_system_init_task"));

  LOG_D() << "are all modules registered? answer: "
          << ModuleManager::GetInstance().IsAllModulesRegistered();
}

/**
 * @brief Whether step 6 actually unmaps the libraries.
 *
 * Off, on evidence rather than on caution. With it on, roughly one run in four
 * died under ASan in `QArrayDataPointer<char16_t>::data()` -- a QString whose
 * storage is a QStringLiteral in a module's own read-only data. Such a string
 * costs nothing to copy and is shared by pointer, so any one of them that
 * outlives the module (an event id, a registry key, a translation) dangles the
 * moment the image is unmapped, and the crash lands far from the cause.
 *
 * Nothing is gained by unmapping at process exit: the process is ending. What
 * unloading is *for* is replacing a module without restarting, and that needs
 * the host to own every string that came from a module, which is a larger
 * piece of work than the ordering here. Everything that makes it possible is
 * in place -- the Module owns its library, the registries let go first, and
 * this is the right point in the sequence -- so it is one constant away when
 * that work is done.
 */
constexpr bool kUnloadLibrariesAtShutdown = false;

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

  // 5. SWEEP OUTSTANDING SDK HANDLES. Only now is the ledger authoritative:
  //    no module code can run, so anything a module still holds is
  //    definitively leaked rather than merely in use. Each one is logged
  //    against the module and the entry point that issued it, then wiped and
  //    freed -- which for a secret means it stops living in the heap for the
  //    rest of the process rather than merely being unreachable.
  size_t swept = 0;
  for (const auto& module_id : module_ids) {
    swept += GFSdkSweepModuleHandles(module_id.toUtf8().constData());
  }

  // 6. UNLOAD THE LIBRARIES -- last, so that no module code is unmapped while
  //    a thread could still be inside it, which is what steps 1 to 3
  //    established and the only reason this point is safe at all.
  //
  //    Gated off; see kUnloadLibrariesAtShutdown for the measurement that
  //    decided it. The registries still let go here, which is the half that
  //    matters at shutdown: nothing can route an event into a module any more.
  //
  //    The registries let go first. Their ModulePtr is what an event would be
  //    routed through, so dropping it is what makes "nothing can call into
  //    this module" true rather than merely likely; unloading before that
  //    would leave the routing table pointing into unmapped code.
  //
  //    Unloading is asked of each module rather than inferred from a refcount
  //    reaching zero: other holders may still have a reference, and a module
  //    that has been unloaded is inert rather than dangling -- it drops its
  //    own function table and refuses every later call.
  auto modules = manager.TakeAllModules();
  auto unloaded = 0;
  if (kUnloadLibrariesAtShutdown) {
    for (const auto& module : modules) {
      if (module != nullptr && module->UnloadLibrary()) ++unloaded;
    }
  }

  LOG_D() << "module system shut down cleanly, modules:" << module_ids.size()
          << ", sdk handles reclaimed:" << swept
          << ", libraries unloaded:" << unloaded;
}

}  // namespace GpgFrontend::Module