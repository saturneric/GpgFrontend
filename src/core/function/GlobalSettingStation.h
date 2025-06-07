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
 * @class GlobalSettingStation
 * @brief Singleton class for managing global settings in the application.
 *
 * This class handles reading and writing of global settings, as well as
 * managing application directories and resource paths.
 */
class GF_CORE_EXPORT GlobalSettingStation
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
   * @brief Get the Settings object
   *
   * @return QSettings
   */
  [[nodiscard]] auto GetSettings() const -> QSettings;

  /**
   * @brief Get the App Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetAppDir() const -> QString;

  /**
   * @brief Gets the application data directory.
   * @return Path to the application data directory.
   */
  [[nodiscard]] auto GetAppDataPath() const -> QString;

  /**
   * @brief Get the Log Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetAppLogPath() const -> QString;

  /**
   * @brief Get the Log Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetConfigPath() const -> QString;

  /**
   * @brief Get the Modules Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetModulesDir() const -> QString;

  /**
   * @brief Get the Data Objects Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetDataObjectsDir() const -> QString;

  /**
   * @brief Get the Log Files Size object
   *
   * @return QString
   */
  [[nodiscard]] auto GetLogFilesSize() const -> QString;

  /**
   * @brief Get the Data Objects Files Size object
   *
   * @return QString
   */
  [[nodiscard]] auto GetDataObjectsFilesSize() const -> QString;

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
   * @brief Get the Integrated Module Path object
   *
   * @return QString
   */
  [[nodiscard]] auto GetIntegratedModulePath() const -> QString;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsProtableMode() const -> bool;

  /**
   * @brief Get the App Secure Key object
   *
   * @return GFBuffer
   */
  auto GetAppSecureKey() -> GFBuffer;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

auto GF_CORE_EXPORT GetSettings() -> QSettings;

}  // namespace GpgFrontend
