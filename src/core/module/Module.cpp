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

#include <utility>

#include "core/module/GlobalModuleContext.h"

namespace GpgFrontend::Module {

class Module::Impl {
 public:
  friend class GlobalModuleContext;

  using ExecCallback = std::function<void(int)>;

  Impl(ModuleRawPtr m_ptr, ModuleIdentifier id, ModuleVersion version,
       ModuleMetaData meta_data)
      : m_ptr_(m_ptr),
        identifier_(std::move(id)),
        version_(std::move(version)),
        meta_data_(std::move(meta_data)) {}

  auto GetChannel() -> int { return get_gpc()->GetChannel(m_ptr_); }

  auto GetDefaultChannel() -> int {
    return get_gpc()->GetDefaultChannel(m_ptr_);
  }

  auto GetTaskRunner() -> std::optional<TaskRunnerPtr> {
    return get_gpc()->GetTaskRunner(m_ptr_);
  }

  auto ListenEvent(EventIdentifier event) -> bool {
    return get_gpc()->ListenEvent(GetModuleIdentifier(), std::move(event));
  }

  [[nodiscard]] auto GetModuleIdentifier() const -> ModuleIdentifier {
    return identifier_;
  }

  void SetGPC(GlobalModuleContext* gpc) { gpc_ = gpc; }

 private:
  GlobalModuleContext* gpc_;
  Module* m_ptr_;
  const ModuleIdentifier identifier_;
  const ModuleVersion version_;
  const ModuleMetaData meta_data_;

  auto get_gpc() -> GlobalModuleContext* {
    if (gpc_ == nullptr) {
      throw std::runtime_error("module is not registered by module manager");
    }
    return gpc_;
  }
};

Module::Module(ModuleIdentifier id, ModuleVersion version,
               ModuleMetaData meta_data)
    : p_(SecureCreateUniqueObject<Impl>(this, id, version, meta_data)) {}

Module::~Module() = default;

auto Module::getChannel() -> int { return p_->GetChannel(); }

auto Module::getDefaultChannel() -> int { return p_->GetDefaultChannel(); }

auto Module::getTaskRunner() -> TaskRunnerPtr {
  return p_->GetTaskRunner().value_or(nullptr);
}

auto Module::listenEvent(EventIdentifier event) -> bool {
  return p_->ListenEvent(std::move(event));
}

auto Module::GetModuleIdentifier() const -> ModuleIdentifier {
  return p_->GetModuleIdentifier();
}

void Module::SetGPC(GlobalModuleContext* gpc) { p_->SetGPC(gpc); }
}  // namespace GpgFrontend::Module