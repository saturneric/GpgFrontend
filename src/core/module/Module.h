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

#include "core/module/Event.h"
#include "core/thread/TaskRunner.h"

namespace GpgFrontend::Module {

class Module;
class GlobalModuleContext;
class ModuleManager;

using ModuleIdentifier = QString;
using ModuleVersion = QString;
using ModuleMetaData = QMap<QString, QString>;
using ModulePtr = QSharedPointer<Module>;

using TaskRunnerPtr = QSharedPointer<Thread::TaskRunner>;

/**
 * @brief Base class for all GpgFrontend modules.
 *
 * A Module can be constructed directly for integrated (built-in) modules or
 * loaded from a QLibrary for dynamically linked modules. Dynamic modules are
 * validated against required symbol exports and version constraints at load
 * time. The lifecycle is: Register -> Active -> Exec (per event) -> Deactivate
 * -> UnRegister. Subclasses override the virtual lifecycle methods.
 */
class GF_CORE_EXPORT Module : public QObject {
  Q_OBJECT
 public:
  /**
   * @brief Construct an integrated (built-in) module with the given identity
   * and metadata.
   *
   * @param id unique module identifier string
   * @param version module version string
   * @param meta_data arbitrary key-value metadata map
   */
  Module(ModuleIdentifier id, ModuleVersion version,
         const ModuleMetaData& meta_data);

  /**
   * @brief Load and validate a dynamic module from a QLibrary.
   *
   * Resolves required symbol exports, reads the module identifier, version,
   * SDK version, and Qt version strings. Call IsGood() after construction to
   * verify the module loaded successfully.
   *
   * @param module_library loaded QLibrary to extract the module from
   */
  explicit Module(QLibrary& module_library);

  ~Module();

  /**
   * @brief Return true if the module was successfully initialised.
   *
   * @return true if the module is valid and ready to register
   */
  auto IsGood() -> bool;

  /**
   * @brief Called by the module system when the module is registered.
   *
   * Override to perform one-time setup. Return 0 on success.
   *
   * @return 0 on success, non-zero on failure
   */
  virtual auto Register() -> int;

  /**
   * @brief Called by the module system when the module is activated.
   *
   * Override to start background tasks or subscribe to events.
   * Return 0 on success.
   *
   * @return 0 on success, non-zero on failure
   */
  virtual auto Active() -> int;

  /**
   * @brief Called by the module system to deliver an event to the module.
   *
   * Override to handle the event and invoke its callback when done.
   * Return 0 on success.
   *
   * @param event the event to handle
   * @return 0 on success, non-zero on failure
   */
  virtual auto Exec(EventReference event) -> int;

  /**
   * @brief Called by the module system when the module is deactivated.
   *
   * Override to stop background tasks and clean up resources.
   * Return 0 on success.
   *
   * @return 0 on success, non-zero on failure
   */
  virtual auto Deactivate() -> int;

  /**
   * @brief Called by the module system when the module is unregistered.
   *
   * Override to release any remaining resources. Return 0 on success.
   *
   * @return 0 on success, non-zero on failure
   */
  virtual auto UnRegister() -> int;

  /**
   * @brief Return this module's unique identifier.
   *
   * @return module identifier string
   */
  [[nodiscard]] auto GetModuleIdentifier() const -> ModuleIdentifier;

  /**
   * @brief Return this module's version string.
   *
   * @return version string
   */
  [[nodiscard]] auto GetModuleVersion() const -> ModuleVersion;

  /**
   * @brief Return this module's metadata map.
   *
   * @return key-value metadata
   */
  [[nodiscard]] auto GetModuleMetaData() const -> ModuleMetaData;

  /**
   * @brief Return the filesystem path of the dynamic module library.
   *
   * Returns an empty string for integrated modules.
   *
   * @return absolute path to the module library file
   */
  [[nodiscard]] auto GetModulePath() const -> QString;

  /**
   * @brief Return a checksum of the module binary.
   *
   * Used to detect version changes between runs. Empty for integrated modules.
   *
   * @return binary checksum string
   */
  [[nodiscard]] auto GetModuleHash() const -> QString;

  /**
   * @brief Return the GF SDK version the module was built against.
   *
   * @return SDK version string
   */
  [[nodiscard]] auto GetModuleSDKVersion() const -> QString;

  /**
   * @brief Return the Qt version the module was built against.
   *
   * @return Qt version string
   */
  [[nodiscard]] auto GetModuleQtEnvVersion() const -> QString;

  /**
   * @brief Inject the GlobalModuleContext into the module.
   *
   * Called by the module system after registration. Must be set before any
   * lifecycle methods are invoked.
   *
   * @param gmc pointer to the global module context
   */
  void SetGPC(GlobalModuleContext* gmc);

 protected:
  /**
   * @brief Return the channel ID assigned to this module.
   *
   * @return channel ID
   */
  auto getChannel() -> int;

  /**
   * @brief Return the default channel ID.
   *
   * @return default channel ID
   */
  auto getDefaultChannel() -> int;

  /**
   * @brief Return the TaskRunner assigned to this module.
   *
   * @return task runner shared pointer
   */
  auto getTaskRunner() -> TaskRunnerPtr;

  /**
   * @brief Subscribe this module to an event type.
   *
   * @param event_id event type identifier to subscribe to
   * @return true if the subscription was registered successfully
   */
  auto listenEvent(EventIdentifier event_id) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend::Module
