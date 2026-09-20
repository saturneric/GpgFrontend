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

#include "core/function/CoreInitProgress.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleLoadStats.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleSdkBridge.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace {

auto SearchModuleFromPath(const QString& mods_path, bool integrated)
    -> QMap<QString, bool> {
  QMap<QString, bool> modules;

  QDir dir(mods_path);
  if (!dir.exists()) return modules;

  // One namespace per module: <root>/<key>/module.gfmodule, with the module's
  // native files beside it under native/. A native library is no longer
  // something that can be *found* -- it is something a verified descriptor
  // *names*, and one sitting in a namespace that no descriptor binds is not a
  // module at all.
  //
  // This is also what retires the package-supersedes-loose-library
  // reconciliation that used to live further down. The two never corresponded
  // by name -- the package was named for the CMake target and the library for
  // the SDK prefix -- and now they do not need to: the descriptor's filename
  // is fixed, and its directory is derived from the identity it signs.
  //
  // This is also why there is no "packaged only" policy to honour here: there
  // is no other kind.

  const auto namespaces =
      dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

  for (const auto& candidate : namespaces) {
    const QFileInfo descriptor(candidate.absoluteFilePath() + "/" +
                               GpgFrontend::Module::kModuleDescriptorFileName);
    // Not every subdirectory is a module namespace, and one that holds no
    // descriptor is simply not offered. Whether the directory NAME is the
    // right one for the identity inside is a question only the verified
    // manifest can answer, so it is asked later, by the manager.
    if (!descriptor.isFile()) continue;
    modules.insert(descriptor.absoluteFilePath(), integrated);
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
  const auto workers = std::max(
      1, std::min<int>(discovered.size(), hardware > 0 ? hardware : 1));

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

auto ParseModuleLoadingPolicy(const QString& key) -> ModuleLoadingPolicyParse {
  if (key == "disable") return {ModuleLoadingPolicy::kDISABLE, true};
  if (key == "only_integrated") {
    return {ModuleLoadingPolicy::kONLY_INTEGRATED, true};
  }
  if (key == "all") return {ModuleLoadingPolicy::kALL, true};
  // Accepted, never written: an alias kept so an existing profile is not
  // reported as corrupt on upgrade. See ModuleInit.h.
  if (key == "packaged_only") return {ModuleLoadingPolicy::kALL, true};
  return {ModuleLoadingPolicy::kONLY_INTEGRATED, false};
}

auto ModuleLoadingPolicyKey(ModuleLoadingPolicy policy) -> QString {
  switch (policy) {
    case ModuleLoadingPolicy::kDISABLE:
      return "disable";
    case ModuleLoadingPolicy::kONLY_INTEGRATED:
      return "only_integrated";
    case ModuleLoadingPolicy::kALL:
      return "all";
  }
  return "only_integrated";
}

void LoadGpgFrontendModules(ModuleInitArgs) {
  const auto stored =
      GetSettings()
          .value("basic/module_loading_policy",
                 ModuleLoadingPolicyKey(ModuleLoadingPolicy::kONLY_INTEGRATED))
          .toString();

  const auto parsed = ParseModuleLoadingPolicy(stored);
  if (!parsed.recognised) {
    // Said out loud rather than quietly repaired: the stored value is not one
    // this build knows, and behaving as though the user had chosen the
    // fallback would make the setting mean something other than it says.
    LOG_W() << "module loading policy" << stored
            << "is not one this version understands; falling back to"
            << ModuleLoadingPolicyKey(parsed.policy);
  }
  const auto policy = parsed.policy;

  if (policy == ModuleLoadingPolicy::kDISABLE) {
    LOG_I() << "module loading is disabled by user settings, abort...";
    ModuleManager::GetInstance().SetNeedRegisterModulesNum(0);
    // Credited rather than skipped: the startup progress bar blends a fixed
    // set of weights, so a track that never reports leaves it stuck short of
    // 100 for the whole of every start with modules turned off.
    CoreInitProgress::GetInstance().MarkStageDone(
        CoreInitStage::kMODULES, CoreInitStep::kSCANNING_MODULES);
    return;
  }

  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
      ->PostTask(new Thread::Task(
          [policy](const DataObjectPtr&) -> int {
            ModuleLoadStats::GetInstance().Begin();
            auto& progress = CoreInitProgress::GetInstance();
            progress.Report(CoreInitStage::kMODULES, 0.0,
                            CoreInitStep::kSCANNING_MODULES);

            QMap<QString, bool> modules = LoadIntegratedMods();

            // if user want to load all modules, then check external modules
            if (policy == ModuleLoadingPolicy::kALL) {
              LOG_I() << "loading external modules as well since user settings "
                         "is set to load all modules";
              modules.insert(LoadExternalMods());
            }

            auto& manager = ModuleManager::GetInstance();

            // PHASE ONE, concurrent: verify and install. This maps no image
            // and runs no module code, so several can run at once -- and it
            // is essentially the whole cost of loading.
            progress.Report(CoreInitStage::kMODULES, 0.1,
                            CoreInitStep::kVERIFYING_MODULES);
            auto prepared = PrepareModulesConcurrently(manager, modules);

            // Every prepared candidate is a verified descriptor, so
            // there is nothing left to reconcile: a loose library is no
            // longer a candidate, it is a referent.
            auto to_load = prepared;

            // Counted after superseding, so the number the manager waits for
            // is the number that will actually be attempted.
            manager.SetNeedRegisterModulesNum(static_cast<int>(to_load.size()));

            // PHASE TWO, sequential: map each library and register it.
            // QLibrary::load() runs the module's own static initialisers, and
            // the host cannot establish that one module's are safe against
            // another's -- so this half stays one at a time, on purpose.
            // Phase one is the hashing and so most of the cost, which is why
            // it is worth the larger share of this track. The rest is spent
            // naming modules as they register -- the part of a start a user
            // can actually recognise.
            const auto to_load_count = static_cast<double>(to_load.size());
            auto loaded = 0;
            for (const auto& candidate : to_load) {
              // library_name is a package's manifest name and is empty for a
              // loose library, which still has a filename worth showing.
              progress.Report(CoreInitStage::kMODULES,
                              0.7 + 0.3 * (loaded / to_load_count),
                              CoreInitStep::kLOADING_MODULE,
                              candidate.library_name.isEmpty()
                                  ? QFileInfo(candidate.source_path).fileName()
                                  : candidate.library_name);
              manager.LoadPreparedModule(candidate);
              ++loaded;
            }

            // Freeze the figures before anything else in the process can
            // add to them, so what startup cost stays answerable afterwards.
            ModuleLoadStats::GetInstance().Finish();
            progress.MarkStageDone(CoreInitStage::kMODULES,
                                   CoreInitStep::kLOADING_MODULE);

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
    swept += ModuleSdkSweepHandles(module_id.toUtf8().constData());
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
  //    Taking the modules out of the registries is part of unloading, so it
  //    happens only when unloading does. It is not free: this runs on the
  //    calling thread while step 2's deactivations were POSTED to the module
  //    runner, so clearing the register table here races a queued deactivation
  //    still reading it -- observed once in sixteen runs under ASan as a
  //    double free of a ModuleRegisterInfo, on the module runner, inside
  //    GlobalModuleContext::DeactivateModule.
  //
  //    Doing it unconditionally bought nothing while unloading is off, so it
  //    no longer happens unconditionally. Turning unloading back on means
  //    fixing the ordering first: either the deactivations must complete
  //    before the table is touched, or the table must be taken on the module
  //    runner rather than from here.
  auto unloaded = 0;
  if (kUnloadLibrariesAtShutdown) {
    for (const auto& module : manager.TakeAllModules()) {
      if (module != nullptr && module->UnloadLibrary()) ++unloaded;
    }
  }

  LOG_D() << "module system shut down cleanly, modules:" << module_ids.size()
          << ", sdk handles reclaimed:" << swept
          << ", libraries unloaded:" << unloaded;
}

}  // namespace GpgFrontend::Module