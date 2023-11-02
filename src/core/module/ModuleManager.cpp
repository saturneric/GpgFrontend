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

#include <boost/format.hpp>
#include <utility>

#include "core/module/GlobalModuleContext.h"
#include "core/module/GlobalRegisterTable.h"
#include "core/module/Module.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Module {

ModuleMangerPtr ModuleManager::g_ = nullptr;

class ModuleManager::Impl {
 public:
  Impl()
      : task_runner_(std::make_shared<Thread::TaskRunner>()),
        gmc_(std::make_shared<GlobalModuleContext>(task_runner_)),
        grt_(std::make_shared<GlobalRegisterTable>()) {
    task_runner_->Start();
  }

  void RegisterModule(const ModulePtr& module) {
    task_runner_->PostTask(new Thread::Task(
        [=](GpgFrontend::DataObjectPtr) -> int {
          module->SetGPC(gmc_);
          gmc_->RegisterModule(module);
          return 0;
        },
        __func__, nullptr));
  }

  void TriggerEvent(const EventRefrernce& event) {
    task_runner_->PostTask(new Thread::Task(
        [=](const GpgFrontend::DataObjectPtr&) -> int {
          gmc_->TriggerEvent(event);
          return 0;
        },
        __func__, nullptr));
  }

  void ActiveModule(const ModuleIdentifier& identifier) {
    task_runner_->PostTask(new Thread::Task(
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

 private:
  static ModuleMangerPtr global_module_manager;
  TaskRunnerPtr task_runner_;
  GMCPtr gmc_;
  GRTPtr grt_;
};

auto UpsertRTValue(const std::string& namespace_, const std::string& key,
                   const std::any& value) -> bool {
  return ModuleManager::GetInstance()->UpsertRTValue(namespace_, key,
                                                     std::any(value));
}

auto GPGFRONTEND_CORE_EXPORT ListenRTPublishEvent(QObject* o, Namespace n,
                                                  Key k, LPCallback c) -> bool {
  return ModuleManager::GetInstance()->ListenRTPublish(o, n, k, c);
}

ModuleManager::ModuleManager() : p_(std::make_unique<Impl>()) {}

ModuleManager::~ModuleManager() = default;

auto ModuleManager::GetInstance() -> ModuleMangerPtr {
  if (g_ == nullptr) g_ = std::shared_ptr<ModuleManager>(new ModuleManager());
  return g_;
}

void ModuleManager::RegisterModule(ModulePtr module) {
  return p_->RegisterModule(module);
}

void ModuleManager::TriggerEvent(EventRefrernce event) {
  return p_->TriggerEvent(event);
}

void ModuleManager::ActiveModule(ModuleIdentifier identifier) {
  return p_->ActiveModule(identifier);
}

auto ModuleManager::GetTaskRunner(ModuleIdentifier module_id)
    -> std::optional<TaskRunnerPtr> {
  return p_->GetTaskRunner(std::move(module_id));
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

auto GetRealModuleIdentifier(const ModuleIdentifier& m_id) -> ModuleIdentifier {
  // WARNING: when YOU need to CHANGE this line, YOU SHOULD change the same code
  // in Module.cpp as well.
  return (boost::format("__module_%1%") % m_id).str();
}

}  // namespace GpgFrontend::Module