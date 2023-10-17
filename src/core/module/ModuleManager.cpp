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

#include "core/module/GlobalModuleContext.h"
#include "core/module/Module.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Module {

ModuleMangerPtr ModuleManager::g_ = nullptr;

class ModuleManager::Impl {
 public:
  Impl()
      : task_runner_(std::make_shared<Thread::TaskRunner>()),
        gpc_(std::make_shared<GlobalModuleContext>(task_runner_)) {
    task_runner_->start();
  }

  void RegisterModule(ModulePtr module) {
    task_runner_->PostTask(new Thread::Task(
        std::move([=](GpgFrontend::Thread::DataObjectPtr) -> int {
          module->SetGPC(gpc_);
          gpc_->RegisterModule(module);
          return 0;
        }),
        __func__, nullptr, true));
  }

  void TriggerEvent(EventRefrernce event) {
    task_runner_->PostTask(new Thread::Task(
        std::move([=](GpgFrontend::Thread::DataObjectPtr) -> int {
          gpc_->TriggerEvent(event);
          return 0;
        }),
        __func__, nullptr, true));
  }

  void ActiveModule(ModuleIdentifier identifier) {
    task_runner_->PostTask(new Thread::Task(
        std::move([=](GpgFrontend::Thread::DataObjectPtr) -> int {
          gpc_->ActiveModule(identifier);
          return 0;
        }),
        __func__, nullptr, true));
  }

  std::optional<TaskRunnerPtr> GetTaskRunner(ModuleIdentifier module_id) {
    return gpc_->GetTaskRunner(module_id);
  }

 private:
  static ModuleMangerPtr global_module_manager_;
  TaskRunnerPtr task_runner_;
  GlobalModuleContextPtr gpc_;
};

ModuleManager::ModuleManager() : p_(std::make_unique<Impl>()) {}

ModuleManager::~ModuleManager() = default;

ModuleMangerPtr ModuleManager::GetInstance() {
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

std::optional<TaskRunnerPtr> ModuleManager::GetTaskRunner(
    ModuleIdentifier module_id) {
  return p_->GetTaskRunner(module_id);
}

}  // namespace GpgFrontend::Module