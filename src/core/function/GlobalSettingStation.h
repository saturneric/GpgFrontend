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

#include <libconfig.h++>

#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @class GlobalSettingStation
 * @brief Singleton class for managing global settings in the application.
 *
 * This class handles reading and writing of global settings, as well as
 * managing application directories and resource paths.
 */
class GPGFRONTEND_CORE_EXPORT GlobalSettingStation
    : public SingletonFunctionObject<GlobalSettingStation> {
 public:
  /**
   * @brief Construct a new Global Setting Station object
   *
   */
  explicit GlobalSettingStation(
      int channel = SingletonFunctionObject::GetDefaultChannel()) noexcept;

  /**
   * @brief Destroy the Global Setting Station object
   *
   */
  ~GlobalSettingStation() noexcept override;

  /**
   * @brief
   *
   * @return libconfig::Setting&
   */
  auto GetMainSettings() noexcept -> libconfig::Setting &;

  /**
   * @brief Get the App Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetAppDir() const -> std::filesystem::path;

  /**
   * @brief Gets the application data directory.
   * @return Path to the application data directory.
   */
  [[nodiscard]] auto GetAppDataPath() const -> std::filesystem::path;

  /**
   * @brief Get the Log Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetLogDir() const -> std::filesystem::path;

  /**
   * @brief Get the Standalone Database Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetStandaloneDatabaseDir() const -> std::filesystem::path;

  /**
   * @brief Get the App Config Path object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetAppConfigPath() const -> std::filesystem::path;

  /**
   * @brief Get the Standalone Gpg Bin Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetStandaloneGpgBinDir() const -> std::filesystem::path;

  /**
   * @brief Get the Locale Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetLocaleDir() const -> std::filesystem::path;

  /**
   * @brief Get the Resource Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetResourceDir() const -> std::filesystem::path;

  /**
   * @brief Get the Certs Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetCertsDir() const -> std::filesystem::path;

  /**
   * @brief Get the Log Files Size object
   *
   * @return std::string
   */
  [[nodiscard]] auto GetLogFilesSize() const -> std::string;

  /**
   * @brief Get the Data Objects Files Size object
   *
   * @return std::string
   */
  [[nodiscard]] auto GetDataObjectsFilesSize() const -> std::string;

  /**
   * @brief clear all log files
   *
   */
  void ClearAllLogFiles() const;

  /**
   * @brief clear all data objects
   *
   */
  void ClearAllDataObjects() const;

  /**
   * @brief sync the settings to the file
   *
   */
  void SyncSettings() noexcept;

  /**
   * @brief Looks up a setting by path.
   * @param path The path to the setting.
   * @param default_value The default value to return if setting is not found.
   * @return The setting value.
   */
  template <typename T>
  auto LookupSettings(std::string path, T default_value) noexcept -> T {
    T value = default_value;
    try {
      value = static_cast<T>(GetMainSettings().lookup(path));
    } catch (...) {
      SPDLOG_WARN("setting not found: {}", path);
    }
    return value;
  }

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};
}  // namespace GpgFrontend
