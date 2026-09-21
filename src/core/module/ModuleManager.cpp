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

#include "ModuleManager.h"

#include <atomic>
#include <optional>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/SettingsObject.h"
#include "core/module/GlobalModuleContext.h"
#include "core/module/GlobalRegisterTable.h"
#include "core/module/Module.h"
#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleExternalTrust.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleLoadStats.h"
#include "core/module/ModuleNamespace.h"
#include "core/struct/settings_object/ModuleSO.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/IOUtils.h"
#include "core/utils/MemoryUtils.h"

#if defined(Q_OS_WINDOWS)
#include <windows.h>
#endif

namespace GpgFrontend::Module {

namespace {

/**
 * @brief Make the module's own directory part of the loader search path for
 * the duration of a single module load.
 *
 * The resulting search order is: executable directory, module directory,
 * system directories, %PATH%. %PATH% is deliberately kept as the last resort
 * so an uninstalled build, which picks Qt and the compiler runtime up from
 * there, keeps working.
 *
 * SetDllDirectory() is process-global, which is safe here only because module
 * loading is serialized on the module task runner and the guard is held just
 * across QLibrary::load().
 */
class ScopedModuleLibrarySearchPath {
 public:
  explicit ScopedModuleLibrarySearchPath(const QString& module_library_path) {
#if defined(Q_OS_WINDOWS)
    const auto path = ResolveModuleLibrarySearchPath(module_library_path);
    if (path.isEmpty()) return;

    applied_ =
        SetDllDirectoryW(reinterpret_cast<const wchar_t*>(path.utf16())) != 0;
    if (!applied_) {
      LOG_W() << "cannot add module directory to the library search path: "
              << path;
    }
#else
    Q_UNUSED(module_library_path)
#endif
  }

  ~ScopedModuleLibrarySearchPath() {
#if defined(Q_OS_WINDOWS)
    // an empty string, unlike nullptr, restores the default search order
    // without putting the current working directory back into it
    if (applied_) SetDllDirectoryW(L"");
#endif
  }

  ScopedModuleLibrarySearchPath(const ScopedModuleLibrarySearchPath&) = delete;
  auto operator=(const ScopedModuleLibrarySearchPath&)
      -> ScopedModuleLibrarySearchPath& = delete;
  ScopedModuleLibrarySearchPath(ScopedModuleLibrarySearchPath&&) = delete;
  auto operator=(ScopedModuleLibrarySearchPath&&)
      -> ScopedModuleLibrarySearchPath& = delete;

 private:
  bool applied_ = false;
};

}  // namespace

auto IsModuleLibraryFileName(const QString& file_name) -> bool {
  static const QRegularExpression kModuleFileNameRegex(
      QStringLiteral("^libgf_mod_.+$"));
  return kModuleFileNameRegex.match(file_name).hasMatch();
}

auto IsModuleDescriptorFileName(const QString& file_name) -> bool {
  return file_name.endsWith(kModulePackageSuffix, Qt::CaseInsensitive);
}

auto InspectModuleLibrary(const QString& module_library_path,
                          const QString& known_hash)
    -> ModuleLibraryInspection {
  if (module_library_path.isEmpty()) return {false, "empty module path", {}};

  const QFileInfo info(module_library_path);
  if (!info.exists() || !info.isFile()) {
    return {false, "not an existing regular file", {}};
  }
  if (!info.isReadable()) return {false, "file is not readable", {}};
  if (info.size() <= 0) return {false, "file is empty", {}};

  if (!IsModuleLibraryFileName(info.fileName())) {
    return {false, "file name is not a module library name", {}};
  }

  // one single open: the header check and the hash both come from this handle,
  // so the hash describes the bytes that were actually inspected
  QFile file(info.filePath());
  if (!file.open(QIODevice::ReadOnly)) {
    return {false, QString("cannot open file: %1").arg(file.errorString()), {}};
  }

  if (!HasNativeImageHeader(file.read(8))) {
    return {false, "file is not a native shared library image", {}};
  }

  // A packaged module already has this value, from the signed manifest, which
  // ResolveAndVerifyNativeEntry() checked against those very bytes a moment
  // ago. Recomputing it here read the whole library a second time to arrive at
  // an answer that was already known.
  //
  // Nothing is given up by trusting it here: this value is not a security
  // check. It records what was last seen so stale module settings can be
  // reset, and the security check is the tree verification that produced it.
  if (!known_hash.isEmpty()) return {true, {}, known_hash};

  auto hash = CalculateBinaryChacksum(file);
  if (hash.isEmpty()) return {false, "cannot calculate module checksum", {}};

  return {true, {}, hash};
}

auto ResolveModuleLibrarySearchPath(const QString& module_library_path)
    -> QString {
  if (module_library_path.isEmpty()) return {};

  const QFileInfo info(module_library_path);
  const auto dir = info.absoluteDir();
  if (!dir.exists()) return {};

  return QDir::toNativeSeparators(dir.absolutePath());
}

class ModuleManager::Impl {
 public:
  Impl()
      : gmc_(GpgFrontend::SecureCreateUniqueObject<GlobalModuleContext>()),
        grt_(GpgFrontend::SecureCreateUniqueObject<GlobalRegisterTable>()) {}

  ~Impl() = default;

  /**
   * @brief Verify a descriptor and locate the native library it binds.
   *
   * Two questions, asked by two layers, and kept apart on purpose.
   *
   * VerifyModuleDescriptor() answers the first entirely in memory: is this a
   * descriptor signed for this Host, and do its metadata and resources agree
   * with what it carries. It never touches a filesystem and has no idea that
   * native libraries exist.
   *
   * ResolveAndVerifyNativeEntry() answers the second: which file does its
   * logical entry name mean here, is that file inside the directory this Host
   * chose, and is it the one the descriptor binds. The descriptor never says
   * where its native lives -- a logical name has no room for a path -- so this
   * is the only place the mapping happens, which is what keeps a package from
   * influencing where a loader looks.
   *
   * Nothing is installed, nothing is cached and nothing is materialised. The
   * library is loaded from where it already is, which is what lets $ORIGIN,
   * @loader_path, debuggers and platform code signing all work normally on it.
   *
   * @param package_path the `*.gfmodule` the scan found
   * @param[out] library_path the verified path to hand the loader, on success
   * @param[out] manifest what the descriptor says about itself, on success
   * @param[out] module_hash the binding value the descriptor recorded
   * @param[out] library_name the entry's filename on this platform
   * @return false when the descriptor or its entry was refused
   */
  auto VerifyAndResolveEntry(ModuleOrigin origin, const QString& package_path,
                             QString& library_path, ModuleManifest& manifest,
                             QString& module_hash, QString& library_name)
      -> bool {
    // Two verifiers, because the two boundaries are genuinely different and
    // one function full of `if (external)` would make it possible to add a
    // check to the wrong side without noticing. Integrated descriptors are
    // checked against the key compiled into this Host and must carry none of
    // their own; external ones carry the key they are checked against, which
    // proves internal consistency and nothing more.
    const auto read = origin == ModuleOrigin::kINTEGRATED
                          ? VerifyModuleDescriptor(package_path)
                          : VerifyExternalModuleDescriptor(package_path);
    if (!read.ok) {
      LOG_W() << "module manager refuses module descriptor: " << package_path
              << ", reason: " << read.reason << " ("
              << ModuleDescriptorStatusToString(read.status) << ")";
      RecordRefusal({package_path, origin, {}, read.reason, {}, false});
      return false;
    }

    // The user's two decisions, asked BEFORE anything opens the native.
    //
    // Discovery must never imply execution: an external module the user has
    // not approved costs one descriptor read and stops here, with its native
    // never opened and its bytes never hashed. Both decisions are required,
    // and the key is asked about first so a module whose key was never
    // accepted says so rather than reporting "not enabled" and sending the
    // reader to the wrong control.
    if (origin == ModuleOrigin::kEXTERNAL) {
#if defined(Q_OS_MACOS)
      // Not "unimplemented": the configuration this application ships under
      // forbids it. Library Validation is on, so the process cannot load a
      // dylib that does not carry this application's Team ID -- and a third
      // party's module by definition does not. Admitting one here would mean
      // verifying it, trusting it, enabling it, and then watching dyld refuse
      // it anyway, with the user having made two decisions for nothing.
      //
      // Fail closed and say so. When macOS external modules become possible
      // they will need a native-binding design of their own, reviewed on its
      // own evidence; apple-binding-id is not it and is not coming back.
      const QString why =
          QObject::tr("External modules are not supported on macOS: the "
                      "system only loads code signed with this "
                      "application's own Team ID.");
      LOG_W() << "module manager refuses external module: " << package_path
              << ", reason: " << why;
      RecordRefusal({package_path, origin, read.manifest.id, why,
                     read.build_public_key, false});
      return false;
#else
      const auto authorization = ExternalModuleAuthorization(
          read.manifest.id, read.build_public_key);
      if (authorization != ModuleAuthorizationState::kTRUSTED_AND_ENABLED) {
        const QString why =
            authorization == ModuleAuthorizationState::kKEY_UNTRUSTED
                ? QObject::tr("Waiting for you to trust the build key that "
                              "signed it.")
                : QObject::tr("Waiting for you to enable it.");
        LOG_I() << "module manager holds external module: " << package_path
                << ", reason: "
                << ModuleAuthorizationStateToString(authorization);
        // pending_user_action: this one has an action attached, and telling
        // it apart from a broken module is the difference between a control
        // to press and a problem to report.
        RecordRefusal({package_path, origin, read.manifest.id, why,
                       read.build_public_key, true});
        return false;
      }
#endif
    }

    // The namespace this descriptor was found in, and whether it is the one
    // its own signed identity says it should be.
    //
    // A locator check, not the integrity check -- that is the entry binding
    // below. What this catches is a descriptor dropped into another module's
    // namespace, where the relative native/ lookup would otherwise go looking
    // in a directory that belongs to something else. Recomputed from the id
    // rather than read from anywhere, so there is no second statement about
    // where a module lives for the first to disagree with.
    const QDir namespace_dir(QFileInfo(package_path).absolutePath());
    const auto expected_key = ModuleDirectoryKey(read.manifest.id);
    if (namespace_dir.dirName() != expected_key) {
      const auto why =
          QObject::tr("It declares %1, but sits in a directory named %2 "
                      "rather than %3.")
              .arg(read.manifest.id, namespace_dir.dirName(), expected_key);
      LOG_W() << "module manager refuses module descriptor: " << package_path
              << ", reason: " << why;
      RecordRefusal({package_path, origin, read.manifest.id, why,
                     read.build_public_key, false});
      return false;
    }

    // Where the natives live is ModuleNamespace's to answer, not this
    // function's: on macOS the descriptor and the code are in two different
    // trees, because Apple wants executable code under Frameworks.
    const ModuleNativeRoot root{ModuleNativeRootFor(package_path)};

    // The one place origin meets policy. Origin arrived from the directory
    // that was scanned; the requirement is what this build was compiled with.
    // Nothing between here and the manifest can influence either.
    const ModuleEntryTrustPolicy policy{origin,
                                        HostIntegratedBindingRequirement()};

    const auto entry =
        ResolveAndVerifyNativeEntry(read.manifest, root, policy);
    if (!entry.ok) {
      LOG_W() << "module manager refuses module entry: " << package_path
              << ", reason: " << entry.reason << " ("
              << ModuleEntryStatusToString(entry.status) << ")";
      RecordRefusal({package_path, origin, read.manifest.id, entry.reason,
                     read.build_public_key, false});
      return false;
    }

    // It loaded this time, so whatever was said about it last time is no
    // longer true. Without this, enabling a module would leave it listed as
    // pending forever.
    ForgetRefusal(package_path);

    library_path = entry.path;
    manifest = read.manifest;
    // Empty when this descriptor binds nothing, which is a legitimate state
    // under a relaxed integrated policy. It is a display value and a settings
    // key, never a check: what decided whether to load is above.
    module_hash = read.manifest.entry_native.verification.has_value()
                      ? read.manifest.entry_native.verification->value
                      : QString();
    library_name = QFileInfo(entry.path).fileName();
    return true;
  }

  /**
   * @brief Phase one: verify and install. Maps nothing, runs no module code.
   *
   * Takes an admission ticket like any other work inside the module system, so
   * a preparation already running when teardown begins is waited for by the
   * quiesce step, and one starting after it declines.
   *
   * This is where the seconds are: verifying and unpacking a package is
   * expensive, and it used to sit on the module runner where
   * TaskRunner::Stop()'s three-second wait could time out -- and destroying a
   * QThread that is still running is fatal. Off that thread, the cost stops
   * being a shutdown hazard as well as stopping being serial.
   */
  auto PrepareModule(const QString& path, ModuleOrigin origin)
      -> ModuleLoadCandidate {
    ModuleLoadCandidate candidate;
    candidate.source_path = path;
    candidate.origin = origin;
    candidate.packaged = IsModuleDescriptorFileName(QFileInfo(path).fileName());
    candidate.library_path = path;

    ModuleDispatchScope admission(GlobalModuleDispatchGate());
    if (!admission.Entered()) {
      LOG_D() << "module manager declines to prepare during shutdown: " << path;
      return candidate;
    }

    if (candidate.packaged) {
      QString library_path;
      ModuleManifest verified;
      QString module_hash;
      QString library_name;
      if (!VerifyAndResolveEntry(origin, path, library_path, verified, module_hash,
                                 library_name)) {
        return candidate;
      }
      candidate.library_path = library_path;
      candidate.manifest = verified;
      candidate.module_hash = module_hash;
      candidate.library_name = library_name;
    }

    candidate.ok = true;
    return candidate;
  }

  auto LoadPreparedModule(const ModuleLoadCandidate& candidate) -> bool {
    // Every refusal below owes the same three things: say why, stop waiting
    // for this module, and count it. Written out seven times, one of them
    // eventually forgets one -- and a forgotten decrement is a startup that
    // never finishes waiting for registration.
    //
    // Deliberately not an RAII guard: the SUCCESS path must not decrement, so
    // a scope-exit version would have to be disarmed, which is the same
    // discipline problem wearing a different hat.
    const auto refuse = [this](const QString& why) {
      LOG_W() << "module manager refuses module: " << why;
      DropFromExpectedRegistrations();
      ModuleLoadStats::GetInstance().AddRefusedModule();
      return false;
    };

    if (!candidate.ok) return refuse(candidate.source_path);

    // Phase two is serial because QLibrary::load() below runs third-party
    // static initialisers. This records that it stayed serial, so a later
    // refactor that parallelises the loop fails a test instead of passing
    // quietly -- see ModuleLoadStats::NativeLoadScope.
    ModuleLoadStats::NativeLoadScope native_load;

    const auto& module_library_path = candidate.source_path;
    const auto& library_path = candidate.library_path;
    const auto& manifest = candidate.manifest;

    // Mapping an image is work inside the module system too, so it takes its
    // own ticket -- and shutdown may have begun while phase one was reading a
    // package, in which case loading now would put module code into a process
    // that has already stopped waiting for it.
    ModuleDispatchScope admission(GlobalModuleDispatchGate());
    if (!admission.Entered()) {
      // Not refuse(): shutdown overtaking a load is routine, not a fault of
      // the module, so it is not worth a warning in the user's log.
      LOG_D() << "module manager abandons a load that shutdown overtook: "
              << module_library_path;
      DropFromExpectedRegistrations();
      ModuleLoadStats::GetInstance().AddRefusedModule();
      return false;
    }

    // A packaged module was inspected in phase one, against its bytes rather
    // than against a path -- which is the only place that question can be
    // answered for an image that never becomes a file. A loose library has no
    // manifest to inspect, so it is inspected here, where it still can be:
    // QLibrary::load() below runs the module's own initializers, so everything
    // decidable without mapping has to be decided before this point.
    QString module_hash = candidate.module_hash;
    if (!candidate.packaged) {
      ModuleLoadStats::GetInstance().AddHashedBytes(
          QFileInfo(library_path).size());

      const auto inspection = InspectModuleLibrary(library_path);
      if (!inspection.ok) {
        return refuse(
            QString("%1, reason: %2").arg(library_path, inspection.reason));
      }
      module_hash = inspection.hash;
    }

    auto module_library = std::make_unique<QLibrary>(library_path);

    ScopedModuleLibrarySearchPath search_path(library_path);
    if (!module_library->load()) {
      return refuse(
          QString("%1, reason: %2")
              .arg(module_library->fileName(), module_library->errorString()));
    }

    // Ownership moves into the Module, which is what gives teardown something
    // to unload. It used to be a local here, so a successfully loaded module
    // stayed mapped for the whole run with nothing holding a handle on it.
    auto module = SecureCreateSharedObject<Module>(std::move(module_library),
                                                   module_hash);
    if (!module->IsGood()) {
      // Drop the symbol pointers before the image goes away. The Module owns
      // the library now, so destroying it is what unloads: a rejected module
      // does not stay mapped for the whole run.
      module->UnloadLibrary();
      module.reset();
      return refuse(
          QString("%1, reason: it is not a usable module").arg(library_path));
    }

    if (manifest) {
      // Runtime identity against signed identity. Without this the signature
      // covers a name nothing enforces: a package could say it is one module
      // and carry another, and everything downstream -- settings, activation,
      // the module list -- would key off the binary's word for it.
      if (module->GetModuleIdentifier() != manifest->id) {
        const auto said = module->GetModuleIdentifier();
        module->UnloadLibrary();
        module.reset();
        return refuse(
            QString("%1, reason: its manifest says %2 and the module inside "
                    "says %3")
                .arg(module_library_path, manifest->id, said));
      }

      // The same argument applies to the version, which until now was signed
      // and then not looked at. A package saying 1.3.2 while the binary says
      // something else is a package whose signature covers a claim nothing
      // checks -- and the version is what an update decision is made on.
      if (module->GetModuleVersion() != manifest->version) {
        const auto said = module->GetModuleVersion();
        module->UnloadLibrary();
        module.reset();
        return refuse(
            QString("%1, reason: its manifest says version %2 and the module "
                    "inside says %3")
                .arg(module_library_path, manifest->version, said));
      }

      // Metadata now comes from the manifest, which is readable without
      // executing anything.
      module->SetModuleMetaData(manifest->metadata);

      // What a user can act on is the package, not the descriptor or temporary
      // file the image happened to arrive through.
      module->SetSourcePackagePath(module_library_path);

      // Kept so the UI can separate what the host verified from what the
      // module says about itself.
      module->SetModuleManifest(*manifest);
    }

    // The module takes the image with it. Where an open image can be unlinked
    // this is also when that happens, so the file stops existing the moment it
    // has been mapped; where it cannot -- Windows -- the module outliving it is
    // exactly what keeps the mapping valid.

    module->SetGPC(gmc_.get());
    ModuleLoadStats::GetInstance().AddLoadedModule();

    LOG_D() << "module loaded, awaiting registration: "
            << QFileInfo(module_library_path).fileName();

    auto runner = Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);

    runner->PostTask(new Thread::Task(
        [=](const GpgFrontend::DataObjectPtr&) -> int {
          // register module
          if (!gmc_->RegisterModule(
                  module, candidate.origin == ModuleOrigin::kINTEGRATED)) {
            return -1;
          }

          return 0;
        },
        __func__, nullptr));

    runner->PostTask(new Thread::Task(
        [=](const GpgFrontend::DataObjectPtr&) -> int {
          const auto module_id = module->GetModuleIdentifier();
          const auto module_hash = module->GetModuleHash();

          SettingsObject so(QString("module.%1.so").arg(module_id));
          ModuleSO module_so(so);

          // A changed hash resets this module's stored settings rather
          // than refusing the module. That is right while the hash is only a
          // record of what was last seen: a developer rebuilding a module
          // changes it every time, and a rebuilt module is not an attack.
          //
          // It stops being right once an installed package's digest is
          // authoritative -- once there is a store that says which version is
          // installed, a binary that changed underneath it is a refusal and
          // not a settings migration. That store does not exist yet, so
          // neither does the refusal.
          //
          // reset module settings if necessary
          if (module_so.module_id != module_id ||
              module_so.module_hash != module_hash) {
            module_so.module_id = module_id;
            module_so.module_hash = module_hash;
            // auto active integrated module by default
            module_so.auto_activate =
                candidate.origin == ModuleOrigin::kINTEGRATED;
            module_so.set_by_user = false;

            so.Store(module_so.ToJson());
          }

          // if this module need auto active
          if (module_so.auto_activate) {
            if (!gmc_->ActiveModule(module_id)) {
              return -1;
            }
          }

          return 0;
        },
        __func__, nullptr));

    return true;
  }

  /// Set once, by whichever thread gets there first.
  ///
  /// The read and the write used to be two statements over a plain int, and
  /// the three accessors below run on three different threads: this one on the
  /// module runner, DropFromExpectedRegistrations() on the load loop, and
  /// IsAllModulesRegistered() on whatever thread asks -- including the one
  /// behind `--module-status`, which every platform's smoke test uses.
  void SetNeedRegisterModulesNum(int n) {
    if (n < 0) return;
    auto unset = -1;
    need_register_modules_.compare_exchange_strong(unset, n);
  }

  auto SearchModule(const ModuleIdentifier& module_id) -> ModulePtr {
    return gmc_->SearchModule(module_id);
  }

  auto ListAllRegisteredModuleID() -> QStringList {
    return gmc_->ListAllRegisteredModuleID();
  }

  auto TakeAllModules() -> QList<ModulePtr> { return gmc_->TakeAllModules(); }

  void RegisterModule(const ModulePtr& module) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
        ->PostTask(new Thread::Task(
            [=](const GpgFrontend::DataObjectPtr&) -> int {
              module->SetGPC(gmc_.get());
              return gmc_->RegisterModule(module, false) ? 0 : -1;
            },
            __func__, nullptr));
  }

  void ListenEvent(const ModuleIdentifier& module_id,
                   const EventIdentifier& event_id) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
        ->PostTask(new Thread::Task(
            [=](const GpgFrontend::DataObjectPtr&) -> int {
              gmc_->ListenEvent(module_id, event_id);
              return 0;
            },
            __func__, nullptr));
  }

  void TriggerEvent(const EventReference& event) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
        ->PostTask(new Thread::Task(
            [=](const GpgFrontend::DataObjectPtr&) -> int {
              gmc_->TriggerEvent(event);
              return 0;
            },
            __func__, nullptr));
  }

  auto SearchEvent(const EventTriggerIdentifier& trigger_id)
      -> std::optional<EventReference> {
    return gmc_->SearchEvent(trigger_id);
  }

  auto GetModuleListening(const ModuleIdentifier& module_id) -> QStringList {
    return gmc_->GetModuleListening(module_id);
  }

  void ActiveModule(const ModuleIdentifier& identifier) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
        ->PostTask(new Thread::Task(
            [=](const GpgFrontend::DataObjectPtr&) -> int {
              gmc_->ActiveModule(identifier);
              return 0;
            },
            __func__, nullptr));
  }

  void DeactivateModule(const ModuleIdentifier& identifier) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
        ->PostTask(new Thread::Task(
            [=](const GpgFrontend::DataObjectPtr&) -> int {
              gmc_->DeactivateModule(identifier);
              return 0;
            },
            __func__, nullptr));
  }

  auto GetTaskRunner(const ModuleIdentifier& module_id)
      -> std::optional<TaskRunnerPtr> {
    return gmc_->GetTaskRunner(module_id);
  }

  auto UpsertRTValue(Namespace n, Key k, std::any v) -> bool {
    return grt_->PublishKV(std::move(n), std::move(k), std::move(v));
  }

  auto RetrieveRTValue(Namespace n, Key k) -> std::optional<std::any> {
    return grt_->LookupKV(std::move(n), std::move(k));
  }

  auto ListenPublish(QObject* o, Namespace n, Key k, LPCallback c) -> bool {
    return grt_->ListenPublish(o, std::move(n), std::move(k), std::move(c));
  }

  auto ListRTChildKeys(const QString& n, const QString& k) -> QContainer<Key> {
    return grt_->ListChildKeys(n, k);
  }

  auto IsModuleActivated(const ModuleIdentifier& id) -> bool {
    return gmc_->IsModuleActivated(id);
  }

  auto IsIntegratedModule(const ModuleIdentifier& id) -> bool {
    return gmc_->IsIntegratedModule(id);
  }

  /// One fewer module the startup scan is still waiting for.
  ///
  /// Only while startup is still waiting. IsAllModulesRegistered() is the
  /// equality of this count with the number actually registered, so a module
  /// refused AFTER startup finished -- loaded ad hoc, or by a test -- would
  /// otherwise push the target below the count permanently, and the
  /// "modules are ready" signal would never be true again.
  void DropFromExpectedRegistrations() {
    // A compare-exchange loop rather than a test followed by a decrement:
    // between those two statements the registered count can rise, and the
    // decrement would then take the target below it -- which is exactly the
    // permanent "never ready" state the comment above warns about.
    auto current = need_register_modules_.load(std::memory_order_relaxed);
    while (current > gmc_->GetRegisteredModuleNum()) {
      if (need_register_modules_.compare_exchange_weak(
              current, current - 1, std::memory_order_relaxed)) {
        return;
      }
    }
  }

  auto IsAllModulesRegistered() {
    // Read once. Logging one value and comparing another is how a report says
    // "need 4, registered 4" and still answers false.
    const auto needed = need_register_modules_.load(std::memory_order_relaxed);
    if (needed == -1) return false;
    const auto registered = gmc_->GetRegisteredModuleNum();
    LOG_D() << "module manager report: needing registration" << needed
            << ", registered" << registered;
    return needed == registered;
  }

  auto GRT() -> GlobalRegisterTable* { return grt_.get(); }

  auto IsEventListening(const EventTriggerIdentifier& trigger_id) -> bool {
    return gmc_->IsEventListening(trigger_id);
  }

  void RecordRefusal(ModuleRefusalRecord record) {
    const QMutexLocker lock(&refusals_mutex_);
    // Keyed by descriptor path: a rescan should update what it says about a
    // module rather than list it twice, and a module the user has since
    // enabled must stop being reported as pending.
    for (auto& existing : refusals_) {
      if (existing.descriptor_path == record.descriptor_path) {
        existing = std::move(record);
        return;
      }
    }
    refusals_.append(std::move(record));
  }

  void ForgetRefusal(const QString& descriptor_path) {
    const QMutexLocker lock(&refusals_mutex_);
    refusals_.removeIf([&](const ModuleRefusalRecord& r) {
      return r.descriptor_path == descriptor_path;
    });
  }

  auto ListRefusals() -> QList<ModuleRefusalRecord> {
    const QMutexLocker lock(&refusals_mutex_);
    return refusals_;
  }

 private:
  SecureUniquePtr<GlobalModuleContext> gmc_;
  SecureUniquePtr<GlobalRegisterTable> grt_;
  std::atomic<int> need_register_modules_ = -1;

  /// Guards refusals_ alone. Phase one prepares modules concurrently, so
  /// every record below is written from a worker thread and read from the UI.
  QMutex refusals_mutex_;
  QList<ModuleRefusalRecord> refusals_;
};

auto GF_CORE_EXPORT IsModuleExists(ModuleIdentifier id) -> bool {
  auto module = ModuleManager::GetInstance().SearchModule(std::move(id));
  return module != nullptr && module->IsGood();
}

auto UpsertRTValue(const QString& namespace_, const QString& key,
                   const std::any& value) -> bool {
  return ModuleManager::GetInstance().UpsertRTValue(namespace_, key,
                                                    std::any(value));
}

auto ListRTChildKeys(const QString& namespace_, const QString& key)
    -> QContainer<Key> {
  return ModuleManager::GetInstance().ListRTChildKeys(namespace_, key);
}

ModuleManager::ModuleManager(int channel)
    : SingletonFunctionObject<ModuleManager>(channel),
      p_(SecureCreateUniqueObject<Impl>()) {}

ModuleManager::~ModuleManager() = default;

auto ModuleManager::ListModuleRefusals() -> QList<ModuleRefusalRecord> {
  return p_->ListRefusals();
}

auto ModuleManager::PrepareModule(const QString& path, ModuleOrigin origin)
    -> ModuleLoadCandidate {
  return p_->PrepareModule(path, origin);
}

auto ModuleManager::LoadPreparedModule(const ModuleLoadCandidate& candidate)
    -> bool {
  return p_->LoadPreparedModule(candidate);
}

auto ModuleManager::SearchModule(ModuleIdentifier id) -> ModulePtr {
  return p_->SearchModule(id);
}

void ModuleManager::RegisterModule(ModulePtr module) {
  p_->RegisterModule(module);
}

void ModuleManager::ListenEvent(ModuleIdentifier module,
                                EventIdentifier event) {
  p_->ListenEvent(module, event);
}

auto ModuleManager::GetModuleListening(ModuleIdentifier id) -> QStringList {
  return p_->GetModuleListening(id);
}

void ModuleManager::TriggerEvent(EventReference event) {
  p_->TriggerEvent(event);
}

auto ModuleManager::SearchEvent(EventTriggerIdentifier id)
    -> std::optional<EventReference> {
  return p_->SearchEvent(id);
}

void ModuleManager::ActiveModule(ModuleIdentifier id) { p_->ActiveModule(id); }

void ModuleManager::DeactivateModule(ModuleIdentifier id) {
  p_->DeactivateModule(id);
}

auto ModuleManager::GetTaskRunner(ModuleIdentifier id)
    -> std::optional<TaskRunnerPtr> {
  return p_->GetTaskRunner(id);
}

auto ModuleManager::UpsertRTValue(Namespace n, Key k, std::any v) -> bool {
  return p_->UpsertRTValue(std::move(n), std::move(k), std::move(v));
}

auto ModuleManager::RetrieveRTValue(Namespace n, Key k)
    -> std::optional<std::any> {
  return p_->RetrieveRTValue(n, k);
}

auto ModuleManager::ListenRTPublish(QObject* o, Namespace n, Key k,
                                    LPCallback c) -> bool {
  return p_->ListenPublish(o, std::move(n), std::move(k), std::move(c));
}

auto ModuleManager::ListRTChildKeys(const QString& n, const QString& k)
    -> QContainer<Key> {
  return p_->ListRTChildKeys(n, k);
}

auto ModuleManager::IsModuleActivated(ModuleIdentifier id) -> bool {
  return p_->IsModuleActivated(id);
}

auto ModuleManager::IsIntegratedModule(ModuleIdentifier id) -> bool {
  return p_->IsIntegratedModule(id);
}

auto ModuleManager::GetModuleProvenance(ModuleIdentifier id)
    -> ModuleProvenance {
  ModuleProvenance p;

  auto module = p_->SearchModule(id);
  if (module == nullptr) return p;

  p.identifier = module->GetModuleIdentifier();
  p.version = module->GetModuleVersion();
  p.integrated = p_->IsIntegratedModule(id);
  p.activated = p_->IsModuleActivated(id);

  // Asked once, of the module, and recorded. Two widgets used to answer this
  // separately -- one via IsPackaged(), one by testing whether a manifest was
  // present -- which are the same answer only for as long as nothing changes.
  p.packaged = module->IsPackaged();

  // The descriptor it was verified from, which is the file a person can point
  // at -- not the native inside its namespace.
  p.source_package_path = module->GetModulePath();
  p.manifest = module->GetModuleManifest();
  p.sdk_abi = module->GetModuleSDKABIVersion();
  p.hash = module->GetModuleHash();
  p.metadata = module->GetModuleMetaData();

  return p;
}

auto ModuleManager::ListAllRegisteredModuleID() -> QStringList {
  return p_->ListAllRegisteredModuleID();
}

auto ModuleManager::TakeAllModules() -> QList<ModulePtr> {
  return p_->TakeAllModules();
};

auto ModuleManager::GRT() -> GlobalRegisterTable* { return p_->GRT(); }

auto ModuleManager::IsAllModulesRegistered() -> bool {
  return p_->IsAllModulesRegistered();
}

void ModuleManager::SetNeedRegisterModulesNum(int n) {
  p_->SetNeedRegisterModulesNum(n);
}

auto ModuleManager::IsEventListening(const EventTriggerIdentifier& trigger_id)
    -> bool {
  return p_->IsEventListening(trigger_id);
}

auto IsEventListening(const EventTriggerIdentifier& trigger_id) -> bool {
  return ModuleManager::GetInstance().IsEventListening(trigger_id);
}
}  // namespace GpgFrontend::Module