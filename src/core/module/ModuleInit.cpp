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
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "core/function/CoreInitProgress.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleHostPolicy.h"
#include "core/module/ModuleLoadStats.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleSdkBridge.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

using GpgFrontend::Module::ModuleOrigin;

namespace {

auto SearchModuleFromPath(const QString& mods_path) -> QStringList {
  QStringList modules;

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
  // This is also why there is no "packaged only" policy to honor here: there
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
    modules.append(descriptor.absoluteFilePath());
  }

  return modules;
}

auto LoadIntegratedMods() -> QStringList {
  const auto module_path = GpgFrontend::GlobalSettingStation::GetInstance()
                               .GetIntegratedModulePath();
  LOG_I() << "loading integrated modules from path:" << module_path;

  if (!QDir(module_path).exists()) {
    LOG_W() << "integrated modules at path: " << module_path
            << " not found; aborting.";
    return {};
  }

  return SearchModuleFromPath(module_path);
}

auto LoadExternalMods() -> QStringList {
  auto mods_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetModulesDir();

  if (!QDir(mods_path).exists()) {
    LOG_W() << "external module directory at path " << mods_path
            << " not found; aborting.";
    return {};
  }

  return SearchModuleFromPath(mods_path);
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
                                const QStringList& discovered,
                                ModuleOrigin origin,
                                const QSet<QString>& integrated_ids)
    -> QList<GpgFrontend::Module::ModuleLoadCandidate> {
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
        prepared[i] =
            manager.PrepareModule(discovered.at(i), origin, integrated_ids);
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

namespace {

/// The startup scan: discovery and phase one. Its own thread, never the
/// module runner, so the runner is free to be stopped while packages are
/// still being verified; joined by ShutdownGpgFrontendModules(). Detached,
/// rather than std::terminate()d, if the process ends without that.
struct ScanThread {
  std::thread thread;
  ScanThread() = default;
  ScanThread(const ScanThread&) = delete;
  auto operator=(const ScanThread&) -> ScanThread& = delete;
  ~ScanThread() {
    if (thread.joinable()) thread.detach();
  }
};
ScanThread g_scan;  // NOLINT(cert-err58-cpp)

}  // namespace

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
    LOG_I() << "module loading is disabled by user settings; aborting.";
    ModuleManager::GetInstance().SetNeedRegisterModulesNum(0);
    // Credited rather than skipped: the startup progress bar blends a fixed
    // set of weights, so a track that never reports leaves it stuck short of
    // 100 for the whole of every start with modules turned off.
    CoreInitProgress::GetInstance().MarkStageDone(
        CoreInitStage::kMODULES, CoreInitStep::kSCANNING_MODULES);
    return;
  }

  if (g_scan.thread.joinable()) return;  // one scan per process

  g_scan.thread = std::thread([policy]() {
    ModuleLoadStats::GetInstance().Begin();
    auto& progress = CoreInitProgress::GetInstance();
    progress.Report(CoreInitStage::kMODULES, 0.0,
                    CoreInitStep::kSCANNING_MODULES);

    auto& manager = ModuleManager::GetInstance();

    // PHASE ONE, concurrent: verify. This maps no image and runs no module
    // code, so several can run at once -- and it is essentially the whole
    // cost of loading.
    //
    // Integrated first, and apart: an external package may not claim an id
    // an integrated module has, so the integrated ids have to be known
    // before any external descriptor is judged -- and the order modules load
    // in must not depend on how directories happen to sort.
    progress.Report(CoreInitStage::kMODULES, 0.1,
                    CoreInitStep::kVERIFYING_MODULES);
    auto prepared = PrepareModulesConcurrently(manager, LoadIntegratedMods(),
                                               ModuleOrigin::kINTEGRATED, {});

    if (policy == ModuleLoadingPolicy::kALL) {
      LOG_I() << "also loading external modules, because the settings "
                 "allow modules the user has added";
      QSet<QString> integrated_ids;
      for (const auto& c : prepared) {
        if (c.ok && c.manifest.has_value())
          integrated_ids.insert(c.manifest->id);
      }
      prepared.append(PrepareModulesConcurrently(manager, LoadExternalMods(),
                                                 ModuleOrigin::kEXTERNAL,
                                                 integrated_ids));
    }

    // The number the manager waits for is the number that will actually be
    // attempted.
    manager.SetNeedRegisterModulesNum(static_cast<int>(prepared.size()));

    // PHASE TWO, sequential: map each library and register it, one task per
    // module on the module runner. QLibrary::load() runs the module's own
    // static initializers, and the host cannot establish that one module's
    // are safe against another's -- the runner is one thread, so they stay
    // one at a time, and it is never blocked for longer than one load.
    const auto total = static_cast<double>(prepared.size());
    auto runner = Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);
    for (qsizetype i = 0; i < prepared.size(); ++i) {
      const auto candidate = prepared.at(i);
      runner->PostTask(new Thread::Task(
          [candidate, i, total](const DataObjectPtr&) -> int {
            // library_name is the package's manifest name; the descriptor
            // file name is shown when it has none.
            CoreInitProgress::GetInstance().Report(
                CoreInitStage::kMODULES, 0.7 + 0.3 * (i / total),
                CoreInitStep::kLOADING_MODULE,
                candidate.library_name.isEmpty()
                    ? QFileInfo(candidate.source_path).fileName()
                    : candidate.library_name);
            return ModuleManager::GetInstance().LoadPreparedModule(candidate)
                       ? 0
                       : -1;
          },
          QString("module/load/%1").arg(i)));
    }

    runner->PostTask(new Thread::Task(
        [](const DataObjectPtr&) -> int {
          // Freeze the figures before anything else in the process can add
          // to them, so what startup cost stays answerable afterwards.
          ModuleLoadStats::GetInstance().Finish();
          CoreInitProgress::GetInstance().MarkStageDone(
              CoreInitStage::kMODULES, CoreInitStep::kLOADING_MODULE);
          LOG_I() << "module loading finished:"
                  << ModuleLoadStats::GetInstance().Summary();
          return 0;
        },
        "module/load/finished"));
  });
}

/**
 * @brief Whether step 5 actually unmaps the libraries.
 *
 * Off, on evidence rather than on caution. With it on, roughly one run in four
 * died under ASan in `QArrayDataPointer<char16_t>::data()` -- a QString whose
 * storage is a QStringLiteral in a module's own read-only data. Such a string
 * costs nothing to copy and is shared by pointer, so any one of them that
 * outlives the module (an event id, a registry key, a translation) dangles the
 * moment the image is unmapped, and the crash lands far from the cause.
 *
 * Nothing is gained by unmapping at process exit: the process is ending, and
 * replacing a module without restarting is not something this host does.
 */
constexpr bool kUnloadLibrariesAtShutdown = false;

void ShutdownGpgFrontendModules() {
  // The ordering here is the contract, and every step exists because skipping
  // it turns a tidy shutdown into a use-after-free.
  auto& manager = ModuleManager::GetInstance();
  auto& gate = GlobalModuleDispatchGate();

  // 1. STOP NEW CALLS. From here the set of in-flight calls can only shrink,
  //    and a scan still verifying packages stops offering them.
  gate.Close();
  if (g_scan.thread.joinable()) g_scan.thread.join();

  // 2. DEACTIVATE, THEN UNREGISTER -- every module, in that order, as ONE
  //    task on the module runner. On the runner because that is the only
  //    thread module lifecycle code runs on; as one task because each module's
  //    unregister hook must run after its deactivate hook has returned, never
  //    beside it. Waited for with a bound: the event loop is gone, so there is
  //    no nested loop this wait could deadlock against, but a module stuck in
  //    a blocking call could still hold the runner forever.
  constexpr int kLifecycleTimeoutMs = 5000;
  auto done = std::make_shared<std::promise<QStringList>>();
  auto finished = done->get_future();
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
      ->PostTask(new Thread::Task(
          [done](const DataObjectPtr&) -> int {
            done->set_value(ModuleManager::GetInstance()
                                .DeactivateAndUnregisterAllForShutdown());
            return 0;
          },
          "module/shutdown"));
  if (finished.wait_for(std::chrono::milliseconds(kLifecycleTimeoutMs)) !=
      std::future_status::ready) {
    LOG_W() << "module lifecycle hooks did not finish within"
            << kLifecycleTimeoutMs
            << "ms. Skipping teardown rather than freeing resources module "
               "code may still be using.";
    return;
  }
  const auto module_ids = finished.get();

  // 3. QUIESCE. Anything still inside module code through another thread --
  //    a GUI-thread widget call, a command -- finishes before anything below
  //    frees what it may be using. A timeout is REPORTED, and the remaining
  //    steps are skipped.
  constexpr int kQuiesceTimeoutMs = 5000;
  if (!gate.WaitQuiescent(kQuiesceTimeoutMs)) {
    LOG_W() << "module system did not go quiet within" << kQuiesceTimeoutMs
            << "ms;" << gate.InFlight()
            << "call(s) still inside module code. Skipping teardown rather "
               "than freeing resources they may still be using.";
    return;
  }

  // 4. REVOKE, THEN SWEEP OUTSTANDING SDK HANDLES. The gates only see calls
  //    the host made INTO a module; a thread the module started itself can
  //    still be calling the host. Revoking the grant refuses its next call,
  //    and waiting for the SDK to go idle lets the one already past the gate
  //    finish. Only then is the ledger authoritative: anything a module still
  //    holds is leaked rather than in use, and is wiped and freed.
  constexpr int kSdkIdleTimeoutMs = 2000;
  size_t swept = 0;
  for (const auto& module_id : module_ids) {
    if (auto module = manager.SearchModule(module_id); module != nullptr) {
      module->ReleaseGrant();
    }
    const auto id = module_id.toUtf8();
    if (!ModuleSdkWaitHostApiIdle(id.constData(), kSdkIdleTimeoutMs)) {
      LOG_W() << "module" << module_id << "is still inside an SDK call after"
              << kSdkIdleTimeoutMs << "ms; its handles are not reclaimed";
      continue;
    }
    swept += ModuleSdkSweepHandles(id.constData());
  }

  // 5. UNLOAD THE LIBRARIES -- gated off; see kUnloadLibrariesAtShutdown.
  //    Taking the modules out of the registry is part of unloading, so it
  //    happens only when unloading does. Every lifecycle task has finished by
  //    now (step 2 waited for them), so the registry is no longer read.
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