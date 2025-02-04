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

#include <memory>

#include "core/function/GlobalSettingStation.h"
#include "core/function/SecureMemoryAllocator.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/SettingsObject.h"
#include "core/module/GlobalModuleContext.h"
#include "core/module/GlobalRegisterTable.h"
#include "core/module/Module.h"
#include "core/struct/settings_object/ModuleSO.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Module {

class ModuleManager::Impl {
 public:
  Impl()
      : gmc_(GpgFrontend::SecureCreateUniqueObject<GlobalModuleContext>()),
        grt_(GpgFrontend::SecureCreateUniqueObject<GlobalRegisterTable>()) {}

  ~Impl() = default;

  auto LoadAndRegisterModule(const QString& module_library_path,
                             bool integrated_module) -> bool {
    QLibrary module_library(module_library_path);
    if (!module_library.load()) {
      LOG_W() << "module manager failed to load module: "
              << module_library.fileName()
              << ", reason: " << module_library.errorString();
      need_register_modules_--;
      return false;
    }

    auto module = SecureCreateSharedObject<Module>(module_library);
    if (!module->IsGood()) {
      LOG_W() << "module manager failed to load module, "
                 "reason: illegal module: "
              << module_library.fileName();
      need_register_modules_--;
      return false;
    }

    module->SetGPC(gmc_.get());

    LOG_D() << "a new need register module: "
            << QFileInfo(module_library_path).fileName();

    auto runner = Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);

    runner->PostTask(new Thread::Task(
        [=](GpgFrontend::DataObjectPtr) -> int {
          // register module
          if (!gmc_->RegisterModule(module, integrated_module)) return -1;

          return 0;
        },
        __func__, nullptr));

    runner->PostTask(new Thread::Task(
        [=](GpgFrontend::DataObjectPtr) -> int {
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

  auto SearchModule(ModuleIdentifier module_id) -> ModulePtr {
    return gmc_->SearchModule(std::move(module_id));
  }

  auto ListAllRegisteredModuleID() -> QStringList {
    return gmc_->ListAllRegisteredModuleID();
  }

  void RegisterModule(const ModulePtr& module) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
        ->PostTask(new Thread::Task(
            [=](GpgFrontend::DataObjectPtr) -> int {
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

  auto SearchEvent(EventTriggerIdentifier trigger_id)
      -> std::optional<EventReference> {
    return gmc_->SearchEvent(std::move(trigger_id));
  }

  auto GetModuleListening(ModuleIdentifier module_id) -> QStringList {
    return gmc_->GetModuleListening(std::move(module_id));
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

  auto GetTaskRunner(ModuleIdentifier module_id)
      -> std::optional<TaskRunnerPtr> {
    return gmc_->GetTaskRunner(std::move(module_id));
  }

  auto UpsertRTValue(Namespace n, Key k, std::any v) -> bool {
    return grt_->PublishKV(n, k, v);
  }

  auto RetrieveRTValue(Namespace n, Key k) -> std::optional<std::any> {
    return grt_->LookupKV(n, k);
  }

  auto ListenPublish(QObject* o, Namespace n, Key k, LPCallback c) -> bool {
    return grt_->ListenPublish(o, n, k, c);
  }

  auto ListRTChildKeys(const QString& n, const QString& k) -> QContainer<Key> {
    return grt_->ListChildKeys(n, k);
  }

  auto IsModuleActivated(ModuleIdentifier id) -> bool {
    return gmc_->IsModuleActivated(id);
  }

  auto IsIntegratedModule(ModuleIdentifier id) -> bool {
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

 private:
  static ModuleMangerPtr global_module_manager;
  SecureUniquePtr<GlobalModuleContext> gmc_;
  SecureUniquePtr<GlobalRegisterTable> grt_;
  QContainer<QLibrary> module_libraries_;
  int need_register_modules_ = -1;
};

auto IsModuleActivate(ModuleIdentifier id) -> bool {
  return ModuleManager::GetInstance().IsModuleActivated(id);
}

auto GPGFRONTEND_CORE_EXPORT IsModuleExists(ModuleIdentifier id) -> bool {
  auto module = ModuleManager::GetInstance().SearchModule(id);
  return module != nullptr && module->IsGood();
}

auto UpsertRTValue(const QString& namespace_, const QString& key,
                   const std::any& value) -> bool {
  return ModuleManager::GetInstance().UpsertRTValue(namespace_, key,
                                                    std::any(value));
}

auto ListenRTPublishEvent(QObject* o, Namespace n, Key k,
                          LPCallback c) -> bool {
  return ModuleManager::GetInstance().ListenRTPublish(o, n, k, c);
}

auto ListRTChildKeys(const QString& namespace_,
                     const QString& key) -> QContainer<Key> {
  return ModuleManager::GetInstance().ListRTChildKeys(namespace_, key);
}

ModuleManager::ModuleManager(int channel)
    : SingletonFunctionObject<ModuleManager>(channel),
      p_(SecureCreateUniqueObject<Impl>()) {}

ModuleManager::~ModuleManager() = default;

auto ModuleManager::LoadModule(QString module_library_path,
                               bool integrated_module) -> bool {
  return p_->LoadAndRegisterModule(module_library_path, integrated_module);
}

auto ModuleManager::SearchModule(ModuleIdentifier module_id) -> ModulePtr {
  return p_->SearchModule(std::move(module_id));
}

void ModuleManager::RegisterModule(ModulePtr module) {
  return p_->RegisterModule(module);
}

void ModuleManager::ListenEvent(ModuleIdentifier module,
                                EventIdentifier event) {
  return p_->ListenEvent(module, event);
}

auto ModuleManager::GetModuleListening(ModuleIdentifier module_id)
    -> QStringList {
  return p_->GetModuleListening(module_id);
}

void ModuleManager::TriggerEvent(EventReference event) {
  return p_->TriggerEvent(event);
}

auto ModuleManager::SearchEvent(EventTriggerIdentifier trigger_id)
    -> std::optional<EventReference> {
  return p_->SearchEvent(std::move(trigger_id));
}

void ModuleManager::ActiveModule(ModuleIdentifier id) {
  return p_->ActiveModule(id);
}

void ModuleManager::DeactivateModule(ModuleIdentifier id) {
  return p_->DeactivateModule(id);
}

auto ModuleManager::GetTaskRunner(ModuleIdentifier id)
    -> std::optional<TaskRunnerPtr> {
  return p_->GetTaskRunner(std::move(id));
}

auto ModuleManager::UpsertRTValue(Namespace n, Key k, std::any v) -> bool {
  return p_->UpsertRTValue(n, k, v);
}

auto ModuleManager::RetrieveRTValue(Namespace n,
                                    Key k) -> std::optional<std::any> {
  return p_->RetrieveRTValue(n, k);
}

auto ModuleManager::ListenRTPublish(QObject* o, Namespace n, Key k,
                                    LPCallback c) -> bool {
  return p_->ListenPublish(o, n, k, c);
}

auto ModuleManager::ListRTChildKeys(const QString& n,
                                    const QString& k) -> QContainer<Key> {
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
}  // namespace GpgFrontend::Module