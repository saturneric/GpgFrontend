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

#include "GlobalModuleContext.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/module/Event.h"
#include "core/module/Module.h"
#include "core/thread/Task.h"

namespace GpgFrontend::Module {

class GlobalModuleContext::Impl {
 public:
  Impl(TaskRunnerPtr task_runner)
      : default_task_runner_(task_runner),
        random_gen_(
            (boost::posix_time::microsec_clock::universal_time() -
             boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1)))
                .total_milliseconds()) {
    // Initialize acquired channels with default values.
    acquired_channel_.insert(GPGFRONTEND_DEFAULT_CHANNEL);
    acquired_channel_.insert(GPGFRONTEND_NON_ASCII_CHANNEL);
  }

  int GetChannel(ModulePtr module) {
    // Search for the module in the register table.
    auto module_info_opt =
        search_module_register_table(module->GetModuleIdentifier());
    if (!module_info_opt.has_value()) {
      SPDLOG_ERROR(
          "cannot find module id {} at register table, fallbacking to "
          "default "
          "channel",
          module->GetModuleIdentifier());
      return GetDefaultChannel(module);
    }

    auto module_info = module_info_opt.value();
    return module_info->channel;
  }

  int GetDefaultChannel(ModulePtr) { return GPGFRONTEND_DEFAULT_CHANNEL; }

  std::optional<TaskRunnerPtr> GetTaskRunner(ModulePtr module) {
    auto opt = search_module_register_table(module->GetModuleIdentifier());
    if (!opt.has_value()) {
      return std::nullopt;
    }
    return opt.value()->task_runner;
  }

  std::optional<TaskRunnerPtr> GetTaskRunner(ModuleIdentifier module_id) {
    // Search for the module in the register table.
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      SPDLOG_ERROR("cannot find module id {} at register table", module_id);
      return std::nullopt;
    }
    return module_info_opt.value()->task_runner;
  }

  std::optional<TaskRunnerPtr> GetGlobalTaskRunner() {
    return default_task_runner_;
  }

  bool RegisterModule(ModulePtr module) {
    SPDLOG_DEBUG("attempting to register module: {}",
                 module->GetModuleIdentifier());
    // Check if the module is null or already registered.
    if (module == nullptr ||
        module_register_table_.find(module->GetModuleIdentifier()) !=
            module_register_table_.end()) {
      SPDLOG_ERROR("module is null or have already registered this module");
      return false;
    }

    if (!module->Register()) {
      SPDLOG_ERROR("register module {} failed", module->GetModuleIdentifier());
      return false;
    }

    ModuleRegisterInfo register_info;
    register_info.module = module;
    register_info.channel = acquire_new_unique_channel();
    register_info.task_runner = std::make_shared<Thread::TaskRunner>();

    // Register the module with its identifier.
    module_register_table_[module->GetModuleIdentifier()] =
        std::make_shared<ModuleRegisterInfo>(std::move(register_info));

    SPDLOG_DEBUG("successfully registered module: {}",
                 module->GetModuleIdentifier());
    return true;
  }

  bool ActiveModule(ModuleIdentifier module_id) {
    SPDLOG_DEBUG("attempting to activate module: {}", module_id);

    // Search for the module in the register table.
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      SPDLOG_ERROR("cannot find module id {} at register table", module_id);
      return false;
    }

    auto module_info = module_info_opt.value();
    // Activate the module if it is not already active.
    if (module_info->activate && module_info->module->Active()) {
      module_info->activate = true;
    }

    SPDLOG_DEBUG("module activation status: {}", module_info->activate);
    return module_info->activate;
  }

  bool ListenEvent(ModuleIdentifier module_id, EventIdentifier event) {
    SPDLOG_DEBUG("module: {} is attempting to listen to event {}", module_id,
                 event);
    // Check if the event exists, if not, create it.
    auto it = module_events_table_.find(event);
    if (it == module_events_table_.end()) {
      module_events_table_[event] = std::unordered_set<ModuleIdentifier>();
      it = module_events_table_.find(event);
      SPDLOG_INFO("new event {} of module system created", event);
    }

    auto& listeners_set = it->second;
    // Add the listener (module) to the event.
    auto listener_it =
        std::find(listeners_set.begin(), listeners_set.end(), module_id);
    if (listener_it == listeners_set.end()) {
      listeners_set.insert(module_id);
    }
    return true;
  }

  bool DeactivateModule(ModuleIdentifier module_id) {
    // Search for the module in the register table.
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      SPDLOG_ERROR("cannot find module id {} at register table", module_id);
      return false;
    }

    auto module_info = module_info_opt.value();
    // Activate the module if it is not already deactive.
    if (!module_info->activate && module_info->module->Deactive()) {
      module_info->activate = false;
    }

    return !module_info->activate;
  }

  bool TriggerEvent(EventRefrernce event) {
    SPDLOG_DEBUG("attempting to trigger event: {}", event->GetIdentifier());

    // Find the set of listeners associated with the given event in the table
    auto it = module_events_table_.find(event->GetIdentifier());
    if (it == module_events_table_.end()) {
      // Log a warning if the event is not registered and nobody is listening
      SPDLOG_WARN(
          "event {} is not listening by anyone and not registered as well",
          event->GetIdentifier());
      return false;
    }

    // Retrieve the set of listeners for this event
    auto& listeners_set = it->second;

    // Check if the set of listeners is empty
    if (listeners_set.empty()) {
      // Log a warning if nobody is listening to this event
      SPDLOG_WARN("event {} is not listening by anyone",
                  event->GetIdentifier());
      return false;
    }

    // Log the number of listeners for this event
    SPDLOG_DEBUG("event {}'s current listeners size: {}",
                 event->GetIdentifier(), listeners_set.size());

    // Iterate through each listener and execute the corresponding module
    for (auto& listener_module_id : listeners_set) {
      // Search for the module's information in the registration table
      auto module_info_opt = search_module_register_table(listener_module_id);

      // Log an error if the module is not found in the registration table
      if (!module_info_opt.has_value()) {
        SPDLOG_ERROR("cannot find module id {} at register table",
                     listener_module_id);
      }

      // Retrieve the module's information
      auto module_info = module_info_opt.value();

      // Check if the module is activated
      if (!module_info->activate) continue;

      // Execute the module and check if it fails
      if (module_info->module->Exec(event)) {
        // Log an error if the module execution fails
        SPDLOG_ERROR("module {} executed failed", listener_module_id);
      }
    }

    // Return true to indicate successful execution of all modules
    return true;
  }

 private:
  struct ModuleRegisterInfo {
    int channel;
    TaskRunnerPtr task_runner;
    ModulePtr module;
    bool activate;
  };

  using ModuleRegisterInfoPtr = std::shared_ptr<ModuleRegisterInfo>;

  std::unordered_map<ModuleIdentifier, ModuleRegisterInfoPtr>
      module_register_table_;
  std::map<EventIdentifier, std::unordered_set<ModuleIdentifier>>
      module_events_table_;

  std::set<int> acquired_channel_;
  boost::random::mt19937 random_gen_;
  TaskRunnerPtr default_task_runner_;

  int acquire_new_unique_channel() {
    boost::random::uniform_int_distribution<> dist(1, 65535);

    int random_channel = dist(random_gen_);
    // Ensure the acquired channel is unique.
    while (acquired_channel_.find(random_channel) != acquired_channel_.end()) {
      random_channel = dist(random_gen_);
    }

    // Add the acquired channel to the set.
    acquired_channel_.insert(random_channel);
    return random_channel;
  }

  // Function to search for a module in the register table.
  std::optional<ModuleRegisterInfoPtr> search_module_register_table(
      ModuleIdentifier identifier) {
    auto it = module_register_table_.find(identifier);
    if (it == module_register_table_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  std::list<ModuleIdentifier>& search_module_events_table(ModuleIdentifier);
};

// Constructor for GlobalModuleContext, takes a TaskRunnerPtr as an argument.
GlobalModuleContext::GlobalModuleContext(TaskRunnerPtr task_runner)
    : p_(std::make_unique<Impl>(task_runner)) {}

GlobalModuleContext::~GlobalModuleContext() = default;

// Function to get the task runner associated with a module.
std::optional<TaskRunnerPtr> GlobalModuleContext::GetTaskRunner(
    ModulePtr module) {
  return p_->GetTaskRunner(module);
}

// Function to get the task runner associated with a module.
std::optional<TaskRunnerPtr> GlobalModuleContext::GetTaskRunner(
    ModuleIdentifier module_id) {
  return p_->GetTaskRunner(module_id);
}

// Function to get the global task runner.
std::optional<TaskRunnerPtr> GlobalModuleContext::GetGlobalTaskRunner() {
  return p_->GetGlobalTaskRunner();
}

bool GlobalModuleContext::RegisterModule(ModulePtr module) {
  return p_->RegisterModule(module);
}

bool GlobalModuleContext::ActiveModule(ModuleIdentifier module_id) {
  return p_->ActiveModule(module_id);
}

bool GlobalModuleContext::ListenEvent(ModuleIdentifier module_id,
                                      EventIdentifier event) {
  return p_->ListenEvent(module_id, event);
}

bool GlobalModuleContext::DeactivateModule(ModuleIdentifier module_id) {
  return p_->DeactivateModule(module_id);
}

bool GlobalModuleContext::TriggerEvent(EventRefrernce event) {
  return p_->TriggerEvent(event);
}

int GlobalModuleContext::GetChannel(ModulePtr module) {
  return p_->GetChannel(module);
}

int GlobalModuleContext::GetDefaultChannel(ModulePtr _) {
  return p_->GetDefaultChannel(_);
}

}  // namespace GpgFrontend::Module
