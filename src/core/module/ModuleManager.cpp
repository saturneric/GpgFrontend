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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/SettingsObject.h"
#include "core/module/GlobalModuleContext.h"
#include "core/module/GlobalRegisterTable.h"
#include "core/module/Module.h"
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

auto InspectModuleLibrary(const QString& module_library_path)
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

  auto LoadAndRegisterModule(const QString& module_library_path,
                             bool integrated_module) -> bool {
    // everything that can be decided without mapping the image has to be
    // decided here: QLibrary::load() below runs the module's own initializers
    const auto inspection = InspectModuleLibrary(module_library_path);
    if (!inspection.ok) {
      LOG_W() << "module manager refuses to load module: "
              << module_library_path << ", reason: " << inspection.reason;
      need_register_modules_--;
      return false;
    }

    QLibrary module_library(module_library_path);

    ScopedModuleLibrarySearchPath search_path(module_library_path);
    if (!module_library.load()) {
      LOG_W() << "module manager failed to load module: "
              << module_library.fileName()
              << ", reason: " << module_library.errorString();
      need_register_modules_--;
      return false;
    }

    auto module =
        SecureCreateSharedObject<Module>(module_library, inspection.hash);
    if (!module->IsGood()) {
      LOG_W() << "module manager failed to load module, "
                 "reason: illegal module: "
              << module_library.fileName();
      // drop the resolved symbol pointers before the image goes away, then
      // unload so a rejected module does not stay mapped for the whole run
      module.reset();
      module_library.unload();
      need_register_modules_--;
      return false;
    }

    module->SetGPC(gmc_.get());

    LOG_D() << "a new need register module: "
            << QFileInfo(module_library_path).fileName();

    auto runner = Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);

    runner->PostTask(new Thread::Task(
        [=](const GpgFrontend::DataObjectPtr&) -> int {
          // register module
          if (!gmc_->RegisterModule(module, integrated_module)) return -1;

          return 0;
        },
        __func__, nullptr));

    runner->PostTask(new Thread::Task(
        [=](const GpgFrontend::DataObjectPtr&) -> int {
          const auto module_id = module->GetModuleIdentifier();
          const auto module_hash = module->GetModuleHash();

          SettingsObject so(QString("module.%1.so").arg(module_id));
          ModuleSO module_so(so);

          // reset module settings if necessary
          if (module_so.module_id != module_id ||
              module_so.module_hash != module_hash) {
            module_so.module_id = module_id;
            module_so.module_hash = module_hash;
            // auto active integrated module by default
            module_so.auto_activate = integrated_module;
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

auto IsModuleActivate(ModuleIdentifier id) -> bool {
  return ModuleManager::GetInstance().IsModuleActivated(std::move(id));
}

auto GF_CORE_EXPORT IsModuleExists(ModuleIdentifier id) -> bool {
  auto module = ModuleManager::GetInstance().SearchModule(std::move(id));
  return module != nullptr && module->IsGood();
}

auto UpsertRTValue(const QString& namespace_, const QString& key,
                   const std::any& value) -> bool {
  return ModuleManager::GetInstance().UpsertRTValue(namespace_, key,
                                                    std::any(value));
}

auto ListenRTPublishEvent(QObject* o, Namespace n, Key k, LPCallback c)
    -> bool {
  return ModuleManager::GetInstance().ListenRTPublish(
      o, std::move(n), std::move(k), std::move(c));
}

auto ListRTChildKeys(const QString& namespace_, const QString& key)
    -> QContainer<Key> {
  return ModuleManager::GetInstance().ListRTChildKeys(namespace_, key);
}

ModuleManager::ModuleManager(int channel)
    : SingletonFunctionObject<ModuleManager>(channel),
      p_(SecureCreateUniqueObject<Impl>()) {}

ModuleManager::~ModuleManager() = default;

auto ModuleManager::LoadModule(QString path, bool integrated) -> bool {
  return p_->LoadAndRegisterModule(path, integrated);
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

auto ModuleManager::ListAllRegisteredModuleID() -> QStringList {
  return p_->ListAllRegisteredModuleID();
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