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

using ModuleIdentifier = QString;
using ModuleVersion = QString;
using ModuleMetaData = std::map<QString, QString>;
using ModulePtr = std::shared_ptr<Module>;

using TaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class GPGFRONTEND_CORE_EXPORT Module : public QObject {
  Q_OBJECT
 public:
  Module(ModuleIdentifier, ModuleVersion, const ModuleMetaData &);

  explicit Module(QLibrary &module_library);

  ~Module();

  auto IsGood() -> bool;

  virtual auto Register() -> int;

  virtual auto Active() -> int;

  virtual auto Exec(EventRefrernce) -> int;

  virtual auto Deactive() -> int;

  virtual auto UnRegister() -> int;

  [[nodiscard]] auto GetModuleIdentifier() const -> ModuleIdentifier;

  [[nodiscard]] auto GetModuleVersion() const -> ModuleVersion;

  [[nodiscard]] auto GetModuleMetaData() const -> ModuleMetaData;

  [[nodiscard]] auto GetModulePath() const -> QString;

  [[nodiscard]] auto GetModuleHash() const -> QString;

  void SetGPC(GlobalModuleContext *);

 protected:
  auto getChannel() -> int;

  auto getDefaultChannel() -> int;

  auto getTaskRunner() -> TaskRunnerPtr;

  auto listenEvent(EventIdentifier) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend::Module