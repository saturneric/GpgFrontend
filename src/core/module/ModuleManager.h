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

/**
 * @brief Singleton facade over GlobalModuleContext and GlobalRegisterTable.
 *
 * Provides the primary API for loading, registering, activating, and
 * communicating with modules. Also exposes the runtime value store
 * (UpsertRTValue / RetrieveRTValue) and event dispatch (TriggerEvent).
 */
class GF_CORE_EXPORT ModuleManager
    : public SingletonFunctionObject<ModuleManager> {
 public:
  /**
   * @brief Construct the manager and create the GlobalModuleContext and
   * GlobalRegisterTable.
   *
   * @param channel singleton channel identifier
   */
  explicit ModuleManager(int channel);

  virtual ~ModuleManager() override;

  /**
   * @brief Load a module from the given library path and schedule it for
   * registration.
   *
   * @param path filesystem path to the module shared library
   * @param integrated true if this is a built-in integrated module
   * @return true if the library was loaded and the module is valid
   */
  auto LoadModule(QString path, bool integrated) -> bool;

  /**
   * @brief Find a registered module by its identifier.
   *
   * @param module_id module identifier
   * @return shared pointer to the module, or nullptr if not found
   */
  auto SearchModule(ModuleIdentifier module_id) -> ModulePtr;

  /**
   * @brief Set the total number of modules expected to register.
   *
   * Used by IsAllModulesRegistered() to determine when startup is complete.
   *
   * @param n expected module count
   */
  void SetNeedRegisterModulesNum(int n);

  /**
   * @brief Return the identifiers of all currently registered modules.
   *
   * @return list of module identifier strings
   */
  auto ListAllRegisteredModuleID() -> QStringList;

  /**
   * @brief Register a module with the GlobalModuleContext.
   *
   * @param module shared pointer to the module to register
   */
  void RegisterModule(ModulePtr module);

  /**
   * @brief Return true when the number of registered modules equals the
   * expected count.
   *
   * @return true if all expected modules have registered
   */
  auto IsAllModulesRegistered() -> bool;

  /**
   * @brief Return whether the given module is currently active.
   *
   * @param module_id module identifier
   * @return true if active
   */
  auto IsModuleActivated(ModuleIdentifier module_id) -> bool;

  /**
   * @brief Return whether the given module is a built-in integrated module.
   *
   * @param module_id module identifier
   * @return true if integrated
   */
  auto IsIntegratedModule(ModuleIdentifier module_id) -> bool;

  /**
   * @brief Subscribe a module to an event type.
   *
   * @param module_id identifier of the subscribing module
   * @param event_id event type identifier
   */
  void ListenEvent(ModuleIdentifier module_id, EventIdentifier event_id);

  /**
   * @brief Dispatch an event to all subscribed modules.
   *
   * @param event shared pointer to the event to dispatch
   */
  void TriggerEvent(EventReference event);

  /**
   * @brief Look up an in-flight event by its trigger UUID.
   *
   * @param trigger_id trigger UUID string
   * @return the matching EventReference, or empty if not found
   */
  auto SearchEvent(EventTriggerIdentifier trigger_id)
      -> std::optional<EventReference>;

  /**
   * @brief Return the event identifiers the given module is subscribed to.
   *
   * @param module_id module identifier
   * @return list of subscribed event identifiers
   */
  auto GetModuleListening(ModuleIdentifier module_id) -> QStringList;

  /**
   * @brief Activate a registered module.
   *
   * @param module_id module identifier
   */
  void ActiveModule(ModuleIdentifier module_id);

  /**
   * @brief Deactivate an active module.
   *
   * @param module_id module identifier
   */
  void DeactivateModule(ModuleIdentifier module_id);

  /**
   * @brief Return the TaskRunner for the given module.
   *
   * @param module_id module identifier
   * @return task runner if the module is registered, or empty
   */
  auto GetTaskRunner(ModuleIdentifier module_id)
      -> std::optional<TaskRunnerPtr>;

  /**
   * @brief Insert or update a value in the global runtime register table.
   *
   * @param ns namespace string
   * @param key key string
   * @param value typed value to store
   * @return true on success
   */
  auto UpsertRTValue(Namespace ns, Key key, std::any value) -> bool;

  /**
   * @brief Retrieve a value from the global runtime register table.
   *
   * @param ns namespace string
   * @param key key string
   * @return the stored value if present, or empty
   */
  auto RetrieveRTValue(Namespace ns, Key key) -> std::optional<std::any>;

  /**
   * @brief Subscribe to change notifications for a namespace/key in the
   * register table.
   *
   * @param obj QObject whose lifetime bounds the subscription
   * @param ns namespace string
   * @param key key string
   * @param callback called with (namespace, key, version, value) on each
   * publish
   * @return true if the subscription was registered
   */
  auto ListenRTPublish(QObject* obj, Namespace ns, Key key, LPCallback callback)
      -> bool;

  /**
   * @brief List direct child keys under the given namespace/key node.
   *
   * @param ns namespace string
   * @param key parent key string
   * @return list of child key strings
   */
  auto ListRTChildKeys(const QString& ns, const QString& key)
      -> QContainer<Key>;

  /**
   * @brief Return a raw pointer to the GlobalRegisterTable.
   *
   * @return pointer to the register table; never null after construction
   */
  auto GRT() -> GlobalRegisterTable*;

  /**
   * @brief Return whether any module is listening for the given event trigger.
   *
   * @param trigger_id event trigger identifier
   * @return true if at least one module is subscribed
   */
  auto IsEventListening(const EventTriggerIdentifier& trigger_id) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

/**
 * @brief Register a module of type T with the singleton ModuleManager.
 *
 * @tparam T module type (must derive from Module)
 * @tparam Args constructor argument types
 * @param args arguments forwarded to the T constructor
 */
template <typename T, typename... Args>
void RegisterModule(Args&&... args) {
  ModuleManager::GetInstance().RegisterModule(
      GpgFrontend::SecureCreateSharedObject<T>(std::forward<Args>(args)...));
}

/**
 * @brief Register and immediately activate a module of type T.
 *
 * @tparam T module type (must derive from Module)
 * @tparam Args constructor argument types
 * @param args arguments forwarded to the T constructor
 */
template <typename T, typename... Args>
void RegisterAndActivateModule(Args&&... args) {
  auto& manager = ModuleManager::GetInstance();
  auto module =
      GpgFrontend::SecureCreateSharedObject<T>(std::forward<Args>(args)...);
  manager.RegisterModule(module);
  manager.ActiveModule(module->GetModuleIdentifier());
}

/**
 * @brief Create and dispatch an event via the singleton ModuleManager.
 *
 * @param event_id event type identifier
 * @param params key-value parameters (default: empty)
 * @param e_cb optional callback invoked when the event is handled
 */
template <typename... Args>
void TriggerEvent(const EventIdentifier& event_id,
                  const Event::Params& params = {},
                  Event::EventCallback e_cb = nullptr) {
  ModuleManager::GetInstance().TriggerEvent(
      MakeEvent(event_id, params, std::move(e_cb)));
}

/**
 * @brief Return whether the module with the given identifier is currently
 * active.
 *
 * @param module_id module identifier
 * @return true if the module is registered and active
 */
auto GF_CORE_EXPORT IsModuleActivate(ModuleIdentifier module_id) -> bool;

/**
 * @brief Return whether a module with the given identifier is registered.
 *
 * @param module_id module identifier
 * @return true if the module exists in the registry
 */
auto GF_CORE_EXPORT IsModuleExists(ModuleIdentifier module_id) -> bool;

/**
 * @brief Insert or update a value in the singleton ModuleManager's register
 * table.
 *
 * @param namespace_ namespace string
 * @param key key string
 * @param value typed value to store
 * @return true on success
 */
auto GF_CORE_EXPORT UpsertRTValue(const QString& namespace_, const QString& key,
                                  const std::any& value) -> bool;

/**
 * @brief Subscribe to register table change notifications via the singleton
 * ModuleManager.
 *
 * @param obj QObject whose lifetime bounds the subscription
 * @param ns namespace string
 * @param key key string
 * @param callback function called on each publish
 * @return true if the subscription was registered
 */
auto GF_CORE_EXPORT ListenRTPublishEvent(QObject* obj, Namespace ns, Key key,
                                         LPCallback callback) -> bool;

/**
 * @brief List child keys under a namespace/key node via the singleton
 * ModuleManager.
 *
 * @param namespace_ namespace string
 * @param key parent key string
 * @return list of child key strings
 */
auto GF_CORE_EXPORT ListRTChildKeys(const QString& namespace_,
                                    const QString& key) -> QContainer<Key>;

/**
 * @brief Retrieve a typed value from the singleton register table.
 *
 * @tparam T expected value type
 * @param namespace_ namespace string
 * @param key key string
 * @return the value cast to T, or empty if not found or type does not match
 */
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

/**
 * @brief Retrieve a typed value from the singleton register table, returning a
 * default if absent.
 *
 * @tparam T expected value type
 * @param namespace_ namespace string
 * @param key key string
 * @param defaultValue value to return if not found or type does not match
 * @return the stored value cast to T, or @p defaultValue
 */
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

/**
 * @brief Return whether any module is listening for the given event trigger.
 *
 * @param trigger_id event trigger identifier
 * @return true if at least one module is subscribed
 */
auto GF_CORE_EXPORT IsEventListening(const EventTriggerIdentifier& trigger_id)
    -> bool;

}  // namespace GpgFrontend::Module
