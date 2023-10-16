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

#ifndef GPGFRONTEND_GLOBALSETTINGSTATION_H
#define GPGFRONTEND_GLOBALSETTINGSTATION_H

#include <filesystem>

#include "GpgFrontendBuildInstallInfo.h"
#include "core/GpgFrontendCore.h"
#include "core/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @brief
 *
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
  libconfig::Setting &GetUISettings() noexcept;

  /**
   * @brief
   *
   * @return libconfig::Setting&
   */
  template <typename T>
  T LookupSettings(std::string path, T default_value) noexcept {
    T value = default_value;
    try {
      value = static_cast<T>(GetUISettings().lookup(path));
    } catch (...) {
      SPDLOG_WARN("setting not found: {}", path);
    }
    return value;
  }

  /**
   * @brief Get the App Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] std::filesystem::path GetAppDir() const { return app_path_; }

  [[nodiscard]] std::filesystem::path GetAppDataPath() const {
    return app_data_path_;
  }

  /**
   * @brief Get the Log Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] std::filesystem::path GetLogDir() const {
    return app_log_path_;
  }

  /**
   * @brief Get the Standalone Database Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] std::filesystem::path GetStandaloneDatabaseDir() const {
    auto db_path = app_configure_path_ / "db";
    if (!std::filesystem::exists(db_path)) {
      std::filesystem::create_directory(db_path);
    }
    return db_path;
  }

  [[nodiscard]] std::filesystem::path GetAppConfigPath() const {
    return app_configure_path_;
  }

  /**
   * @brief Get the Standalone Gpg Bin Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] std::filesystem::path GetStandaloneGpgBinDir() const {
    return app_resource_path_ / "gpg1.4" / "gpg";
  }

  /**
   * @brief Get the Locale Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] std::filesystem::path GetLocaleDir() const {
    return app_locale_path_;
  }

  /**
   * @brief Get the Resource Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] std::filesystem::path GetResourceDir() const {
    return app_resource_path_;
  }

  /**
   * @brief Get the Certs Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] std::filesystem::path GetCertsDir() const {
    return app_resource_path_ / "certs";
  }

  [[nodiscard]] std::string GetLogFilesSize() const;

  [[nodiscard]] std::string GetDataObjectsFilesSize() const;

  void ClearAllLogFiles() const;

  void ClearAllDataObjects() const;

  /**
   * @brief sync the settings to the file
   *
   */
  void SyncSettings() noexcept;

 private:
  std::filesystem::path app_path_ = QCoreApplication::applicationDirPath()
                                        .toStdString();  ///< Program Location
  std::filesystem::path app_data_path_ =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
          .toStdString();  ///< Program Data Location
  std::filesystem::path app_log_path_ =
      app_data_path_ / "logs";  ///< Program Data Location
  std::filesystem::path app_data_objs_path_ =
      app_data_path_ / "data_objs";  ///< Object storage path

#ifdef LINUX_INSTALL_BUILD
  std::filesystem::path app_resource_path_ =
      std::filesystem::path(APP_LOCALSTATE_PATH) /
      "gpgfrontend";  ///< Program Data Location
#else
  std::filesystem::path app_resource_path_ =
      RESOURCE_DIR_BOOST_PATH(app_path_);  ///< Program Data Location
#endif

#ifdef LINUX_INSTALL_BUILD
  std::filesystem::path app_locale_path_ =
      std::string(APP_LOCALE_PATH);  ///< Program Data Location
#else
  std::filesystem::path app_locale_path_ =
      app_resource_path_ / "locales";  ///< Program Data Location
#endif

  std::filesystem::path app_configure_path_ =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
          .toStdString();  ///< Program Configure Location
  std::filesystem::path ui_config_dir_path_ =
      app_configure_path_ / "conf";  ///< Configure File Directory Location
  std::filesystem::path ui_config_path_ =
      ui_config_dir_path_ / "main.cfg";  ///< Main Configure File Location

  libconfig::Config ui_cfg_;  ///< UI Configure File

  /**
   * @brief
   *
   */
  void init_app_secure_key();

  /**
   * @brief
   *
   */
  int64_t get_files_size_at_path(std::filesystem::path path,
                                 std::string filename_pattern) const;

  /**
   * @brief
   *
   */
  std::string get_human_readable_size(int64_t size) const;

  /**
   * @brief
   *
   */
  void delete_all_files(std::filesystem::path path,
                        std::string filename_pattern) const;
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GLOBALSETTINGSTATION_H
