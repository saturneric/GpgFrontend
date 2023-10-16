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

#include "PluginManager.h"

#include "core/thread/TaskRunner.h"
#include "plugin/system/GlobalPluginContext.h"
#include "plugin/system/Plugin.h"

namespace GpgFrontend::Plugin {

PluginMangerPtr PluginManager::g_ = nullptr;

class PluginManager::Impl {
 public:
  Impl()
      : task_runner_(std::make_shared<Thread::TaskRunner>()),
        global_plugin_context_(
            std::make_shared<GlobalPluginContext>(task_runner_)) {}

  void RegisterPlugin(PluginPtr plugin) {
    task_runner_->PostTask(new Thread::Task(
        std::move([=](GpgFrontend::Thread::DataObjectPtr) -> int {
          global_plugin_context_->RegisterPlugin(plugin);
          return 0;
        }),
        __func__, nullptr, true));
  }

  void TriggerEvent(EventRefrernce event) {
    task_runner_->PostTask(new Thread::Task(
        std::move([=](GpgFrontend::Thread::DataObjectPtr) -> int {
          global_plugin_context_->TriggerEvent(event);
          return 0;
        }),
        __func__, nullptr, true));
  }

  void ActivePlugin(PluginIdentifier identifier) {
    task_runner_->PostTask(new Thread::Task(
        std::move([=](GpgFrontend::Thread::DataObjectPtr) -> int {
          global_plugin_context_->ActivePlugin(identifier);
          return 0;
        }),
        __func__, nullptr, true));
  }

  std::optional<TaskRunnerPtr> GetTaskRunner(PluginIdentifier plugin_id) {
    return global_plugin_context_->GetTaskRunner(plugin_id);
  }

 private:
  static PluginMangerPtr global_plugin_manager_;
  TaskRunnerPtr task_runner_;
  GlobalPluginContextPtr global_plugin_context_;
};

PluginManager::PluginManager() : p_(std::make_unique<Impl>()) {}

PluginManager::~PluginManager() = default;

PluginMangerPtr PluginManager::GetInstance() {
  if (g_ == nullptr) g_ = std::shared_ptr<PluginManager>(new PluginManager());
  return g_;
}

void PluginManager::RegisterPlugin(PluginPtr plugin) {
  return p_->RegisterPlugin(plugin);
}

void PluginManager::TriggerEvent(EventRefrernce event) {
  return p_->TriggerEvent(event);
}

void PluginManager::ActivePlugin(PluginIdentifier identifier) {
  return p_->ActivePlugin(identifier);
}

std::optional<TaskRunnerPtr> PluginManager::GetTaskRunner(
    PluginIdentifier plugin_id) {
  return p_->GetTaskRunner(plugin_id);
}

}  // namespace GpgFrontend::Plugin