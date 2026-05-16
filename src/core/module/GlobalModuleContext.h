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

#include <optional>

#include "core/module/Event.h"
#include "core/thread/TaskRunner.h"
#include "module/GlobalRegisterTable.h"

namespace GpgFrontend::Module {

class GlobalModuleContext;
class GlobalRegisterTable;

class Module;
class ModuleManager;
using ModuleIdentifier = QString;
using ModulePtr = QSharedPointer<Module>;
using ModuleRawPtr = Module*;

using GMCPtr = QSharedPointer<GlobalModuleContext>;
using GRTPtr = QSharedPointer<GlobalRegisterTable>;

using TaskRunnerPtr = QSharedPointer<Thread::TaskRunner>;

/**
 * @brief Core orchestrator of the module system.
 *
 * Manages module registration, activation, deactivation, channel assignment,
 * event subscription, and event dispatch. Each module is assigned a unique
 * integer channel and a dedicated TaskRunner. Integrated (built-in) modules
 * are distinguished from dynamically loaded ones.
 */
class GF_CORE_EXPORT GlobalModuleContext : public QObject {
  Q_OBJECT
 public:
  /**
   * @brief Construct the context and reserve the default and non-ASCII
   * channels.
   */
  explicit GlobalModuleContext();

  ~GlobalModuleContext() override;

  /**
   * @brief Find a registered module by its identifier.
   *
   * @param module_id module identifier string
   * @return shared pointer to the module, or nullptr if not found
   */
  auto SearchModule(const ModuleIdentifier& module_id) -> ModulePtr;

  /**
   * @brief Return the channel assigned to the given module.
   *
   * Falls back to the default channel if the module is not registered.
   *
   * @param module raw pointer to the module
   * @return assigned channel ID
   */
  auto GetChannel(ModuleRawPtr module) -> int;

  /**
   * @brief Return the default channel (kGpgFrontendDefaultChannel).
   *
   * @param module unused; kept for API symmetry with GetChannel
   * @return default channel ID
   */
  static auto GetDefaultChannel(ModuleRawPtr module) -> int;

  /**
   * @brief Return the TaskRunner for the given module.
   *
   * @param module raw pointer to the module
   * @return task runner if the module is registered, or empty
   */
  auto GetTaskRunner(ModuleRawPtr module) -> std::optional<TaskRunnerPtr>;

  /**
   * @brief Return the TaskRunner for the module with the given identifier.
   *
   * @param module_id module identifier string
   * @return task runner if the module is registered, or empty
   */
  auto GetTaskRunner(const ModuleIdentifier& module_id)
      -> std::optional<TaskRunnerPtr>;

  /**
   * @brief Return the global task runner shared across all modules.
   *
   * @return global task runner, or empty if unavailable
   */
  auto GetGlobalTaskRunner() -> std::optional<TaskRunnerPtr>;

  /**
   * @brief Register a module and assign it a channel and task runner.
   *
   * @param module shared pointer to the module to register
   * @param integrated true if this is a built-in integrated module
   * @return true on success, false if the module is already registered or
   * invalid
   */
  auto RegisterModule(const ModulePtr& module, bool integrated) -> bool;

  /**
   * @brief Activate a registered module by calling its Active() lifecycle
   * method.
   *
   * @param module_id identifier of the module to activate
   * @return true on success, false if not registered or activation fails
   */
  auto ActiveModule(const ModuleIdentifier& module_id) -> bool;

  /**
   * @brief Deactivate an active module by calling its Deactivate() lifecycle
   * method.
   *
   * @param module_id identifier of the module to deactivate
   * @return true on success, false if not active or deactivation fails
   */
  auto DeactivateModule(const ModuleIdentifier& module_id) -> bool;

  /**
   * @brief Subscribe a module to an event type.
   *
   * @param module_id identifier of the subscribing module
   * @param event_id event type identifier to subscribe to
   * @return true if the subscription was registered successfully
   */
  auto ListenEvent(const ModuleIdentifier& module_id,
                   const EventIdentifier& event_id) -> bool;

  /**
   * @brief Dispatch an event to all modules subscribed to its identifier.
   *
   * @param event shared pointer to the event to dispatch
   * @return true if the event was dispatched to at least one module
   */
  auto TriggerEvent(const EventReference& event) -> bool;

  /**
   * @brief Look up an in-flight event by its trigger UUID.
   *
   * @param trigger_id trigger UUID string
   * @return the matching EventReference, or empty if not found
   */
  auto SearchEvent(const EventTriggerIdentifier& trigger_id)
      -> std::optional<EventReference>;

  /**
   * @brief Return the list of event identifiers the given module is subscribed
   * to.
   *
   * @param module_id module identifier
   * @return list of subscribed event identifiers
   */
  auto GetModuleListening(const ModuleIdentifier& module_id) -> QStringList;

  /**
   * @brief Return whether the given module is currently active.
   *
   * @param module_id module identifier
   * @return true if active
   */
  auto IsModuleActivated(const ModuleIdentifier& module_id) -> bool;

  /**
   * @brief Return whether the given module is a built-in integrated module.
   *
   * @param module_id module identifier
   * @return true if integrated
   */
  auto IsIntegratedModule(const ModuleIdentifier& module_id) -> bool;

  /**
   * @brief Return the identifiers of all currently registered modules.
   *
   * @return list of module identifier strings
   */
  auto ListAllRegisteredModuleID() -> QStringList;

  /**
   * @brief Return the number of currently registered modules.
   *
   * @return registered module count
   */
  [[nodiscard]] auto GetRegisteredModuleNum() const -> int;

  /**
   * @brief Return whether any module is currently subscribed to the given event
   * trigger.
   *
   * @param trigger_id event trigger identifier
   * @return true if at least one module is listening
   */
  auto IsEventListening(const EventTriggerIdentifier& trigger_id) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend::Module
