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

#include <optional>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/SettingsObject.h"
#include "core/module/GlobalModuleContext.h"
#include "core/module/GlobalRegisterTable.h"
#include "core/module/Module.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleLoadStats.h"
#include "core/module/ModulePackageVerifier.h"
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

/**
 * @brief Whether the given file header looks like a shared library image the
 * platform loader could actually map.
 *
 * Cheap sanity filter only: it keeps text files, scripts and truncated
 * downloads away from the loader. It says nothing about who produced the file.
 */
auto HasNativeImageHeader(const QByteArray& header) -> bool {
#if defined(Q_OS_WINDOWS)
  return header.size() >= 2 && header.startsWith("MZ");
#elif defined(Q_OS_MACOS)
  if (header.size() < 4) return false;

  const auto magic = static_cast<quint32>(
      (static_cast<quint8>(header[0]) << 24) |
      (static_cast<quint8>(header[1]) << 16) |
      (static_cast<quint8>(header[2]) << 8) | static_cast<quint8>(header[3]));

  // thin mach-o in both endiannesses, plus a fat/universal archive
  return magic == 0xFEEDFACE || magic == 0xFEEDFACF || magic == 0xCEFAEDFE ||
         magic == 0xCFFAEDFE || magic == 0xCAFEBABE || magic == 0xBEBAFECA;
#else
  return header.size() >= 4 && header[0] == '\x7f' && header[1] == 'E' &&
         header[2] == 'L' && header[3] == 'F';
#endif
}

}  // namespace

auto IsModuleLibraryFileName(const QString& file_name) -> bool {
  static const QRegularExpression kModuleFileNameRegex(
      QStringLiteral("^libgf_mod_.+$"));
  return kModuleFileNameRegex.match(file_name).hasMatch();
}

auto IsModulePackageFileName(const QString& file_name) -> bool {
  return file_name.endsWith(kModulePackageSuffix, Qt::CaseInsensitive);
}

auto InspectModuleImage(const VerifiedModuleImage& image,
                        const QString& known_hash) -> ModuleLibraryInspection {
  if (!image.IsValid()) return {false, "there is no image to inspect", {}};

  // From the manifest, which is signed, rather than from a filename -- there
  // is no file yet, and on the platform where there never will be one the load
  // path is a number.
  if (!IsModuleLibraryFileName(image.LibraryName())) {
    return {false, "the packaged library is not named like a module", {}};
  }

  if (!HasNativeImageHeader(image.Bytes().left(8))) {
    return {false, "the packaged library is not a native image", {}};
  }

  if (known_hash.isEmpty()) {
    return {false, "the manifest carries no digest for the library", {}};
  }

  return {true, {}, known_hash};
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

  // A packaged module already has this digest, from the signed manifest, which
  // verification checked against those very bytes a moment ago -- see
  // InspectModuleImage(), which is the path a package takes. Recomputing it
  // here read the whole library a second time to arrive at an answer that was
  // already known.
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
   * @brief Verify a package and make its library loadable, installing nothing.
   *
   * The ordering the whole format exists for is inside
   * ReadVerifiedModuleImage(): every entry is diverted, so no byte of an
   * unverified package reaches a filesystem, and the library is handed back
   * only once the manifest, the signature and every file digest agree.
   *
   * Nothing is installed and nothing is cached. The verified bytes are
   * materialised only as far as the native loader requires -- on Linux not at
   * all, the image being an anonymous descriptor -- and that materialisation
   * lives exactly as long as the module does. A start therefore costs the same
   * every time, and there is no on-disk state for a later start, an upgrade or
   * a second profile to disagree with.
   *
   * @param package_path the `*.gfmodule` the scan found
   * @param[out] mapping the materialised image, on success
   * @param[out] library_path the path to hand the loader, on success
   * @param[out] manifest what the package says about itself, on success
   * @param[out] module_hash the signed digest of the library, on success
   * @param[out] library_name the library's name per the signed manifest
   * @return false when the package was refused; nothing was materialised
   */
  auto VerifyAndMaterializePackage(const QString& package_path,
                                   std::shared_ptr<ModuleImageMapping>& mapping,
                                   QString& library_path,
                                   ModuleManifest& manifest,
                                   QString& module_hash, QString& library_name)
      -> bool {
    const auto read = ReadVerifiedModuleImage(package_path);
    if (!read.ok) {
      LOG_W() << "module manager refuses module package: " << package_path
              << ", reason: " << read.reason << " ("
              << ModulePackageStatusToString(read.status) << ")";
      return false;
    }

    // Everything a pre-load inspection can ask about a packaged module is a
    // property of the package, not of wherever its bytes end up. Asking here,
    // of the bytes, is both earlier and more honest than asking later of a
    // path -- and a path is the one thing that cannot answer it, since a
    // descriptor in /proc/self/fd is named after a number.
    //
    // The digest comes from the verification result rather than a fresh walk
    // of manifest.files: the verifier already located the sole bin/ entry, and
    // finding it a second time here is a second chance to find it differently.
    const auto inspection = InspectModuleImage(read.image, read.library_sha256);
    if (!inspection.ok) {
      LOG_W() << "module manager refuses module package: " << package_path
              << ", reason: " << inspection.reason;
      return false;
    }

    QString reason;
    auto materialized = ModuleImageMapping::Create(read.image, &reason);
    if (!materialized) {
      // Fails closed: there is no path, so there is nothing for phase two to
      // load even if it were careless enough to try.
      LOG_W() << "module manager could not make module loadable: "
              << package_path << ", reason: " << reason;
      return false;
    }

    library_path = materialized->LoadPath();
    mapping = std::shared_ptr<ModuleImageMapping>(std::move(materialized));
    manifest = read.manifest;
    module_hash = inspection.hash;
    library_name = read.image.LibraryName();
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
  auto PrepareModule(const QString& path, bool integrated)
      -> ModuleLoadCandidate {
    ModuleLoadCandidate candidate;
    candidate.source_path = path;
    candidate.integrated = integrated;
    candidate.packaged = IsModulePackageFileName(QFileInfo(path).fileName());
    candidate.library_path = path;

    ModuleDispatchScope admission(GlobalModuleDispatchGate());
    if (!admission.Entered()) {
      LOG_D() << "module manager declines to prepare during shutdown: " << path;
      return candidate;
    }

    if (candidate.packaged) {
      std::shared_ptr<ModuleImageMapping> mapping;
      QString library_path;
      ModuleManifest verified;
      QString module_hash;
      QString library_name;
      if (!VerifyAndMaterializePackage(path, mapping, library_path, verified,
                                       module_hash, library_name)) {
        return candidate;
      }
      candidate.library_path = library_path;
      candidate.mapping = mapping;
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
    if (candidate.mapping) module->AdoptImageMapping(candidate.mapping);

    module->SetGPC(gmc_.get());
    ModuleLoadStats::GetInstance().AddLoadedModule();

    LOG_D() << "a new need register module: "
            << QFileInfo(module_library_path).fileName();

    auto runner = Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);

    runner->PostTask(new Thread::Task(
        [=](const GpgFrontend::DataObjectPtr&) -> int {
          // register module
          if (!gmc_->RegisterModule(module, candidate.integrated)) return -1;

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
            module_so.auto_activate = candidate.integrated;
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

  void SetNeedRegisterModulesNum(int n) {
    if (need_register_modules_ != -1 || n < 0) return;
    need_register_modules_ = n;
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
    if (need_register_modules_ <= gmc_->GetRegisteredModuleNum()) return;
    need_register_modules_--;
  }

  auto IsAllModulesRegistered() {
    if (need_register_modules_ == -1) return false;
    LOG_D() << "module manager report, need register: "
            << need_register_modules_ << "registered"
            << gmc_->GetRegisteredModuleNum();
    return need_register_modules_ == gmc_->GetRegisteredModuleNum();
  }

  auto GRT() -> GlobalRegisterTable* { return grt_.get(); }

  auto IsEventListening(const EventTriggerIdentifier& trigger_id) -> bool {
    return gmc_->IsEventListening(trigger_id);
  }

 private:
  static ModuleMangerPtr global_module_manager;
  SecureUniquePtr<GlobalModuleContext> gmc_;
  SecureUniquePtr<GlobalRegisterTable> grt_;
  int need_register_modules_ = -1;
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

auto ModuleManager::PrepareModule(const QString& path, bool integrated)
    -> ModuleLoadCandidate {
  return p_->PrepareModule(path, integrated);
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

  // The package it came from, never the descriptor or temporary file its
  // image was mapped through.
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