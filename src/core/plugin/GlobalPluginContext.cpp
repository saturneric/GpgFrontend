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

#include "GlobalPluginContext.h"

#include <memory>
#include <optional>
#include <unordered_set>

#include "GpgConstants.h"
#include "plugin/Event.h"
#include "plugin/Plugin.h"
#include "spdlog/spdlog.h"
#include "thread/Task.h"

namespace GpgFrontend::Plugin {

// Constructor for GlobalPluginContext, takes a TaskRunnerPtr as an argument.
GlobalPluginContext::GlobalPluginContext(TaskRunnerPtr task_runner)
    : default_task_runner_(task_runner),
      random_gen_((boost::posix_time::microsec_clock::universal_time() -
                   boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1)))
                      .total_milliseconds()) {
  // Initialize acquired channels with default values.
  acquired_channel_.insert(GPGFRONTEND_DEFAULT_CHANNEL);
  acquired_channel_.insert(GPGFRONTEND_NON_ASCII_CHANNEL);
}

// Function to acquire a new unique channel.
int GlobalPluginContext::acquire_new_unique_channel() {
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
std::optional<PluginRegisterInfoPtr>
GlobalPluginContext::search_plugin_register_table(PluginIdentifier identifier) {
  auto it = plugin_register_table_.find(identifier);
  if (it == plugin_register_table_.end()) {
    return std::nullopt;
  }
  return it->second;
}

// Function to get the task runner associated with a plugin.
std::optional<TaskRunnerPtr> GlobalPluginContext::GetTaskRunner(
    PluginPtr plugin) {
  auto opt = search_plugin_register_table(plugin->gpc_get_identifier());
  if (!opt.has_value()) {
    return std::nullopt;
  }
  return opt.value()->task_runner;
}

// Function to get the task runner associated with a plugin.
std::optional<TaskRunnerPtr> GlobalPluginContext::GetTaskRunner(
    PluginIdentifier plugin) {
  // Search for the plugin in the register table.
  auto plugin_info_opt = search_plugin_register_table(plugin);
  if (!plugin_info_opt.has_value()) {
    SPDLOG_ERROR("cannot find plugin id {} at register table", plugin);
    return std::nullopt;
  }
  return plugin_info_opt.value()->task_runner;
}

// Function to get the global task runner.
std::optional<TaskRunnerPtr> GlobalPluginContext::GetGlobalTaskRunner() {
  return default_task_runner_;
}

bool GlobalPluginContext::RegisterPlugin(PluginPtr plugin) {
  SPDLOG_DEBUG("attempting to register plugin: {}",
               plugin->gpc_get_identifier());
  // Check if the plugin is null or already registered.
  if (plugin == nullptr ||
      plugin_register_table_.find(plugin->gpc_get_identifier()) !=
          plugin_register_table_.end()) {
    SPDLOG_ERROR("plugin is null or have already registered this plugin");
    return false;
  }

  PluginRegisterInfo register_info;
  register_info.plugin = plugin;
  register_info.channel = acquire_new_unique_channel();
  register_info.task_runner = std::make_shared<Thread::TaskRunner>();

  // Register the plugin with its identifier.
  plugin_register_table_[plugin->gpc_get_identifier()] =
      std::make_shared<PluginRegisterInfo>(std::move(register_info));

  SPDLOG_DEBUG("successfully registered plugin: {}",
               plugin->gpc_get_identifier());
  return true;
}

bool GlobalPluginContext::ActivePlugin(PluginIdentifier plugin_id) {
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

bool GlobalPluginContext::ListenEvent(PluginIdentifier plugin_id,
                                      EventIdentifier event) {
  SPDLOG_DEBUG("plugin: {} is attempting to listen to event {}", plugin_id,
               event);
  // Check if the event exists, if not, create it.
  auto it = plugin_events_table_.find(event);
  if (it == plugin_events_table_.end()) {
    plugin_events_table_[event] = std::unordered_set<PluginIdentifier>();
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

bool GlobalPluginContext::DeactivatePlugin(PluginIdentifier plugin_id) {
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

bool GlobalPluginContext::TriggerEvent(EventRefrernce event) {
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
    SPDLOG_WARN("event {} is not listening by anyone", event->GetIdentifier());
    return false;
  }

  // Log the number of listeners for this event
  SPDLOG_DEBUG("event {}'s current listeners size: {}", event->GetIdentifier(),
               listeners_set.size());

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

int GlobalPluginContext::GetChannel(PluginPtr plugin) {
  // Search for the plugin in the register table.
  auto plugin_info_opt =
      search_plugin_register_table(plugin->gpc_get_identifier());
  if (!plugin_info_opt.has_value()) {
    SPDLOG_ERROR(
        "cannot find plugin id {} at register table, fallbacking to default "
        "channel",
        plugin->gpc_get_identifier());
    return GetDefaultChannel(plugin);
  }

  auto plugin_info = plugin_info_opt.value();
  return plugin_info->channel;
}

int GlobalPluginContext::GetDefaultChannel(PluginPtr) {
  return GPGFRONTEND_DEFAULT_CHANNEL;
}

}  // namespace GpgFrontend::Plugin
