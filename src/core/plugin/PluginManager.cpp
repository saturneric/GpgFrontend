/**
 * Copyright (C) 2021 Saturneric
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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "PluginManager.h"

#include <memory>

#include "core/plugin/Event.h"
#include "core/plugin/GlobalPluginContext.h"
#include "core/plugin/Plugin.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Plugin {

PluginMangerPtr PluginManager::global_plugin_manager_ = nullptr;

PluginManager::PluginManager()
    : task_runner_(std::make_shared<Thread::TaskRunner>()),
      global_plugin_context_(
          std::make_shared<GlobalPluginContext>(task_runner_)) {}

PluginMangerPtr PluginManager::GetInstance() {
  if (global_plugin_manager_ == nullptr) {
    global_plugin_manager_ =
        std::shared_ptr<PluginManager>(new PluginManager());
  }
  return global_plugin_manager_;
}

void PluginManager::RegisterPlugin(PluginPtr plugin) {
  task_runner_->PostTask(new Thread::Task(
      std::move([=](GpgFrontend::Thread::Task::DataObjectPtr) -> int {
        global_plugin_context_->RegisterPlugin(plugin);
        return 0;
      }),
      __func__, nullptr, true));
}

void PluginManager::TriggerEvent(EventRefrernce event) {
  task_runner_->PostTask(new Thread::Task(
      std::move([=](GpgFrontend::Thread::Task::DataObjectPtr) -> int {
        global_plugin_context_->TriggerEvent(event);
        return 0;
      }),
      __func__, nullptr, true));
}

void PluginManager::ActivePlugin(PluginIdentifier identifier) {
  task_runner_->PostTask(new Thread::Task(
      std::move([=](GpgFrontend::Thread::Task::DataObjectPtr) -> int {
        global_plugin_context_->ActivePlugin(identifier);
        return 0;
      }),
      __func__, nullptr, true));
}

std::optional<TaskRunnerPtr> PluginManager::GetTaskRunner(
    PluginIdentifier plugin_id) {
  return global_plugin_context_->GetTaskRunner(plugin_id);
}

}  // namespace GpgFrontend::Plugin