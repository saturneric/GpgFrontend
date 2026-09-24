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

#include <algorithm>
#include <atomic>
#include <map>
#include <optional>
#include <unordered_map>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/SettingsObject.h"
#include "core/module/GlobalRegisterTable.h"
#include "core/module/Module.h"
#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleEventRegistry.h"
#include "core/module/ModuleExternalTrust.h"
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
  Impl() : grt_(GpgFrontend::SecureCreateUniqueObject<GlobalRegisterTable>()) {}

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
   * @param integrated_ids the integrated ids an external package may not claim
   * @param[out] library_name the entry's filename on this platform
   * @param[out] identity the file the entry named while it was verified
   * @return false when the descriptor or its entry was refused
   */
  auto VerifyAndResolveEntry(ModuleOrigin origin, const QString& package_path,
                             const QSet<QString>& integrated_ids,
                             QString& library_path, ModuleManifest& manifest,
                             QString& module_hash, QString& library_name,
                             ModuleFileIdentity& identity) -> bool {
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

    // An id is owned by one trust origin. Everything keyed by it -- settings,
    // the secure cache, commands, translations -- would otherwise belong to
    // whichever package claimed it first, and the scan order is a directory
    // listing, not a decision. The project's own namespace, and every id an
    // integrated module actually has, are not an external package's to claim.
    if (origin == ModuleOrigin::kEXTERNAL &&
        (IsReservedModuleId(read.manifest.id) ||
         integrated_ids.contains(read.manifest.id))) {
      const auto why =
          QObject::tr(
              "The module claims the ID %1, which belongs to a module "
              "that comes with GpgFrontend.")
              .arg(read.manifest.id);
      LOG_W() << "module manager refuses external module: " << package_path
              << ", reason: " << why;
      RecordRefusal({package_path, origin, read.manifest.id, why,
                     read.signer_public_key, false});
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
      const QString why = QObject::tr(
          "External modules are not supported on macOS: the "
          "system only loads code signed with this "
          "application's own Team ID.");
      LOG_W() << "module manager refuses external module: " << package_path
              << ", reason: " << why;
      RecordRefusal({package_path, origin, read.manifest.id, why,
                     read.signer_public_key, false});
      return false;
#else
      const auto authorization =
          ExternalModuleAuthorization(read.manifest.id, read.signer_public_key);
      if (authorization != ModuleAuthorizationState::kTRUSTED_AND_ENABLED) {
        const QString why =
            authorization == ModuleAuthorizationState::kPUBLISHER_UNTRUSTED
                ? QObject::tr(
                      "Waiting for you to trust the publisher key that "
                      "signed it.")
                : QObject::tr("Waiting for you to enable it.");
        LOG_I() << "module manager holds external module: " << package_path
                << ", reason: "
                << ModuleAuthorizationStateToString(authorization);
        // pending_user_action: this one has an action attached, and telling
        // it apart from a broken module is the difference between a control
        // to press and a problem to report.
        RecordRefusal({package_path, origin, read.manifest.id, why,
                       read.signer_public_key, true});
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
          QObject::tr(
              "The module declares the ID %1 but is in a directory "
              "named %2 instead of %3.")
              .arg(read.manifest.id, namespace_dir.dirName(), expected_key);
      LOG_W() << "module manager refuses module descriptor: " << package_path
              << ", reason: " << why;
      RecordRefusal({package_path, origin, read.manifest.id, why,
                     read.signer_public_key, false});
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

    const auto entry = ResolveAndVerifyNativeEntry(read.manifest, root, policy);
    if (!entry.ok) {
      LOG_W() << "module manager refuses module entry: " << package_path
              << ", reason: " << entry.reason << " ("
              << ModuleEntryStatusToString(entry.status) << ")";
      RecordRefusal({package_path, origin, read.manifest.id, entry.reason,
                     read.signer_public_key, false});
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
    identity = entry.identity;
    return true;
  }

  /**
   * @brief Phase one: verify. Maps nothing, runs no module code.
   *
   * Takes an admission ticket like any other work inside the module system,
   * so a preparation already running when teardown begins is waited for, and
   * one starting after it declines.
   */
  auto PrepareModule(const QString& path, ModuleOrigin origin,
                     const QSet<QString>& integrated_ids)
      -> ModuleLoadCandidate {
    ModuleLoadCandidate candidate;
    candidate.source_path = path;
    candidate.origin = origin;
    candidate.library_path = path;

    ModuleDispatchScope admission(GlobalModuleDispatchGate());
    if (!admission.Entered()) {
      LOG_D() << "module manager declines to prepare during shutdown: " << path;
      return candidate;
    }

    // A module is a signed package, and nothing else: a native library is
    // something a verified descriptor names, never something that is found.
    if (!IsModuleDescriptorFileName(QFileInfo(path).fileName())) {
      const auto why = QObject::tr("This is not a module package.");
      LOG_W() << "module manager refuses: " << path << ", reason: " << why;
      RecordRefusal({path, origin, {}, why, {}, false});
      return candidate;
    }

    QString library_path;
    ModuleManifest verified;
    QString module_hash;
    QString library_name;
    ModuleFileIdentity identity;
    if (!VerifyAndResolveEntry(origin, path, integrated_ids, library_path,
                               verified, module_hash, library_name, identity)) {
      return candidate;
    }
    candidate.library_path = library_path;
    candidate.manifest = verified;
    candidate.module_hash = module_hash;
    candidate.library_name = library_name;
    candidate.identity = identity;
    candidate.ok = true;
    return candidate;
  }

  auto LoadPreparedModule(const ModuleLoadCandidate& candidate) -> bool {
    // Every refusal below owes the same three things: say why, stop waiting
    // for this module, and count it. Written out each time, one copy would
    // eventually forget one of them -- and a forgotten decrement is a startup
    // that never finishes waiting for registration.
    const auto refuse = [this](const QString& why) {
      LOG_W() << "module manager refuses module: " << why;
      DropFromExpectedRegistrations();
      ModuleLoadStats::GetInstance().AddRefusedModule();
      return false;
    };

    if (!candidate.ok || !candidate.manifest.has_value()) {
      return refuse(candidate.source_path);
    }

    // Phase two is serial because QLibrary::load() below runs third-party
    // static initializers. This records that it stayed serial, so a later
    // refactor that parallelises the loop fails a test instead of passing
    // quietly -- see ModuleLoadStats::NativeLoadScope.
    ModuleLoadStats::NativeLoadScope native_load;

    const auto& package_path = candidate.source_path;
    const auto& library_path = candidate.library_path;
    const auto& manifest = *candidate.manifest;

    // Mapping an image is work inside the module system too, so it takes its
    // own ticket -- and shutdown may have begun while phase one was reading a
    // package, in which case loading now would put module code into a process
    // that has already stopped waiting for it.
    ModuleDispatchScope admission(GlobalModuleDispatchGate());
    if (!admission.Entered()) {
      // Not refuse(): shutdown overtaking a load is routine, not a fault of
      // the module, so it is not worth a warning in the user's log.
      LOG_D() << "module manager abandons a load that shutdown overtook: "
              << package_path;
      DropFromExpectedRegistrations();
      ModuleLoadStats::GetInstance().AddRefusedModule();
      return false;
    }

    // Phase one verified these bytes by path, and the loader maps by path.
    // Between the two the file must still be the one that was hashed: the
    // external namespace is writable by the user, and a library swapped in
    // after verification would run with the signed package's identity.
    if (CaptureModuleFileIdentity(library_path) != candidate.identity) {
      return refuse(QString("%1, reason: it changed after it was verified")
                        .arg(library_path));
    }

    auto module_library = std::make_unique<QLibrary>(library_path);

    ScopedModuleLibrarySearchPath search_path(library_path);
    if (!module_library->load()) {
      return refuse(
          QString("%1, reason: %2")
              .arg(module_library->fileName(), module_library->errorString()));
    }

    // Ownership moves into the Module, which is what gives teardown something
    // to unload.
    auto module = SecureCreateSharedObject<Module>(std::move(module_library),
                                                   candidate.module_hash);
    if (!module->IsGood()) {
      module->UnloadLibrary();
      return refuse(
          QString("%1, reason: it is not a usable module").arg(library_path));
    }

    // Runtime identity against signed identity. Without this the signature
    // covers a name nothing enforces: a package could say it is one module
    // and carry another, and everything downstream -- settings, activation,
    // the module list -- would key off the binary's word for it.
    if (module->GetModuleIdentifier() != manifest.id) {
      const auto said = module->GetModuleIdentifier();
      module->UnloadLibrary();
      return refuse(QString("%1, reason: its manifest says %2 and the module "
                            "inside says %3")
                        .arg(package_path, manifest.id, said));
    }

    // The same argument applies to the version: it is what an update
    // decision is made on.
    if (module->GetModuleVersion() != manifest.version) {
      const auto said = module->GetModuleVersion();
      module->UnloadLibrary();
      return refuse(
          QString("%1, reason: its manifest says version %2 and the module "
                  "inside says %3")
              .arg(package_path, manifest.version, said));
    }

    module->SetModuleMetaData(manifest.metadata);
    // What a user can act on is the package, not the file the image arrived
    // through.
    module->SetSourcePackagePath(package_path);
    module->SetModuleManifest(manifest);

    ModuleLoadStats::GetInstance().AddLoadedModule();
    LOG_D() << "module loaded, awaiting registration: "
            << QFileInfo(package_path).fileName();

    RegisterLoadedModule(module, candidate.origin == ModuleOrigin::kINTEGRATED,
                         true);
    return true;
  }

  /// @param from_scan whether this is one of the modules the startup scan is
  ///        counting; only those move IsAllModulesRegistered().
  void RegisterLoadedModule(const ModulePtr& module, bool integrated,
                            bool from_scan) {
    PostToRunner(
        "module/register", [this, module, integrated, from_scan]() -> int {
          // Counted once this module is DECIDED -- registered and, if its
          // settings say so, activated -- whatever the outcome. Startup waits
          // for this count, and what the UI does next (installing module
          // translations, building menus from their scripts) needs activation
          // to have happened, not merely registration.
          const auto counted = qScopeGuard([this, from_scan]() {
            if (from_scan) ++registered_modules_;
          });
          if (!register_now(module, integrated)) return -1;

          const auto settings =
              ReconcileModuleSettings(module->GetModuleIdentifier(),
                                      module->GetModuleHash(), integrated);
          if (settings.auto_activate &&
              !activate_now(module->GetModuleIdentifier())) {
            return -1;
          }
          return 0;
        });
  }

  /// Set once, by whichever thread gets there first.
  void SetNeedRegisterModulesNum(int n) {
    if (n < 0) return;
    auto unset = -1;
    need_register_modules_.compare_exchange_strong(unset, n);
  }

  auto SearchModule(const ModuleIdentifier& module_id) -> ModulePtr {
    const QMutexLocker lock(&mutex_);
    const auto* rec = find_locked(module_id);
    return rec == nullptr ? nullptr : rec->module;
  }

  auto ListAllRegisteredModuleID() -> QStringList {
    const QMutexLocker lock(&mutex_);
    QStringList ids;
    for (const auto& [id, rec] : records_) ids.append(id);
    ids.sort();
    return ids;
  }

  auto TakeAllModules() -> QList<ModulePtr> {
    const QMutexLocker lock(&mutex_);
    QList<ModulePtr> modules;
    for (const auto& [id, rec] : records_) {
      if (rec.module != nullptr) modules.append(rec.module);
    }
    records_.clear();
    events_.clear();
    triggers_.clear();
    return modules;
  }

  auto IsModuleActivated(const ModuleIdentifier& id) -> bool {
    const QMutexLocker lock(&mutex_);
    const auto* rec = find_locked(id);
    return rec != nullptr && rec->state == State::kACTIVE;
  }

  auto IsIntegratedModule(const ModuleIdentifier& id) -> bool {
    const QMutexLocker lock(&mutex_);
    const auto* rec = find_locked(id);
    return rec != nullptr && rec->integrated;
  }

  auto GetModuleListening(const ModuleIdentifier& id) -> QStringList {
    const QMutexLocker lock(&mutex_);
    const auto* rec = find_locked(id);
    return rec == nullptr ? QStringList() : rec->listening;
  }

  auto ListenEvent(const ModuleIdentifier& module_id,
                   const EventIdentifier& event) -> bool {
    // An id the Host never fires is a subscription that can only ever be
    // silence. Refusing it names the module and the id now.
    if (!IsKnownModuleEvent(event)) {
      LOG_W() << "refusing to subscribe module" << module_id << "to event"
              << event
              << ": this host fires no such event. See "
                 "core/module/ModuleEventRegistry.cpp for the catalogue.";
      return false;
    }

    const QMutexLocker lock(&mutex_);
    auto* rec = find_locked(module_id);
    if (rec == nullptr) {
      LOG_W() << "module" << module_id << "not found in the register table";
      return false;
    }
    if (rec->state != State::kACTIVATING && rec->state != State::kACTIVE) {
      LOG_W() << "refusing to subscribe module" << module_id << "to event"
              << event << ": the module is not active";
      return false;
    }

    // THE allowlist check. The module runtime reconciles its handler table
    // against the manifest too, but that code ships inside the module; this
    // is where the Host decides for itself.
    const auto manifest = rec->module->GetModuleManifest();
    if (!manifest.has_value() || !manifest->events.contains(event)) {
      LOG_W() << "refusing to subscribe module" << module_id << "to event"
              << event
              << ": its signed manifest does not declare it. The declared "
                 "list is the allowlist -- add the event to module.json and "
                 "rebuild.";
      return false;
    }

    if (rec->listening.contains(event)) return true;
    rec->listening.append(event);
    events_[event].insert(module_id);
    return true;
  }

  void TriggerEvent(const EventReference& event) {
    const auto event_id = event->GetIdentifier();
    const auto trigger_id = event->GetTriggerIdentifier();

    QList<QPair<ModuleIdentifier, ModulePtr>> targets;
    {
      const QMutexLocker lock(&mutex_);
      const auto it = events_.find(event_id);
      if (it != events_.end()) {
        for (const auto& listener : it->second) {
          const auto* rec = find_locked(listener);
          if (rec == nullptr || rec->state != State::kACTIVE) continue;
          targets.append({listener, rec->module});
        }
      }
      if (!targets.isEmpty()) {
        auto& trigger = triggers_[trigger_id];
        trigger.event = event;
        for (const auto& t : targets) trigger.awaiting.insert(t.first);
      }
    }

    // Nobody to ask is still an answer: a caller waiting on the callback --
    // a modal waiting dialog, say -- must hear that no one is coming.
    if (targets.isEmpty()) {
      LOG_I() << "event" << event_id << "has no active listeners";
      event->ExecuteCallback(
          {}, FailureParams(QStringLiteral("no active module handles this")));
      return;
    }

    for (const auto& [listener, module] : targets) {
      PostToRunner(
          QString("event/%1/module/exec/%2").arg(event_id, listener),
          [this, listener = listener, module = module, event,
           trigger_id]() -> int {
            // Admission is taken here, in the task, not where the task was
            // posted: whether module code may run is a question about NOW.
            ModuleDispatchScope scope(GlobalModuleDispatchGate());
            if (!scope.Entered()) {
              FailListener(trigger_id, listener,
                           QStringLiteral("the host is shutting down"));
              return kModuleUnloadingCode;
            }
            ModuleDispatchScope own(ModuleEntryGate(listener));
            if (!own.Entered()) {
              FailListener(trigger_id, listener,
                           QStringLiteral("the module is not active"));
              return kModuleUnloadingCode;
            }

            const auto rc = module->Exec(event);
            if (rc < 0) {
              LOG_W() << "module" << listener << "failed to handle event"
                      << event->GetIdentifier() << "- return code:" << rc;
              FailListener(trigger_id, listener,
                           QStringLiteral("the module failed to handle it"));
            }
            return rc;
          });
    }
  }

  auto AnswerEvent(const EventTriggerIdentifier& trigger_id,
                   const ModuleIdentifier& listener,
                   const Event::Params& params) -> bool {
    auto event = take_answer(trigger_id, listener);
    if (event == nullptr) {
      LOG_W() << "refusing an answer from module" << listener << "to trigger"
              << trigger_id
              << ": it was not delivered that event, or already answered it";
      return false;
    }
    event->ExecuteCallback(listener, params);
    return true;
  }

  auto PendingTriggerCount() -> int {
    const QMutexLocker lock(&mutex_);
    return static_cast<int>(triggers_.size());
  }

  auto LifecycleSnapshot() -> QList<ModuleLifecycleSnapshot> {
    const QMutexLocker lock(&mutex_);
    QList<ModuleLifecycleSnapshot> out;
    out.reserve(static_cast<qsizetype>(records_.size()));
    for (const auto& [id, rec] : records_) {
      int owed = 0;
      for (const auto& [trigger_id, trigger] : triggers_) {
        if (trigger.awaiting.contains(id)) ++owed;
      }
      out.append({id, rec.state, rec.integrated, rec.listening, owed});
    }
    std::sort(out.begin(), out.end(),
              [](const auto& a, const auto& b) { return a.id < b.id; });
    return out;
  }

  auto ListenersOf(const EventIdentifier& event_id) -> QStringList {
    const QMutexLocker lock(&mutex_);
    const auto it = events_.find(event_id);
    if (it == events_.end()) return {};
    auto listeners = QStringList(it->second.cbegin(), it->second.cend());
    listeners.sort();
    return listeners;
  }

  auto IsEventListening(const EventIdentifier& event_id) -> bool {
    const QMutexLocker lock(&mutex_);
    const auto it = events_.find(event_id);
    return it != events_.end() && !it->second.isEmpty();
  }

  void ActiveModule(const ModuleIdentifier& id,
                    const ModuleTransitionCallback& done) {
    PostToRunner(
        "module/activate/" + id,
        [this, id]() -> int { return activate_now(id) ? 0 : -1; }, done);
  }

  void DeactivateModule(const ModuleIdentifier& id,
                        const ModuleTransitionCallback& done) {
    PostToRunner(
        "module/deactivate/" + id,
        [this, id]() -> int { return deactivate_now(id, true) ? 0 : -1; },
        done);
  }

  auto DeactivateAndUnregisterAllForShutdown() -> QStringList {
    const auto ids = ListAllRegisteredModuleID();
    for (const auto& id : ids) deactivate_now(id, false);
    for (const auto& id : ids) {
      if (auto module = SearchModule(id); module != nullptr) {
        module->UnRegister();
      }
    }
    return ids;
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

  /// One fewer module the startup scan is still waiting for.
  ///
  /// Only while startup is still waiting: a module refused after startup
  /// finished would otherwise push the target below the count permanently,
  /// and the "modules are ready" signal would never be true again.
  void DropFromExpectedRegistrations() {
    auto current = need_register_modules_.load(std::memory_order_relaxed);
    while (current > registered_modules_.load()) {
      if (need_register_modules_.compare_exchange_weak(
              current, current - 1, std::memory_order_relaxed)) {
        return;
      }
    }
  }

  auto IsAllModulesRegistered() -> bool {
    // Read once. Logging one value and comparing another is how a report says
    // "need 4, registered 4" and still answers false.
    const auto needed = need_register_modules_.load(std::memory_order_relaxed);
    if (needed == -1) return false;
    const auto registered = registered_modules_.load();
    LOG_D() << "module manager report: needing registration" << needed
            << ", registered" << registered;
    return needed == registered;
  }

  auto GRT() -> GlobalRegisterTable* { return grt_.get(); }

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
  using State = ModuleLifecycleState;

  struct ModuleRecord {
    ModulePtr module;
    bool integrated = false;
    State state = State::kREGISTERED;
    QStringList listening;
  };

  /// One delivered event and the listeners that still owe it an answer.
  struct Trigger {
    EventReference event;
    QSet<ModuleIdentifier> awaiting;
  };

  SecureUniquePtr<GlobalRegisterTable> grt_;
  std::atomic<int> need_register_modules_ = -1;
  std::atomic<int> registered_modules_ = 0;

  /// Guards records_, events_ and triggers_. Never held across a call into
  /// module code or an event callback.
  QMutex mutex_;
  std::unordered_map<ModuleIdentifier, ModuleRecord> records_;
  std::map<EventIdentifier, QSet<ModuleIdentifier>> events_;
  std::map<EventTriggerIdentifier, Trigger> triggers_;

  /// Guards refusals_ alone. Phase one prepares modules concurrently, so
  /// every record below is written from a worker thread and read from the UI.
  QMutex refusals_mutex_;
  QList<ModuleRefusalRecord> refusals_;

  static auto FailureParams(const QString& reason) -> Event::Params {
    // `err` is what the runtime's own failure answers carry; `error_msg` is
    // what the key-server callers read. Both, so neither kind of caller has
    // to know which side answered.
    return {{QStringLiteral("ret"), GFBuffer(QStringLiteral("-1"))},
            {QStringLiteral("err"), GFBuffer(reason)},
            {QStringLiteral("error_msg"), GFBuffer(reason)}};
  }

  static void PostToRunner(const QString& name, std::function<int()> fn,
                           const ModuleTransitionCallback& done = nullptr) {
    auto runner = Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);
    auto runnable = [fn = std::move(fn)](const DataObjectPtr&) -> int {
      return fn();
    };
    if (done) {
      runner->PostTask(new Thread::Task(
          runnable, name, nullptr,
          [done](int rc, const DataObjectPtr&) { done(rc == 0); }));
    } else {
      runner->PostTask(new Thread::Task(runnable, name, nullptr));
    }
  }

  auto find_locked(const ModuleIdentifier& id) -> ModuleRecord* {
    const auto it = records_.find(id);
    return it == records_.end() ? nullptr : &it->second;
  }

  void drop_listeners_locked(const ModuleIdentifier& id, ModuleRecord& rec) {
    for (const auto& event_id : rec.listening) {
      const auto it = events_.find(event_id);
      if (it == events_.end()) continue;
      it->second.remove(id);
      if (it->second.isEmpty()) events_.erase(it);
    }
    rec.listening.clear();
  }

  auto register_now(const ModulePtr& module, bool integrated) -> bool {
    if (module == nullptr || !module->IsGood()) {
      LOG_W() << "refusing to register a module that is not usable";
      return false;
    }
    const auto id = module->GetModuleIdentifier();
    const QMutexLocker lock(&mutex_);
    if (records_.find(id) != records_.end()) {
      LOG_W() << "module" << id << "has already been registered";
      return false;
    }
    records_[id] = ModuleRecord{module, integrated, State::kREGISTERED, {}};
    LOG_D() << "registered module" << id;
    return true;
  }

  auto activate_now(const ModuleIdentifier& id) -> bool {
    ModulePtr module;
    {
      const QMutexLocker lock(&mutex_);
      auto* rec = find_locked(id);
      if (rec == nullptr) {
        LOG_W() << "module" << id << "not found in the register table";
        return false;
      }
      if (rec->state == State::kACTIVE) return true;
      if (rec->state == State::kACTIVATING ||
          rec->state == State::kDEACTIVATING) {
        return false;
      }
      rec->state = State::kACTIVATING;
      module = rec->module;
    }

    LOG_D() << "activating module" << id;
    const auto rc = module->Active();

    const QMutexLocker lock(&mutex_);
    auto* rec = find_locked(id);
    if (rec == nullptr) return false;
    if (rc == 0) {
      rec->state = State::kACTIVE;
      LOG_D() << "activated module" << id;
      return true;
    }
    // Whatever it subscribed to while it tried goes with it.
    rec->state = State::kFAILED;
    drop_listeners_locked(id, *rec);
    LOG_W() << "module" << id << "failed to activate, return code:" << rc;
    return false;
  }

  auto deactivate_now(const ModuleIdentifier& id, bool revoke) -> bool {
    ModulePtr module;
    {
      const QMutexLocker lock(&mutex_);
      auto* rec = find_locked(id);
      if (rec == nullptr) return false;
      if (rec->state != State::kACTIVE) {
        return rec->state != State::kACTIVATING &&
               rec->state != State::kDEACTIVATING;
      }
      rec->state = State::kDEACTIVATING;
      // No new trigger reaches it from here.
      drop_listeners_locked(id, *rec);
      module = rec->module;
    }

    // Cannot be refused: whatever the module says, it is inactive after this.
    if (const auto rc = module->Deactivate(revoke); rc != 0) {
      LOG_W() << "module" << id
              << "reported a failure while deactivating:" << rc;
    }

    {
      const QMutexLocker lock(&mutex_);
      if (auto* rec = find_locked(id); rec != nullptr) {
        rec->state = State::kINACTIVE;
      }
    }

    // It can no longer answer what it was asked; its askers are told so.
    fail_awaiting(id, QStringLiteral("the module was deactivated"));
    return true;
  }

  /// Claim @p listener's answer to @p trigger_id. Null when it owes none.
  auto take_answer(const EventTriggerIdentifier& trigger_id,
                   const ModuleIdentifier& listener) -> EventReference {
    const QMutexLocker lock(&mutex_);
    const auto it = triggers_.find(trigger_id);
    if (it == triggers_.end() || !it->second.awaiting.contains(listener)) {
      return nullptr;
    }
    it->second.awaiting.remove(listener);
    auto event = it->second.event;
    // The event, its parameters and its callback go the moment the last
    // answer is in -- they used to stay for the life of the process.
    if (it->second.awaiting.isEmpty()) triggers_.erase(it);
    return event;
  }

  void FailListener(const EventTriggerIdentifier& trigger_id,
                    const ModuleIdentifier& listener, const QString& reason) {
    auto event = take_answer(trigger_id, listener);
    if (event == nullptr) return;  // already answered
    event->ExecuteCallback(listener, FailureParams(reason));
  }

  void fail_awaiting(const ModuleIdentifier& listener, const QString& reason) {
    QList<EventTriggerIdentifier> owed;
    {
      const QMutexLocker lock(&mutex_);
      for (const auto& [trigger_id, trigger] : triggers_) {
        if (trigger.awaiting.contains(listener)) owed.append(trigger_id);
      }
    }
    for (const auto& trigger_id : owed) {
      FailListener(trigger_id, listener, reason);
    }
  }
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

auto ReconcileModuleSettings(const QString& module_id,
                             const QString& module_hash, bool integrated)
    -> ModuleSO {
  SettingsObject so(QString("module.%1.so").arg(module_id));
  ModuleSO module_so(so);

  // A changed hash is a different build -- a developer's rebuild, or an
  // upgrade -- and the stored record is refreshed for it. What the user chose
  // is not the build's to undo: an explicit choice survives, and only a
  // choice nobody made takes the default.
  if (module_so.module_id != module_id ||
      module_so.module_hash != module_hash) {
    module_so.module_id = module_id;
    module_so.module_hash = module_hash;
    if (!module_so.set_by_user) module_so.auto_activate = integrated;
    so.Store(module_so.ToJson());
  }
  return module_so;
}

ModuleManager::ModuleManager(int channel)
    : SingletonFunctionObject<ModuleManager>(channel),
      p_(SecureCreateUniqueObject<Impl>()) {}

ModuleManager::~ModuleManager() = default;

auto ModuleManager::ListModuleRefusals() -> QList<ModuleRefusalRecord> {
  return p_->ListRefusals();
}

auto ModuleManager::PrepareModule(const QString& path, ModuleOrigin origin,
                                  const QSet<QString>& integrated_ids)
    -> ModuleLoadCandidate {
  return p_->PrepareModule(path, origin, integrated_ids);
}

auto ModuleManager::LoadPreparedModule(const ModuleLoadCandidate& candidate)
    -> bool {
  return p_->LoadPreparedModule(candidate);
}

void ModuleManager::RegisterLoadedModule(ModulePtr module, bool integrated) {
  p_->RegisterLoadedModule(module, integrated, false);
}

auto ModuleManager::SearchModule(ModuleIdentifier id) -> ModulePtr {
  return p_->SearchModule(id);
}

auto ModuleManager::ListenEvent(ModuleIdentifier module, EventIdentifier event)
    -> bool {
  return p_->ListenEvent(module, event);
}

auto ModuleManager::GetModuleListening(ModuleIdentifier id) -> QStringList {
  return p_->GetModuleListening(id);
}

void ModuleManager::TriggerEvent(EventReference event) {
  p_->TriggerEvent(event);
}

auto ModuleManager::AnswerEvent(const EventTriggerIdentifier& trigger_id,
                                const ModuleIdentifier& listener,
                                const Event::Params& params) -> bool {
  return p_->AnswerEvent(trigger_id, listener, params);
}

auto ModuleManager::PendingTriggerCount() -> int {
  return p_->PendingTriggerCount();
}

void ModuleManager::ActiveModule(ModuleIdentifier id,
                                 ModuleTransitionCallback done) {
  p_->ActiveModule(id, done);
}

void ModuleManager::DeactivateModule(ModuleIdentifier id,
                                     ModuleTransitionCallback done) {
  p_->DeactivateModule(id, done);
}

auto ModuleManager::DeactivateAndUnregisterAllForShutdown() -> QStringList {
  return p_->DeactivateAndUnregisterAllForShutdown();
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
}

auto ModuleManager::GRT() -> GlobalRegisterTable* { return p_->GRT(); }

auto ModuleManager::IsAllModulesRegistered() -> bool {
  return p_->IsAllModulesRegistered();
}

void ModuleManager::SetNeedRegisterModulesNum(int n) {
  p_->SetNeedRegisterModulesNum(n);
}

auto ModuleManager::LifecycleSnapshot() -> QList<ModuleLifecycleSnapshot> {
  return p_->LifecycleSnapshot();
}

auto ModuleManager::ListenersOf(const EventIdentifier& event_id)
    -> QStringList {
  return p_->ListenersOf(event_id);
}

auto ModuleLifecycleStateName(ModuleLifecycleState state) -> QString {
  switch (state) {
    case ModuleLifecycleState::kREGISTERED:
      return QStringLiteral("Registered");
    case ModuleLifecycleState::kACTIVATING:
      return QStringLiteral("Activating");
    case ModuleLifecycleState::kACTIVE:
      return QStringLiteral("Active");
    case ModuleLifecycleState::kDEACTIVATING:
      return QStringLiteral("Deactivating");
    case ModuleLifecycleState::kINACTIVE:
      return QStringLiteral("Inactive");
    case ModuleLifecycleState::kFAILED:
      return QStringLiteral("Failed");
  }
  return QStringLiteral("Unknown");
}

auto ModuleManager::IsEventListening(const EventIdentifier& event_id) -> bool {
  return p_->IsEventListening(event_id);
}

auto IsEventListening(const EventTriggerIdentifier& trigger_id) -> bool {
  return ModuleManager::GetInstance().IsEventListening(trigger_id);
}

}  // namespace GpgFrontend::Module
