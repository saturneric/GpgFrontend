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

#include "Module.h"

#include <memory>

#include "core/module/GlobalModuleContext.h"

namespace GpgFrontend::Module {

class Module::Impl {
 public:
  friend class GlobalModuleContext;

  Impl(ModuleIdentifier id, ModuleVersion version, ModuleMetaData meta_data)
      : identifier_((boost::format("__module_%1%") % id).str()),
        version_(version),
        meta_data_(meta_data) {}

  int GetChannel() {
    return get_global_module_context()->GetChannel(self_shared_ptr_);
  }

  int GetDefaultChannel() {
    return get_global_module_context()->GetDefaultChannel(self_shared_ptr_);
  }

  std::optional<TaskRunnerPtr> GetTaskRunner() {
    return get_global_module_context()->GetTaskRunner(self_shared_ptr_);
  }

  bool ListenEvent(EventIdentifier event) {
    return get_global_module_context()->ListenEvent(gpc_get_identifier(),
                                                    event);
  }

  ModuleIdentifier GetModuleIdentifier() const { return identifier_; }

  void SetGPC(GlobalModuleContextPtr gpc) { gpc_ = gpc; }

 private:
  GlobalModuleContextPtr gpc_;
  const std::shared_ptr<Module> self_shared_ptr_;
  const ModuleIdentifier identifier_;
  const ModuleVersion version_;
  const ModuleMetaData meta_data_;

  ModuleIdentifier gpc_get_identifier() { return identifier_; }

  const GlobalModuleContextPtr get_global_module_context() {
    if (gpc_ == nullptr) {
      throw std::runtime_error("module is not registered by module manager");
    }
    return gpc_;
  }
};

Module::Module(ModuleIdentifier id, ModuleVersion version,
               ModuleMetaData meta_data)
    : p_(std::make_unique<Impl>(id, version, meta_data)) {}

Module::~Module() = default;

int Module::getChannel() { return p_->GetChannel(); }

int Module::getDefaultChannel() { return p_->GetDefaultChannel(); }

TaskRunnerPtr Module::getTaskRunner() {
  return p_->GetTaskRunner().value_or(nullptr);
}

bool Module::listenEvent(EventIdentifier event) {
  return p_->ListenEvent(event);
}

ModuleIdentifier Module::GetModuleIdentifier() const {
  return p_->GetModuleIdentifier();
}

void Module::SetGPC(GlobalModuleContextPtr gpc) { p_->SetGPC(gpc); }
}  // namespace GpgFrontend::Module