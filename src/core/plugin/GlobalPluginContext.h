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
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_GLOBALPLUGINCONTEXT_H
#define GPGFRONTEND_GLOBALPLUGINCONTEXT_H

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/GpgFrontendCore.h"
#include "core/plugin/Event.h"
#include "core/plugin/Plugin.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Plugin {

class Plugin;
class PluginManager;

using PluginList = std::list<std::string>;

struct PluginRegisterInfo {
  int channel;
  TaskRunnerPtr task_runner;
  PluginPtr plugin;
  bool activate;
};

using PluginRegisterInfoPtr = std::shared_ptr<PluginRegisterInfo>;

class GPGFRONTEND_CORE_EXPORT GlobalPluginContext : public QObject {
  Q_OBJECT
 public:
  GlobalPluginContext(TaskRunnerPtr);

  int GetChannel(PluginPtr);

  int GetDefaultChannel(PluginPtr);

  std::optional<TaskRunnerPtr> GetTaskRunner(PluginPtr);

  std::optional<TaskRunnerPtr> GetTaskRunner(PluginIdentifier plugin);

  std::optional<TaskRunnerPtr> GetGlobalTaskRunner();

  bool RegisterPlugin(PluginPtr);

  bool ActivePlugin(PluginIdentifier);

  bool DeactivatePlugin(PluginIdentifier);

  bool ListenEvent(PluginIdentifier, EventIdentifier);

  bool TriggerEvent(EventRefrernce);

 private:
  std::unordered_map<PluginIdentifier, PluginRegisterInfoPtr>
      plugin_register_table_;
  std::map<EventIdentifier, std::unordered_set<PluginIdentifier>>
      plugin_events_table_;

  std::set<int> acquired_channel_;
  boost::random::mt19937 random_gen_;
  TaskRunnerPtr default_task_runner_;

  int acquire_new_unique_channel();

  std::optional<PluginRegisterInfoPtr> search_plugin_register_table(
      PluginIdentifier);

  std::list<PluginIdentifier> &search_plugin_events_table(PluginIdentifier);
};

}  // namespace GpgFrontend::Plugin

#endif  // GPGFRONTEND_GLOBALPLUGINCONTEXT_H