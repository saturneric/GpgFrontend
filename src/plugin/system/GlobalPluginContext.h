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

#ifndef GPGFRONTEND_GLOBALPLUGINCONTEXT_H
#define GPGFRONTEND_GLOBALPLUGINCONTEXT_H

#include "GpgFrontendPluginSystemExport.h"
#include "core/GpgFrontendCore.h"
#include "core/thread/TaskRunner.h"
#include "plugin/system/Event.h"

namespace GpgFrontend::Plugin {

class GlobalPluginContext;

class Plugin;
class PluginManager;
using PluginIdentifier = std::string;
using PluginPtr = std::shared_ptr<Plugin>;
using PluginList = std::list<std::string>;

using GlobalPluginContextPtr = std::shared_ptr<GlobalPluginContext>;

using TaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class GPGFRONTEND_PLUGIN_SYSTEM_EXPORT GlobalPluginContext : public QObject {
  Q_OBJECT
 public:
  GlobalPluginContext(TaskRunnerPtr);

  ~GlobalPluginContext();

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
  class Impl;
  std::unique_ptr<Impl> p_;
};

}  // namespace GpgFrontend::Plugin

#endif  // GPGFRONTEND_GLOBALPLUGINCONTEXT_H