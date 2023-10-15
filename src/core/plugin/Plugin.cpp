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

#include "Plugin.h"

#include "GpgConstants.h"
#include "core/GpgFrontendCore.h"
#include "core/plugin/GlobalPluginContext.h"

namespace GpgFrontend::Plugin {

Plugin::Plugin(PluginIdentifier id, PluginVersion version,
               PluginMetaData meta_data)
    : self_shared_ptr_(this),
      identifier_((boost::format("__plugin_%1%") % id).str()),
      version_(version),
      meta_data_(meta_data) {}

const GlobalPluginContextPtr
GpgFrontend::Plugin::Plugin::get_global_plugin_context() {
  if (global_plugin_context_ == nullptr) {
    throw std::runtime_error("plugin is not registered by plugin manager");
  }
  return global_plugin_context_;
}

int Plugin::getChannel() {
  return get_global_plugin_context()->GetChannel(self_shared_ptr_);
}

int Plugin::getDefaultChannel() {
  return get_global_plugin_context()->GetDefaultChannel(self_shared_ptr_);
}

std::optional<TaskRunnerPtr> Plugin::getTaskRunner() {
  return get_global_plugin_context()->GetTaskRunner(self_shared_ptr_);
}

bool Plugin::listenEvent(EventIdentifier event) {
  return get_global_plugin_context()->ListenEvent(gpc_get_identifier(), event);
}

PluginIdentifier Plugin::GetPluginIdentifier() const { return identifier_; }

PluginIdentifier Plugin::gpc_get_identifier() { return identifier_; }

}  // namespace GpgFrontend::Plugin