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

namespace GpgFrontend::Thread {
class TaskRunner;
}

namespace GpgFrontend::Module {

using TaskRunnerPtr = std::shared_ptr<Thread::TaskRunner>;

class Event;
class Module;
class GlobalModuleContext;
class ModuleManager;

using EventRefrernce = std::shared_ptr<Event>;
using ModuleIdentifier = std::string;
using ModulePtr = std::shared_ptr<Module>;
using ModuleMangerPtr = std::shared_ptr<ModuleManager>;
using GMCPtr = std::shared_ptr<GlobalModuleContext>;
using Namespace = std::string;
using Key = std::string;
using LPCallback = std::function<void(Namespace, Key, int, std::any)>;

auto GPGFRONTEND_CORE_EXPORT GetRealModuleIdentifier(const ModuleIdentifier& id)
    -> ModuleIdentifier;

class GPGFRONTEND_CORE_EXPORT ModuleManager : public QObject {
  Q_OBJECT
 public:
  ~ModuleManager() override;

  static auto GetInstance() -> ModuleMangerPtr;

  void RegisterModule(ModulePtr);

  void TriggerEvent(EventRefrernce);

  void ActiveModule(ModuleIdentifier);

  void DeactiveModule(ModuleIdentifier);

  auto GetTaskRunner(ModuleIdentifier) -> std::optional<TaskRunnerPtr>;

  auto UpsertRTValue(Namespace, Key, std::any) -> bool;

  auto RetrieveRTValue(Namespace, Key) -> std::optional<std::any>;

  auto ListenRTPublish(QObject*, Namespace, Key, LPCallback) -> bool;

 private:
  class Impl;
  std::unique_ptr<Impl> p_;
  static ModuleMangerPtr g_;

  ModuleManager();
};

template <typename T, typename... Args>
void RegisterModule(Args&&... args) {
  ModuleManager::GetInstance()->RegisterModule(
      std::make_shared<T>(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
void RegisterAndActivateModule(Args&&... args) {
  auto manager = ModuleManager::GetInstance();
  auto module = std::make_shared<T>(std::forward<Args>(args)...);
  manager->RegisterModule(module);
  manager->ActiveModule(module->GetModuleIdentifier());
}

template <typename... Args>
void TriggerEvent(const EventIdentifier& event_id, Args&&... args,
                  Event::EventCallback e_cb = nullptr) {
  ModuleManager::GetInstance()->TriggerEvent(
      std::move(MakeEvent(event_id, std::forward<Args>(args)..., e_cb)));
}

auto GPGFRONTEND_CORE_EXPORT UpsertRTValue(const std::string& namespace_,
                                           const std::string& key,
                                           const std::any& value) -> bool;

auto GPGFRONTEND_CORE_EXPORT ListenRTPublishEvent(QObject*, Namespace, Key,
                                                  LPCallback) -> bool;

template <typename T>
auto RetrieveRTValueTyped(const std::string& namespace_, const std::string& key)
    -> std::optional<T> {
  auto any_value =
      ModuleManager::GetInstance()->RetrieveRTValue(namespace_, key);
  if (any_value && any_value->type() == typeid(T)) {
    return std::any_cast<T>(*any_value);
  }
  return std::nullopt;
}

template <typename T>
auto RetrieveRTValueTypedOrDefault(const std::string& namespace_,
                                   const std::string& key,
                                   const T& defaultValue) -> T {
  auto any_value =
      ModuleManager::GetInstance()->RetrieveRTValue(namespace_, key);
  if (any_value && any_value->type() == typeid(T)) {
    return std::any_cast<T>(*any_value);
  }
  return defaultValue;
}

}  // namespace GpgFrontend::Module