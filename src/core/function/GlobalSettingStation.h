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

#include <qsettings.h>

#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Singleton managing application configuration, directory paths, and
 * cryptographic key material.
 *
 * Provides access to the platform-appropriate QSettings store, resolves all
 * application directory paths (data, logs, config, modules, secure storage),
 * and maintains the in-memory store of app secure keys indexed by their IDs.
 * In portable mode, data is placed alongside the executable instead of the
 * OS user-data directory.
 */
class GF_CORE_EXPORT GlobalSettingStation
    : public SingletonFunctionObject<GlobalSettingStation> {
 public:
  /**
   * @brief Construct the station, initialise application directories, and
   * detect portable mode.
   *
   * @param channel singleton channel identifier
   */
  explicit GlobalSettingStation(
      int channel = SingletonFunctionObject::GetDefaultChannel()) noexcept;

  /**
   * @brief Destroy the station and release all held key material.
   */
  ~GlobalSettingStation() noexcept override;

  /**
   * @brief Open and return the application settings store.
   *
   * Returns an INI-backed QSettings on Windows and in portable mode; otherwise
   * returns the platform-native QSettings.
   *
   * @return QSettings instance configured for the current platform and mode
   */
  [[nodiscard]] auto GetSettings() const -> QSettings;

  /**
   * @brief Return the path to the application executable directory.
   *
   * @return absolute path to the directory containing the application binary
   */
  [[nodiscard]] auto GetAppDir() const -> QString;

  /**
   * @brief Return the path to the application's writable data directory.
   *
   * @return absolute path to the app-local data directory
   */
  [[nodiscard]] auto GetAppDataPath() const -> QString;

  /**
   * @brief Return the path to the application's log directory.
   *
   * @return absolute path to the log directory
   */
  [[nodiscard]] auto GetAppLogPath() const -> QString;

  /**
   * @brief Return the path to the application's configuration file.
   *
   * @return absolute path to config.ini
   */
  [[nodiscard]] auto GetConfigPath() const -> QString;

  /**
   * @brief Return the path to the application's configuration directory.
   *
   * @return absolute path to the configuration directory
   */
  [[nodiscard]] auto GetConfigDirPath() const -> QString;

  /**
   * @brief Return the path to the modules directory.
   *
   * @return absolute path to the modules directory
   */
  [[nodiscard]] auto GetModulesDir() const -> QString;

  /**
   * @brief Return the path to the encrypted data objects directory.
   *
   * @return absolute path to the data objects directory
   */
  [[nodiscard]] auto GetDataObjectsDir() const -> QString;

  /**
   * @brief Return the total size of all log files as a human-friendly string.
   *
   * @return formatted size string (e.g. "4.2 MB")
   */
  [[nodiscard]] auto GetLogFilesSize() const -> QString;

  /**
   * @brief Return the total size of all data object files as a human-friendly
   * string.
   *
   * @return formatted size string (e.g. "1.1 MB")
   */
  [[nodiscard]] auto GetDataObjectsFilesSize() const -> QString;

  /**
   * @brief Delete all log files from the log directory.
   */
  void ClearAllLogFiles() const;

  /**
   * @brief Delete all files from the data objects directory.
   */
  void ClearAllDataObjects() const;

  /**
   * @brief Return the bundled module directory, resolving the correct path for
   * the current platform and environment.
   *
   * Accounts for AppImage, Flatpak, Windows, macOS bundle, and standard
   * install layouts, falling back to a sibling "modules" directory.
   *
   * @return absolute path to the integrated module directory
   */
  [[nodiscard]] auto GetIntegratedModulePath() const -> QString;

  /**
   * @brief Return whether the application is running in portable mode.
   *
   * In portable mode, data is stored alongside the executable rather than
   * in the OS user-data directory.
   *
   * @return true if portable mode is active, false otherwise
   */
  [[nodiscard]] auto IsProtableMode() const -> bool;

  /**
   * @brief Return the path to the legacy app secure key file.
   *
   * @return absolute path to the legacy key file (secure/app.key)
   */
  auto GetLegacyAppSecureKeyPath() -> QString;

  /**
   * @brief Return the path to the secure key storage directory.
   *
   * @return absolute path to the secure directory
   */
  auto GetAppSecureKeyDir() -> QString;

  /**
   * @brief Set the active encryption key ID used by the application.
   *
   * @param id binary key ID to register as active
   */
  void SetActiveKeyId(const GFBuffer& id);

  /**
   * @brief Set the legacy encryption key ID.
   *
   * @param id binary key ID to register as the legacy key
   */
  void SetLegacyKeyId(const GFBuffer& id);

  /**
   * @brief Return the currently registered active encryption key ID.
   *
   * @return binary key ID of the active key
   */
  auto GetActiveKeyId() -> GFBuffer;

  /**
   * @brief Return the active app encryption key, looked up by the active key
   * ID.
   *
   * @return binary key material for the active key
   */
  auto GetActiveAppSecureKey() -> GFBuffer;

  /**
   * @brief Return the legacy app encryption key, looked up by the legacy key
   * ID.
   *
   * @return binary key material for the legacy key
   */
  auto GetLegacyAppSecureKey() -> GFBuffer;

  /**
   * @brief Look up an app secure key by its ID.
   *
   * @param id binary key ID to look up
   * @return corresponding key material, or an empty GFBuffer if not found
   */
  auto GetAppSecureKey(const GFBuffer& id) -> GFBuffer;

  /**
   * @brief Add key-ID pairs to the in-memory app secure key store.
   *
   * @param keys map of key IDs to their corresponding key material
   */
  void AppendAppSecureKeys(const QMap<GFBuffer, GFBuffer>& keys);

  /**
   * @brief Return the legacy app encryption key (equivalent to
   * GetLegacyAppSecureKey).
   *
   * @return binary key material for the legacy key
   */
  auto GetLegacySecureKey() -> GFBuffer;

  /**
   * @brief Return whether the given OpenPGP engine is registered as supported.
   *
   * @param engine engine to query
   * @return true if the engine is in the supported set, false otherwise
   */
  auto IsEngineSupported(OpenPGPEngine engine) -> bool;

  /**
   * @brief Register an OpenPGP engine as supported.
   *
   * @param engine engine to add to the supported set
   */
  auto AddSupportedEngine(OpenPGPEngine engine) -> void;

  /**
   * @brief Remove an OpenPGP engine from the supported set.
   *
   * @param engine engine to remove
   */
  auto RemoveSupportedEngine(OpenPGPEngine engine) -> void;

  /**
   * @brief Return whether at least one OpenPGP engine is registered as
   * supported.
   *
   * @return true if the supported engine set is non-empty, false otherwise
   */
  auto HasSupportedEngine() -> bool;

  /**
   * @brief Return the string names of all supported OpenPGP engines.
   *
   * @return list of engine name strings
   */
  auto AllSupportedEngines() -> QStringList;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

/**
 * @brief Return a reference to the GlobalSettingStation singleton instance.
 *
 * @return reference to the singleton
 */
auto GF_CORE_EXPORT GetGSS() -> GlobalSettingStation&;

/**
 * @brief Convenience wrapper that returns the application QSettings via the
 * singleton.
 *
 * @return QSettings configured for the current platform and mode
 */
auto GF_CORE_EXPORT GetSettings() -> QSettings;

}  // namespace GpgFrontend
