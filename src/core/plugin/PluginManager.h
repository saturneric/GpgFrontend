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

#ifndef GPGFRONTEND_PLUGINMANAGER_H
#define GPGFRONTEND_PLUGINMANAGER_H

#include <string>

#include "core/GpgFrontendCore.h"
#include "core/thread/Task.h"

namespace GpgFrontend::Thread {
class TaskRunner;
}

namespace GpgFrontend::Plugin {

using TaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class Event;
class Plugin;
class GlobalPluginContext;
class PluginManager;

using EventRefrernce = std::shared_ptr<Event>;
using PluginIdentifier = std::string;
using PluginPtr = std::shared_ptr<Plugin>;
using PluginMangerPtr = std::shared_ptr<PluginManager>;
using GlobalPluginContextPtr = std::shared_ptr<GlobalPluginContext>;

class GPGFRONTEND_CORE_EXPORT PluginManager : public QObject {
  Q_OBJECT
 public:
  static PluginMangerPtr GetInstance();

  void RegisterPlugin(PluginPtr);

  void TriggerEvent(EventRefrernce);

  void ActivePlugin(PluginIdentifier);

  void DeactivePlugin(PluginIdentifier);

  std::optional<TaskRunnerPtr> GetTaskRunner(PluginIdentifier);

 private:
  PluginManager();

  static PluginMangerPtr global_plugin_manager_;
  TaskRunnerPtr task_runner_;
  GlobalPluginContextPtr global_plugin_context_;
};

}  // namespace GpgFrontend::Plugin

#endif  // GPGFRONTEND_PLUGINMANAGER_H