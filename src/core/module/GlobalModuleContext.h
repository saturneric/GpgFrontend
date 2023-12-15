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

#pragma once

#include <optional>

#include "core/module/Event.h"
#include "core/thread/TaskRunner.h"
#include "function/SecureMemoryAllocator.h"
#include "module/GlobalRegisterTable.h"

namespace GpgFrontend::Module {

class GlobalModuleContext;
class GlobalRegisterTable;

class Module;
class ModuleManager;
using ModuleIdentifier = std::string;
using ModulePtr = std::shared_ptr<Module>;
using ModuleRawPtr = Module*;

using GMCPtr = std::shared_ptr<GlobalModuleContext>;
using GRTPtr = std::shared_ptr<GlobalRegisterTable>;

using TaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class GPGFRONTEND_CORE_EXPORT GlobalModuleContext : public QObject {
  Q_OBJECT
 public:
  explicit GlobalModuleContext();

  ~GlobalModuleContext() override;

  auto GetChannel(ModuleRawPtr) -> int;

  static auto GetDefaultChannel(ModuleRawPtr) -> int;

  auto GetTaskRunner(ModuleRawPtr) -> std::optional<TaskRunnerPtr>;

  auto GetTaskRunner(ModuleIdentifier) -> std::optional<TaskRunnerPtr>;

  auto GetGlobalTaskRunner() -> std::optional<TaskRunnerPtr>;

  auto RegisterModule(ModulePtr) -> bool;

  auto ActiveModule(ModuleIdentifier) -> bool;

  auto DeactivateModule(ModuleIdentifier) -> bool;

  auto ListenEvent(ModuleIdentifier, EventIdentifier) -> bool;

  auto TriggerEvent(EventRefrernce) -> bool;

  auto IsModuleActivated(ModuleIdentifier) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend::Module