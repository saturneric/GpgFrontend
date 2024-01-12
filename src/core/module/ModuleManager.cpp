/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
#include <utility>

#include "GpgConstants.h"
#include "core/module/GlobalModuleContext.h"
#include "core/module/GlobalRegisterTable.h"
#include "core/module/Module.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunner.h"
#include "function/SecureMemoryAllocator.h"
#include "function/basic/GpgFunctionObject.h"
#include "thread/TaskRunnerGetter.h"
#include "utils/MemoryUtils.h"

namespace GpgFrontend::Module {

class ModuleManager::Impl {
 public:
  Impl()
      : gmc_(GpgFrontend::SecureCreateUniqueObject<GlobalModuleContext>()),
        grt_(GpgFrontend::SecureCreateUniqueObject<GlobalRegisterTable>()) {}

  ~Impl() = default;

  void RegisterModule(const ModulePtr& module) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
        ->PostTask(new Thread::Task(
            [=](GpgFrontend::DataObjectPtr) -> int {
              module->SetGPC(gmc_.get());
              gmc_->RegisterModule(module);
              return 0;
            },
            __func__, nullptr));
  }

  void TriggerEvent(const EventRefrernce& event) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
        ->PostTask(new Thread::Task(
            [=](const GpgFrontend::DataObjectPtr&) -> int {
              gmc_->TriggerEvent(event);
              return 0;
            },
            __func__, nullptr));
  }

  void ActiveModule(const ModuleIdentifier& identifier) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
        ->PostTask(new Thread::Task(
            [=](const GpgFrontend::DataObjectPtr&) -> int {
              gmc_->ActiveModule(identifier);
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

  auto ListRTChildKeys(const QString& n, const QString& k) -> std::vector<Key> {
    return grt_->ListChildKeys(n, k);
  }

  auto IsModuleActivated(ModuleIdentifier id) -> bool {
    return gmc_->IsModuleActivated(id);
  }

 private:
  static ModuleMangerPtr global_module_manager;
  SecureUniquePtr<GlobalModuleContext> gmc_;
  SecureUniquePtr<GlobalRegisterTable> grt_;
};

auto IsModuleAcivate(ModuleIdentifier id) -> bool {
  return ModuleManager::GetInstance().IsModuleActivated(id);
}

auto UpsertRTValue(const QString& namespace_, const QString& key,
                   const std::any& value) -> bool {
  return ModuleManager::GetInstance().UpsertRTValue(namespace_, key,
                                                    std::any(value));
}

auto ListenRTPublishEvent(QObject* o, Namespace n, Key k, LPCallback c)
    -> bool {
  return ModuleManager::GetInstance().ListenRTPublish(o, n, k, c);
}

auto ListRTChildKeys(const QString& namespace_, const QString& key)
    -> std::vector<Key> {
  return ModuleManager::GetInstance().ListRTChildKeys(namespace_, key);
}

ModuleManager::ModuleManager(int channel)
    : SingletonFunctionObject<ModuleManager>(channel),
      p_(SecureCreateUniqueObject<Impl>()) {}

ModuleManager::~ModuleManager() = default;

void ModuleManager::RegisterModule(ModulePtr module) {
  return p_->RegisterModule(module);
}

void ModuleManager::TriggerEvent(EventRefrernce event) {
  return p_->TriggerEvent(event);
}

void ModuleManager::ActiveModule(ModuleIdentifier id) {
  return p_->ActiveModule(id);
}

auto ModuleManager::GetTaskRunner(ModuleIdentifier id)
    -> std::optional<TaskRunnerPtr> {
  return p_->GetTaskRunner(std::move(id));
}

auto ModuleManager::UpsertRTValue(Namespace n, Key k, std::any v) -> bool {
  return p_->UpsertRTValue(n, k, v);
}

auto ModuleManager::RetrieveRTValue(Namespace n, Key k)
    -> std::optional<std::any> {
  return p_->RetrieveRTValue(n, k);
}

auto ModuleManager::ListenRTPublish(QObject* o, Namespace n, Key k,
                                    LPCallback c) -> bool {
  return p_->ListenPublish(o, n, k, c);
}

auto ModuleManager::ListRTChildKeys(const QString& n, const QString& k)
    -> std::vector<Key> {
  return p_->ListRTChildKeys(n, k);
}

auto ModuleManager::IsModuleActivated(ModuleIdentifier id) -> bool {
  return p_->IsModuleActivated(id);
}

}  // namespace GpgFrontend::Module