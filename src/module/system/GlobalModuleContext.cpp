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

#include "core/thread/Task.h"
#include "module/system/Event.h"
#include "module/system/Module.h"

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

  int GetChannel(ModulePtr plugin) {
    // Search for the plugin in the register table.
    auto plugin_info_opt =
        search_plugin_register_table(plugin->GetPluginIdentifier());
    if (!plugin_info_opt.has_value()) {
      SPDLOG_ERROR(
          "cannot find plugin id {} at register table, fallbacking to "
          "default "
          "channel",
          plugin->GetPluginIdentifier());
      return GetDefaultChannel(plugin);
    }

    auto plugin_info = plugin_info_opt.value();
    return plugin_info->channel;
  }

  int GetDefaultChannel(ModulePtr) { return GPGFRONTEND_DEFAULT_CHANNEL; }

  std::optional<TaskRunnerPtr> GetTaskRunner(ModulePtr plugin) {
    auto opt = search_plugin_register_table(plugin->GetPluginIdentifier());
    if (!opt.has_value()) {
      return std::nullopt;
    }
    return opt.value()->task_runner;
  }

  std::optional<TaskRunnerPtr> GetTaskRunner(ModuleIdentifier plugin_id) {
    // Search for the plugin in the register table.
    auto plugin_info_opt = search_plugin_register_table(plugin_id);
    if (!plugin_info_opt.has_value()) {
      SPDLOG_ERROR("cannot find plugin id {} at register table", plugin_id);
      return std::nullopt;
    }
    return plugin_info_opt.value()->task_runner;
  }

  std::optional<TaskRunnerPtr> GetGlobalTaskRunner() {
    return default_task_runner_;
  }

  bool RegisterPlugin(ModulePtr plugin) {
    SPDLOG_DEBUG("attempting to register plugin: {}",
                 plugin->GetPluginIdentifier());
    // Check if the plugin is null or already registered.
    if (plugin == nullptr ||
        plugin_register_table_.find(plugin->GetPluginIdentifier()) !=
            plugin_register_table_.end()) {
      SPDLOG_ERROR("plugin is null or have already registered this plugin");
      return false;
    }

    if (!plugin->Register()) {
      SPDLOG_ERROR("register plugin {} failed", plugin->GetPluginIdentifier());
      return false;
    }

    PluginRegisterInfo register_info;
    register_info.plugin = plugin;
    register_info.channel = acquire_new_unique_channel();
    register_info.task_runner = std::make_shared<Thread::TaskRunner>();

    // Register the plugin with its identifier.
    plugin_register_table_[plugin->GetPluginIdentifier()] =
        std::make_shared<PluginRegisterInfo>(std::move(register_info));

    SPDLOG_DEBUG("successfully registered plugin: {}",
                 plugin->GetPluginIdentifier());
    return true;
  }

  bool ActivePlugin(ModuleIdentifier plugin_id) {
    SPDLOG_DEBUG("attempting to activate plugin: {}", plugin_id);

    // Search for the plugin in the register table.
    auto plugin_info_opt = search_plugin_register_table(plugin_id);
    if (!plugin_info_opt.has_value()) {
      SPDLOG_ERROR("cannot find plugin id {} at register table", plugin_id);
      return false;
    }

    auto plugin_info = plugin_info_opt.value();
    // Activate the plugin if it is not already active.
    if (plugin_info->activate && plugin_info->plugin->Active()) {
      plugin_info->activate = true;
    }

    SPDLOG_DEBUG("plugin activation status: {}", plugin_info->activate);
    return plugin_info->activate;
  }

  bool ListenEvent(ModuleIdentifier plugin_id, EventIdentifier event) {
    SPDLOG_DEBUG("plugin: {} is attempting to listen to event {}", plugin_id,
                 event);
    // Check if the event exists, if not, create it.
    auto it = plugin_events_table_.find(event);
    if (it == plugin_events_table_.end()) {
      plugin_events_table_[event] = std::unordered_set<ModuleIdentifier>();
      it = plugin_events_table_.find(event);
      SPDLOG_INFO("new event {} of plugin system created", event);
    }

    auto& listeners_set = it->second;
    // Add the listener (plugin) to the event.
    auto listener_it =
        std::find(listeners_set.begin(), listeners_set.end(), plugin_id);
    if (listener_it == listeners_set.end()) {
      listeners_set.insert(plugin_id);
    }
    return true;
  }

  bool DeactivatePlugin(ModuleIdentifier plugin_id) {
    // Search for the plugin in the register table.
    auto plugin_info_opt = search_plugin_register_table(plugin_id);
    if (!plugin_info_opt.has_value()) {
      SPDLOG_ERROR("cannot find plugin id {} at register table", plugin_id);
      return false;
    }

    auto plugin_info = plugin_info_opt.value();
    // Activate the plugin if it is not already deactive.
    if (!plugin_info->activate && plugin_info->plugin->Deactive()) {
      plugin_info->activate = false;
    }

    return !plugin_info->activate;
  }

  bool TriggerEvent(EventRefrernce event) {
    SPDLOG_DEBUG("attempting to trigger event: {}", event->GetIdentifier());

    // Find the set of listeners associated with the given event in the table
    auto it = plugin_events_table_.find(event->GetIdentifier());
    if (it == plugin_events_table_.end()) {
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

    // Iterate through each listener and execute the corresponding plugin
    for (auto& listener_plugin_id : listeners_set) {
      // Search for the plugin's information in the registration table
      auto plugin_info_opt = search_plugin_register_table(listener_plugin_id);

      // Log an error if the plugin is not found in the registration table
      if (!plugin_info_opt.has_value()) {
        SPDLOG_ERROR("cannot find plugin id {} at register table",
                     listener_plugin_id);
      }

      // Retrieve the plugin's information
      auto plugin_info = plugin_info_opt.value();

      // Check if the plugin is activated
      if (!plugin_info->activate) continue;

      // Execute the plugin and check if it fails
      if (plugin_info->plugin->Exec(event)) {
        // Log an error if the plugin execution fails
        SPDLOG_ERROR("plugin {} executed failed", listener_plugin_id);
      }
    }

    // Return true to indicate successful execution of all plugins
    return true;
  }

 private:
  struct PluginRegisterInfo {
    int channel;
    TaskRunnerPtr task_runner;
    ModulePtr plugin;
    bool activate;
  };

  using PluginRegisterInfoPtr = std::shared_ptr<PluginRegisterInfo>;

  std::unordered_map<ModuleIdentifier, PluginRegisterInfoPtr>
      plugin_register_table_;
  std::map<EventIdentifier, std::unordered_set<ModuleIdentifier>>
      plugin_events_table_;

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

  // Function to search for a plugin in the register table.
  std::optional<PluginRegisterInfoPtr> search_plugin_register_table(
      ModuleIdentifier identifier) {
    auto it = plugin_register_table_.find(identifier);
    if (it == plugin_register_table_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  std::list<ModuleIdentifier>& search_plugin_events_table(ModuleIdentifier);
};

// Constructor for GlobalPluginContext, takes a TaskRunnerPtr as an argument.
GlobalModuleContext::GlobalModuleContext(TaskRunnerPtr task_runner)
    : p_(std::make_unique<Impl>(task_runner)) {}

GlobalModuleContext::~GlobalModuleContext() = default;

// Function to get the task runner associated with a plugin.
std::optional<TaskRunnerPtr> GlobalModuleContext::GetTaskRunner(
    ModulePtr plugin) {
  return p_->GetTaskRunner(plugin);
}

// Function to get the task runner associated with a plugin.
std::optional<TaskRunnerPtr> GlobalModuleContext::GetTaskRunner(
    ModuleIdentifier plugin_id) {
  return p_->GetTaskRunner(plugin_id);
}

// Function to get the global task runner.
std::optional<TaskRunnerPtr> GlobalModuleContext::GetGlobalTaskRunner() {
  return p_->GetGlobalTaskRunner();
}

bool GlobalModuleContext::RegisterPlugin(ModulePtr plugin) {
  return p_->RegisterPlugin(plugin);
}

bool GlobalModuleContext::ActivePlugin(ModuleIdentifier plugin_id) {
  return p_->ActivePlugin(plugin_id);
}

bool GlobalModuleContext::ListenEvent(ModuleIdentifier plugin_id,
                                      EventIdentifier event) {
  return p_->ListenEvent(plugin_id, event);
}

bool GlobalModuleContext::DeactivatePlugin(ModuleIdentifier plugin_id) {
  return p_->DeactivatePlugin(plugin_id);
}

bool GlobalModuleContext::TriggerEvent(EventRefrernce event) {
  return p_->TriggerEvent(event);
}

int GlobalModuleContext::GetChannel(ModulePtr plugin) {
  return p_->GetChannel(plugin);
}

int GlobalModuleContext::GetDefaultChannel(ModulePtr _) {
  return p_->GetDefaultChannel(_);
}

}  // namespace GpgFrontend::Module
