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

#include <boost/date_time.hpp>
#include <boost/format.hpp>
#include <boost/random.hpp>
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
  explicit Impl()
      : random_gen_(
            (boost::posix_time::microsec_clock::universal_time() -
             boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1)))
                .total_milliseconds()) {
    // Initialize acquired channels with default values.
    acquired_channel_.insert(kGpgFrontendDefaultChannel);
    acquired_channel_.insert(kGpgFrontendNonAsciiChannel);
  }

  auto GetChannel(ModuleRawPtr module) -> int {
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

  auto RegisterModule(const ModulePtr& module) -> bool {
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

    auto register_info =
        GpgFrontend::SecureCreateSharedObject<ModuleRegisterInfo>();
    register_info->module = module;
    register_info->channel = acquire_new_unique_channel();

    // move module to its task runner' thread
    register_info->module->setParent(nullptr);
    register_info->module->moveToThread(
        Thread::TaskRunnerGetter::GetInstance()
            .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
            ->GetThread());

    // Register the module with its identifier.
    module_register_table_[module->GetModuleIdentifier()] = register_info;

    SPDLOG_DEBUG("successfully registered module: {}",
                 module->GetModuleIdentifier());
    return true;
  }

  auto ActiveModule(ModuleIdentifier module_id) -> bool {
    SPDLOG_DEBUG("attempting to activate module: {}", module_id);

    // Search for the module in the register table.
    auto module_info_opt = search_module_register_table(module_id);
    if (!module_info_opt.has_value()) {
      SPDLOG_ERROR("cannot find module id {} at register table", module_id);
      return false;
    }

    auto module_info = module_info_opt.value();

    // try to get module from module info
    auto module = module_info->module;
    if (module == nullptr) {
      SPDLOG_ERROR(
          "module id {} at register table is releated to a null module",
          module_id);
      return false;
    }

    // Activate the module if it is not already active.
    if (!module_info->activate) {
      module->Active();
      module_info->activate = true;
    }

    SPDLOG_DEBUG("module activation status: {}", module_info->activate);
    return module_info->activate;
  }

  auto ListenEvent(ModuleIdentifier module_id, EventIdentifier event) -> bool {
    SPDLOG_DEBUG("module: {} is attempting to listen to event {}", module_id,
                 event);
    // Check if the event exists, if not, create it.
    auto met_it = module_events_table_.find(event);
    if (met_it == module_events_table_.end()) {
      module_events_table_[event] = std::unordered_set<ModuleIdentifier>();
      met_it = module_events_table_.find(event);
      SPDLOG_INFO("new event {} of module system created", event);
    }

    auto& listeners_set = met_it->second;
    // Add the listener (module) to the event.
    auto listener_it = listeners_set.find(module_id);
    if (listener_it == listeners_set.end()) {
      listeners_set.insert(module_id);
    }
    return true;
  }

  auto DeactivateModule(ModuleIdentifier module_id) -> bool {
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

  auto TriggerEvent(const EventRefrernce& event) -> bool {
    auto event_id = event->GetIdentifier();
    SPDLOG_DEBUG("attempting to trigger event: {}", event_id);

    // Find the set of listeners associated with the given event in the table
    auto met_it = module_events_table_.find(event_id);
    if (met_it == module_events_table_.end()) {
      // Log a warning if the event is not registered and nobody is listening
      SPDLOG_WARN(
          "event {} is not listening by anyone and not registered as well",
          event_id);
      return false;
    }

    // Retrieve the set of listeners for this event
    auto& listeners_set = met_it->second;

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
    for (const auto& listener_module_id : listeners_set) {
      // Search for the module's information in the registration table
      auto module_info_opt = search_module_register_table(listener_module_id);

      // Log an error if the module is not found in the registration table
      if (!module_info_opt.has_value()) {
        SPDLOG_ERROR("cannot find module id {} at register table",
                     listener_module_id);
        continue;
      }

      // Retrieve the module's information
      auto module_info = module_info_opt.value();
      auto module = module_info->module;

      SPDLOG_DEBUG("module {} is listening to event {}, activate state: {}",
                   module_info->module->GetModuleIdentifier(),
                   event->GetIdentifier(), module_info->activate);

      // Check if the module is activated
      if (!module_info->activate) continue;

      Thread::Task::TaskRunnable exec_runnerable =
          [module, event](DataObjectPtr) -> int { return module->Exec(event); };

      Thread::Task::TaskCallback exec_callback = [listener_module_id, event_id](
                                                     int code, DataObjectPtr) {
        if (code < 0) {
          // Log an error if the module execution fails
          SPDLOG_ERROR(
              "module {} execution failed of event {}: exec return code {}",
              listener_module_id, event_id, code);
        }
      };

      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
          ->PostTask(
              new Thread::Task(exec_runnerable,
                               (boost::format("event/%1%/module/exec/%2%") %
                                event_id % listener_module_id)
                                   .str(),
                               nullptr, exec_callback));
    }

    // Return true to indicate successful execution of all modules
    return true;
  }

  auto IsModuleActivated(const ModuleIdentifier& m_id) const -> bool {
    auto m = search_module_register_table(m_id);
    return m.has_value() && m->get()->activate;
  }

 private:
  struct ModuleRegisterInfo {
    int channel;
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

  auto acquire_new_unique_channel() -> int {
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
  auto search_module_register_table(const ModuleIdentifier& identifier) const
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

auto GlobalModuleContext::RegisterModule(ModulePtr module) -> bool {
  return p_->RegisterModule(std::move(module));
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

auto GlobalModuleContext::TriggerEvent(EventRefrernce event) -> bool {
  return p_->TriggerEvent(std::move(event));
}

auto GlobalModuleContext::GetChannel(ModuleRawPtr module) -> int {
  return p_->GetChannel(module);
}

auto GlobalModuleContext::GetDefaultChannel(ModuleRawPtr channel) -> int {
  return GlobalModuleContext::Impl::GetDefaultChannel(channel);
}

auto GlobalModuleContext::IsModuleActivated(ModuleIdentifier m_id) -> bool {
  return p_->IsModuleActivated(std::move(m_id));
}

}  // namespace GpgFrontend::Module
