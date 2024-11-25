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

#include "GlobalModuleContext.h"

#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/module/Event.h"
#include "core/module/Module.h"
#include "core/thread/Task.h"
#include "model/DataObject.h"
#include "thread/TaskRunnerGetter.h"
#include "utils/MemoryUtils.h"

namespace GpgFrontend::Module {

class GlobalModuleContext::Impl {
 public:
  explicit Impl() {
    // Initialize acquired channels with default values.
    acquired_channel_.insert(kGpgFrontendDefaultChannel);
    acquired_channel_.insert(kGpgFrontendNonAsciiChannel);
  }

  auto SearchModule(ModuleIdentifier module_id) -> ModulePtr {
    // Search for the module in the register table.
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      LOG_W() << "cannot find module id " << module_id << " at register table";
      return nullptr;
    }

    return module_info_opt.value()->module;
  }

  auto GetChannel(ModuleRawPtr module) -> int {
    // Search for the module in the register table.
    auto module_info_opt =
        search_module_register_table(module->GetModuleIdentifier());
    if (!module_info_opt.has_value()) {
      LOG_W() << "cannot find module id " << module->GetModuleIdentifier()
              << " at register table, fallback to "
                 "default channel";

      return GetDefaultChannel(module);
    }

    auto module_info = module_info_opt.value();
    return module_info->channel;
  }

  static auto GetDefaultChannel(ModuleRawPtr) -> int {
    return kGpgFrontendDefaultChannel;
  }

  auto GetTaskRunner(ModuleRawPtr /*module*/) -> std::optional<TaskRunnerPtr> {
    return Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);
  }

  auto GetTaskRunner(ModuleIdentifier /*module_id*/)
      -> std::optional<TaskRunnerPtr> {
    return Thread::TaskRunnerGetter::GetInstance().GetTaskRunner(
        Thread::TaskRunnerGetter::kTaskRunnerType_Module);
  }

  auto GetGlobalTaskRunner() -> std::optional<TaskRunnerPtr> {
    return default_task_runner_;
  }

  auto RegisterModule(const ModulePtr& module, bool integrated_module) -> bool {
    // Check if the module is null or already registered.
    if (module == nullptr ||
        module_register_table_.find(module->GetModuleIdentifier()) !=
            module_register_table_.end()) {
      FLOG_W("module is null or have already registered this module");
      registered_modules_++;
      return false;
    }

    LOG_D() << "(+) module: " << module->GetModuleIdentifier()
            << "registering...";

    auto register_info =
        GpgFrontend::SecureCreateSharedObject<ModuleRegisterInfo>();
    register_info->module = module;
    register_info->channel = acquire_new_unique_channel();
    register_info->integrated = integrated_module;

    // move module to its task runner' thread
    register_info->module->setParent(nullptr);
    register_info->module->moveToThread(
        Thread::TaskRunnerGetter::GetInstance()
            .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
            ->GetThread());

    // register the module with its identifier.
    module_register_table_[module->GetModuleIdentifier()] = register_info;

    if (module->Register() != 0) {
      LOG_W() << "module: " << module->GetModuleIdentifier()
              << " register failed.";
      register_info->registered = false;
      registered_modules_++;
      return false;
    }

    register_info->registered = true;
    registered_modules_++;

    LOG_D() << "(+) module: " << module->GetModuleIdentifier() << "registered.";

    return true;
  }

  auto ActiveModule(ModuleIdentifier module_id) -> bool {
    LOG_D() << "(*) module: " << module_id << "activating...";

    // Search for the module in the register table.
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      LOG_W() << "cannot find module id " << module_id << " at register table";
      return false;
    }

    auto module_info = module_info_opt.value();

    if (!module_info->registered) {
      LOG_W() << "module id:" << module_id
              << " is not properly register, activation abort...";
      return false;
    }

    // try to get module from module info
    auto module = module_info->module;
    if (module == nullptr) {
      LOG_W() << "module id:" << module_id
              << " at register table is related to a null module";
      return false;
    }

    // Activate the module if it is not already active.
    if (!module_info->activate) {
      module->Active();
      module_info->activate = true;

      LOG_D() << "(*) module: " << module_id << "activated.";
    }

    return module_info->activate;
  }

  auto ListenEvent(ModuleIdentifier module_id, EventIdentifier event) -> bool {
    // module -> event
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      LOG_W() << "cannot find module id" << module_id << "at register table";
      return false;
    }

    // Check if the event exists, if not, create it.
    auto met_it = module_events_table_.find(event);
    if (met_it == module_events_table_.end()) {
      module_events_table_[event] = std::unordered_set<ModuleIdentifier>();
      met_it = module_events_table_.find(event);
    }

    module_info_opt.value()->listening_event_ids.push_back(event);

    auto& listeners_set = met_it->second;
    // Add the listener (module) to the event.
    auto listener_it = listeners_set.find(module_id);
    if (listener_it == listeners_set.end()) {
      listeners_set.insert(module_id);
    }
    return true;
  }

  auto DeactivateModule(ModuleIdentifier module_id) -> bool {
    // search for the module in the register table.
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      LOG_W() << "cannot find module id " << module_id << " at register table";
      return false;
    }

    auto module_info = module_info_opt.value();
    // activate the module if it is not already Deactivate.
    if (module_info->activate && (module_info->module->Deactivate() == 0)) {
      for (const auto& event_ids : module_info->listening_event_ids) {
        auto& modules = module_events_table_[event_ids];
        if (auto it = modules.find(module_id); it != modules.end()) {
          modules.erase(it);
        }
      }

      module_info->listening_event_ids.clear();
      module_info->activate = false;
    }

    return !module_info->activate;
  }

  auto TriggerEvent(const EventReference& event) -> bool {
    auto event_id = event->GetIdentifier();

    // Find the set of listeners associated with the given event in the table
    auto met_it = module_events_table_.find(event_id);
    if (met_it == module_events_table_.end()) {
      // Log a warning if the event is not registered and nobody is listening
      LOG_I() << "event: " << event_id
              << " is not listening by anyone and not registered as well.";
      return false;
    }

    // Retrieve the set of listeners for this event
    auto& listeners_set = met_it->second;

    // Check if the set of listeners is empty
    if (listeners_set.empty()) {
      // Log a warning if nobody is listening to this event
      LOG_I() << "event: " << event_id << " is not listening by anyone";
      return false;
    }

    // register trigger id index table
    module_on_triggering_events_table_[event->GetTriggerIdentifier()] = event;

    // Iterate through each listener and execute the corresponding module
    for (const auto& listener_module_id : listeners_set) {
      // Search for the module's information in the registration table
      auto module_info_opt = search_module_register_table(listener_module_id);

      // Log an error if the module is not found in the registration table
      if (!module_info_opt.has_value()) {
        LOG_W() << "cannot find module id: " << listener_module_id
                << " at register table";
        continue;
      }

      // Retrieve the module's information
      auto module_info = module_info_opt.value();
      auto module = module_info->module;

      // Check if the module is activated
      if (!module_info->activate) continue;

      Thread::Task::TaskRunnable const exec_runnerable =
          [module, event](DataObjectPtr) -> int { return module->Exec(event); };

      Thread::Task::TaskCallback const exec_callback =
          [listener_module_id, event_id](int code, DataObjectPtr) {
            if (code < 0) {
              // Log an error if the module execution fails
              LOG_W() << "module " << listener_module_id
                      << "execution failed of event " << event_id
                      << ": exec return code: " << code;
            }
          };

      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
          ->PostTask(new Thread::Task(exec_runnerable,
                                      QString("event/%1/module/exec/%2")
                                          .arg(event_id)
                                          .arg(listener_module_id),
                                      nullptr, exec_callback));
    }

    // Return true to indicate successful execution of all modules
    return true;
  }

  auto SearchEvent(const EventTriggerIdentifier& trigger_id)
      -> std::optional<EventReference> {
    if (module_on_triggering_events_table_.find(trigger_id) !=
        module_on_triggering_events_table_.end()) {
      return module_on_triggering_events_table_[trigger_id];
    }
    return {};
  }

  [[nodiscard]] auto IsModuleActivated(const ModuleIdentifier& m_id) const
      -> bool {
    auto m = search_module_register_table(m_id);
    return m.has_value() && m->get()->activate;
  }

  [[nodiscard]] auto IsModuleRegistered(const ModuleIdentifier& m_id) const
      -> bool {
    auto m = search_module_register_table(m_id);
    return m.has_value() && m->get()->activate;
  }

  auto IsIntegratedModule(ModuleIdentifier m_id) -> bool {
    auto m = search_module_register_table(m_id);
    return m.has_value() && m->get()->integrated;
  }

  auto ListAllRegisteredModuleID() -> QList<ModuleIdentifier> {
    QList<ModuleIdentifier> module_ids;
    for (const auto& module : module_register_table_) {
      module_ids.append(module.first);
    }
    module_ids.sort();
    return module_ids;
  }

  auto GetModuleListening(const ModuleIdentifier& module_id)
      -> QList<EventIdentifier> {
    auto module_info = search_module_register_table(module_id);
    if (!module_info.has_value()) return {};
    return module_info->get()->listening_event_ids;
  }

  [[nodiscard]] auto GetRegisteredModuleNum() const -> int {
    return registered_modules_;
  }

 private:
  struct ModuleRegisterInfo {
    int channel;
    ModulePtr module;
    bool registered;
    bool activate;
    bool integrated;
    QList<QString> listening_event_ids;
  };

  using ModuleRegisterInfoPtr = std::shared_ptr<ModuleRegisterInfo>;

  std::unordered_map<ModuleIdentifier, ModuleRegisterInfoPtr>
      module_register_table_;
  std::map<EventIdentifier, std::unordered_set<ModuleIdentifier>>
      module_events_table_;
  std::map<EventTriggerIdentifier, EventReference>
      module_on_triggering_events_table_;

  std::set<int> acquired_channel_;
  TaskRunnerPtr default_task_runner_;
  int registered_modules_ = 0;

  auto acquire_new_unique_channel() -> int {
    int random_channel = QRandomGenerator::global()->bounded(65535);
    // Ensure the acquired channel is unique.
    while (acquired_channel_.find(random_channel) != acquired_channel_.end()) {
      random_channel = QRandomGenerator::global()->bounded(65535);
    }

    // Add the acquired channel to the set.
    acquired_channel_.insert(random_channel);
    return random_channel;
  }

  // Function to search for a module in the register table.
  [[nodiscard]] auto search_module_register_table(
      const ModuleIdentifier& identifier) const
      -> std::optional<ModuleRegisterInfoPtr> {
    auto mrt_it = module_register_table_.find(identifier);
    if (mrt_it == module_register_table_.end()) {
      return std::nullopt;
    }
    return mrt_it->second;
  }
};

// Constructor for GlobalModuleContext, takes a TaskRunnerPtr as an argument.
GlobalModuleContext::GlobalModuleContext()
    : p_(SecureCreateUniqueObject<Impl>()) {}

GlobalModuleContext::~GlobalModuleContext() = default;

auto GlobalModuleContext::SearchModule(ModuleIdentifier module_id)
    -> ModulePtr {
  return p_->SearchModule(std::move(module_id));
}

// Function to get the task runner associated with a module.
auto GlobalModuleContext::GetTaskRunner(ModuleRawPtr module)
    -> std::optional<TaskRunnerPtr> {
  return p_->GetTaskRunner(module);
}

// Function to get the task runner associated with a module.
auto GlobalModuleContext::GetTaskRunner(ModuleIdentifier module_id)
    -> std::optional<TaskRunnerPtr> {
  return p_->GetTaskRunner(std::move(module_id));
}

// Function to get the global task runner.
auto GlobalModuleContext::GetGlobalTaskRunner()
    -> std::optional<TaskRunnerPtr> {
  return p_->GetGlobalTaskRunner();
}

auto GlobalModuleContext::RegisterModule(ModulePtr module,
                                         bool integrated_module) -> bool {
  return p_->RegisterModule(module, integrated_module);
}

auto GlobalModuleContext::ActiveModule(ModuleIdentifier module_id) -> bool {
  return p_->ActiveModule(std::move(module_id));
}

auto GlobalModuleContext::ListenEvent(ModuleIdentifier module_id,
                                      EventIdentifier event) -> bool {
  return p_->ListenEvent(std::move(module_id), std::move(event));
}

auto GlobalModuleContext::DeactivateModule(ModuleIdentifier module_id) -> bool {
  return p_->DeactivateModule(std::move(module_id));
}

auto GlobalModuleContext::TriggerEvent(EventReference event) -> bool {
  return p_->TriggerEvent(event);
}

auto GlobalModuleContext::SearchEvent(EventTriggerIdentifier trigger_id)
    -> std::optional<EventReference> {
  return p_->SearchEvent(trigger_id);
}

auto GlobalModuleContext::GetChannel(ModuleRawPtr module) -> int {
  return p_->GetChannel(module);
}

auto GlobalModuleContext::GetDefaultChannel(ModuleRawPtr channel) -> int {
  return GlobalModuleContext::Impl::GetDefaultChannel(channel);
}

auto GlobalModuleContext::IsModuleActivated(ModuleIdentifier m_id) -> bool {
  return p_->IsModuleActivated(m_id);
}

auto GlobalModuleContext::IsIntegratedModule(ModuleIdentifier m_id) -> bool {
  return p_->IsIntegratedModule(m_id);
}

auto GlobalModuleContext::ListAllRegisteredModuleID()
    -> QList<ModuleIdentifier> {
  return p_->ListAllRegisteredModuleID();
}

auto GlobalModuleContext::GetModuleListening(ModuleIdentifier module_id)
    -> QList<EventIdentifier> {
  return p_->GetModuleListening(module_id);
}

auto GlobalModuleContext::GetRegisteredModuleNum() const -> int {
  return p_->GetRegisteredModuleNum();
}

}  // namespace GpgFrontend::Module
