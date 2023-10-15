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

#ifndef GPGFRONTEND_PLUGIN_H
#define GPGFRONTEND_PLUGIN_H

#include "core/plugin/Event.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Plugin {

using TaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class Plugin;
class GlobalPluginContext;
class PluginManager;

using PluginIdentifier = std::string;
using PluginVersion = std::string;
using PluginMetaData = std::map<std::string, std::string>;
using PluginPtr = std::shared_ptr<Plugin>;

using GlobalPluginContextPtr = std::shared_ptr<GlobalPluginContext>;

class GPGFRONTEND_CORE_EXPORT Plugin : public QObject {
  Q_OBJECT
 public:
  friend class PluginManager;
  friend class GlobalPluginContext;

  Plugin(PluginIdentifier, PluginVersion, PluginMetaData);

  virtual bool Register() = 0;

  virtual bool Active() = 0;

  virtual int Exec(EventRefrernce) = 0;

  virtual bool Deactive() = 0;

  PluginIdentifier GetPluginIdentifier() const;

 protected:
  int getChannel();

  int getDefaultChannel();

  std::optional<TaskRunnerPtr> getTaskRunner();

  bool listenEvent(EventIdentifier);

 private:
  const GlobalPluginContextPtr global_plugin_context_;
  const std::shared_ptr<Plugin> self_shared_ptr_;
  const PluginIdentifier identifier_;
  const PluginVersion version_;
  const PluginMetaData meta_data_;

  void pm_set_global_plugin_cotext(GlobalPluginContextPtr);

  PluginIdentifier gpc_get_identifier();

  bool gpc_register_plugin();

  bool gpc_active_plugin();

  bool gpc_deactive_plugin();

  int gpc_exec_plugin();

  const GlobalPluginContextPtr get_global_plugin_context();
};

}  // namespace GpgFrontend::Plugin

#endif  // GPGFRONTEND_PLUGIN_H