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

#include "Plugin.h"

#include "plugin/system/GlobalPluginContext.h"

namespace GpgFrontend::Plugin {

class Plugin::Impl {
 public:
  friend class GlobalPluginContext;

  Impl(PluginIdentifier id, PluginVersion version, PluginMetaData meta_data)
      : identifier_((boost::format("__plugin_%1%") % id).str()),
        version_(version),
        meta_data_(meta_data) {}

  int GetChannel() {
    return get_global_plugin_context()->GetChannel(self_shared_ptr_);
  }

  int GetDefaultChannel() {
    return get_global_plugin_context()->GetDefaultChannel(self_shared_ptr_);
  }

  std::optional<TaskRunnerPtr> GetTaskRunner() {
    return get_global_plugin_context()->GetTaskRunner(self_shared_ptr_);
  }

  bool ListenEvent(EventIdentifier event) {
    return get_global_plugin_context()->ListenEvent(gpc_get_identifier(),
                                                    event);
  }

  PluginIdentifier GetPluginIdentifier() const { return identifier_; }

  PluginIdentifier SetGPC(GlobalPluginContextPtr gpc) { gpc_ = gpc; }

 private:
  GlobalPluginContextPtr gpc_;
  const std::shared_ptr<Plugin> self_shared_ptr_;
  const PluginIdentifier identifier_;
  const PluginVersion version_;
  const PluginMetaData meta_data_;

  PluginIdentifier gpc_get_identifier() { return identifier_; }

  const GlobalPluginContextPtr get_global_plugin_context() {
    if (gpc_ == nullptr) {
      throw std::runtime_error("plugin is not registered by plugin manager");
    }
    return gpc_;
  }
};

Plugin::Plugin(PluginIdentifier id, PluginVersion version,
               PluginMetaData meta_data)
    : s_(this) {}

int Plugin::getChannel() { return p_->GetChannel(); }

int Plugin::getDefaultChannel() { return p_->GetDefaultChannel(); }

TaskRunnerPtr Plugin::getTaskRunner() {
  return p_->GetTaskRunner().value_or(nullptr);
}

bool Plugin::listenEvent(EventIdentifier event) {
  return p_->ListenEvent(event);
}

PluginIdentifier Plugin::GetPluginIdentifier() const {
  return p_->GetPluginIdentifier();
}

PluginIdentifier Plugin::SetGPC(GlobalPluginContextPtr gpc) { p_->SetGPC(gpc); }
}  // namespace GpgFrontend::Plugin