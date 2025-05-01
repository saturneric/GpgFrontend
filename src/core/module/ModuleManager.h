/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include <vector>

#include "core/function/SecureMemoryAllocator.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/module/Event.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Thread {
class TaskRunner;
}

namespace GpgFrontend::Module {

using TaskRunnerPtr = QSharedPointer<Thread::TaskRunner>;

class Event;
class Module;
class GlobalModuleContext;
class ModuleManager;
class GlobalRegisterTable;

using EventReference = QSharedPointer<Event>;
using ModuleIdentifier = QString;
using ModulePtr = QSharedPointer<Module>;
using ModuleMangerPtr = QSharedPointer<ModuleManager>;
using GMCPtr = QSharedPointer<GlobalModuleContext>;
using Namespace = QString;
using Key = QString;
using LPCallback = std::function<void(Namespace, Key, int, std::any)>;

class GF_CORE_EXPORT ModuleManager
    : public SingletonFunctionObject<ModuleManager> {
 public:
  explicit ModuleManager(int channel);

  virtual ~ModuleManager() override;

  auto LoadModule(QString, bool) -> bool;

  auto SearchModule(ModuleIdentifier) -> ModulePtr;

  void SetNeedRegisterModulesNum(int);

  auto ListAllRegisteredModuleID() -> QStringList;

  void RegisterModule(ModulePtr);

  auto IsAllModulesRegistered() -> bool;

  auto IsModuleActivated(ModuleIdentifier) -> bool;

  auto IsIntegratedModule(ModuleIdentifier) -> bool;

  void ListenEvent(ModuleIdentifier, EventIdentifier);

  void TriggerEvent(EventReference);

  auto SearchEvent(EventTriggerIdentifier) -> std::optional<EventReference>;

  auto GetModuleListening(ModuleIdentifier) -> QStringList;

  void ActiveModule(ModuleIdentifier);

  void DeactivateModule(ModuleIdentifier);

  auto GetTaskRunner(ModuleIdentifier) -> std::optional<TaskRunnerPtr>;

  auto UpsertRTValue(Namespace, Key, std::any) -> bool;

  auto RetrieveRTValue(Namespace, Key) -> std::optional<std::any>;

  auto ListenRTPublish(QObject*, Namespace, Key, LPCallback) -> bool;

  auto ListRTChildKeys(const QString&, const QString&) -> QContainer<Key>;

  auto GRT() -> GlobalRegisterTable*;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

template <typename T, typename... Args>
void RegisterModule(Args&&... args) {
  ModuleManager::GetInstance().RegisterModule(
      GpgFrontend::SecureCreateSharedObject<T>(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
void RegisterAndActivateModule(Args&&... args) {
  auto& manager = ModuleManager::GetInstance();
  auto module =
      GpgFrontend::SecureCreateSharedObject<T>(std::forward<Args>(args)...);
  manager.RegisterModule(module);
  manager.ActiveModule(module->GetModuleIdentifier());
}

template <typename... Args>
void TriggerEvent(const EventIdentifier& event_id,
                  const Event::Params& params = {},
                  Event::EventCallback e_cb = nullptr) {
  ModuleManager::GetInstance().TriggerEvent(MakeEvent(event_id, params, e_cb));
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT IsModuleActivate(ModuleIdentifier) -> bool;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT IsModuleExists(ModuleIdentifier) -> bool;

/**
 * @brief
 *
 * @param namespace_
 * @param key
 * @param value
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT UpsertRTValue(const QString& namespace_, const QString& key,
                                  const std::any& value) -> bool;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT ListenRTPublishEvent(QObject*, Namespace, Key, LPCallback)
    -> bool;

/**
 * @brief
 *
 * @param namespace_
 * @param key
 * @return QContainer<Key>
 */
auto GF_CORE_EXPORT ListRTChildKeys(const QString& namespace_,
                                    const QString& key) -> QContainer<Key>;

template <typename T>
auto RetrieveRTValueTyped(const QString& namespace_, const QString& key)
    -> std::optional<T> {
  auto any_value =
      ModuleManager::GetInstance().RetrieveRTValue(namespace_, key);
  if (any_value && any_value->type() == typeid(T)) {
    return std::any_cast<T>(*any_value);
  }
  return std::nullopt;
}

template <typename T>
auto RetrieveRTValueTypedOrDefault(const QString& namespace_,
                                   const QString& key, const T& defaultValue)
    -> T {
  auto any_value =
      ModuleManager::GetInstance().RetrieveRTValue(namespace_, key);
  if (any_value && any_value->type() == typeid(T)) {
    return std::any_cast<T>(*any_value);
  }
  return defaultValue;
}

}  // namespace GpgFrontend::Module