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

#include "core/module/Event.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Module {

class Module;
class GlobalModuleContext;
class ModuleManager;

using ModuleIdentifier = std::string;
using ModuleVersion = std::string;
using ModuleMetaData = std::map<std::string, std::string>;
using ModulePtr = std::shared_ptr<Module>;
using GMCPtr = std::shared_ptr<GlobalModuleContext>;

using TaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class GPGFRONTEND_CORE_EXPORT Module : public QObject {
  Q_OBJECT
 public:
  Module(ModuleIdentifier, ModuleVersion, ModuleMetaData);

  ~Module();

  virtual bool Register() = 0;

  virtual bool Active() = 0;

  virtual int Exec(EventRefrernce) = 0;

  virtual bool Deactive() = 0;

  ModuleIdentifier GetModuleIdentifier() const;

  void SetGPC(GMCPtr);

 protected:
  int getChannel();

  int getDefaultChannel();

  TaskRunnerPtr getTaskRunner();

  bool listenEvent(EventIdentifier);

 private:
  class Impl;
  std::unique_ptr<Impl> p_;
};

}  // namespace GpgFrontend::Module