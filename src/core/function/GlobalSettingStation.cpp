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

#include "GlobalSettingStation.h"

#include <boost/dll.hpp>
#include <filesystem>

#include "core/utils/FilesystemUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

class GlobalSettingStation::Impl {
 public:
  /**
   * @brief Construct a new Global Setting Station object
   *
   */
  explicit Impl() noexcept {
    SPDLOG_INFO("app path: {}", app_path_.u8string());
    SPDLOG_INFO("app configure path: {}", app_configure_path_.u8string());
    SPDLOG_INFO("app data path: {}", app_data_path_.u8string());
    SPDLOG_INFO("app log path: {}", app_log_path_.u8string());
    SPDLOG_INFO("app locale path: {}", app_locale_path_.u8string());
    SPDLOG_INFO("app conf path: {}", main_config_path_.u8string());

    SPDLOG_INFO("app log files total size: {}", GetLogFilesSize());
    SPDLOG_INFO("app data objects files total size: {}",
                GetDataObjectsFilesSize());

    if (!is_directory(app_configure_path_)) {
      create_directory(app_configure_path_);
    }
    if (!is_directory(app_data_path_)) create_directory(app_data_path_);
    if (!is_directory(app_log_path_)) create_directory(app_log_path_);
    if (!is_directory(config_dir_path_)) create_directory(config_dir_path_);

    if (!exists(main_config_path_)) {
      try {
        this->ui_cfg_.writeFile(main_config_path_.u8string().c_str());
        SPDLOG_DEBUG("user interface configuration successfully written to {}",
                     main_config_path_.u8string());

      } catch (const libconfig::FileIOException &fioex) {
        SPDLOG_DEBUG(
            "i/o error while writing UserInterface configuration file {}",
            main_config_path_.u8string());
      }
    } else {
      try {
        this->ui_cfg_.readFile(main_config_path_.u8string().c_str());
        SPDLOG_DEBUG("user interface configuration successfully read from {}",
                     main_config_path_.u8string());
      } catch (const libconfig::FileIOException &fioex) {
        SPDLOG_ERROR("i/o error while reading UserInterface configure file");
      } catch (const libconfig::ParseException &pex) {
        SPDLOG_ERROR("parse error at {} : {} - {}", pex.getFile(),
                     pex.getLine(), pex.getError());
      }
    }
  }

  auto GetMainSettings() noexcept -> libconfig::Setting & {
    return ui_cfg_.getRoot();
  }

  [[nodiscard]] auto GetLogFilesSize() const -> std::string {
    return GetHumanFriendlyFileSize(GetFileSizeByPath(app_log_path_, "*.log"));
  }

  [[nodiscard]] auto GetDataObjectsFilesSize() const -> std::string {
    return GetHumanFriendlyFileSize(
        GetFileSizeByPath(app_data_objs_path_, "*"));
  }

  void ClearAllLogFiles() const {
    DeleteAllFilesByPattern(app_log_path_, "*.log");
  }

  void ClearAllDataObjects() const {
    DeleteAllFilesByPattern(app_data_objs_path_, "*");
  }

  /**
   * @brief
   *
   * @return libconfig::Setting&
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

  /**
   * @brief Get the App Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetAppDir() const -> std::filesystem::path {
    return app_path_;
  }

  [[nodiscard]] auto GetAppDataPath() const -> std::filesystem::path {
    return app_data_path_;
  }

  /**
   * @brief Get the Log Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetLogDir() const -> std::filesystem::path {
    return app_log_path_;
  }

  /**
   * @brief Get the Standalone Database Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetStandaloneDatabaseDir() const -> std::filesystem::path {
    auto db_path = app_configure_path_ / "db";
    if (!std::filesystem::exists(db_path)) {
      std::filesystem::create_directory(db_path);
    }
    return db_path;
  }

  [[nodiscard]] auto GetAppConfigPath() const -> std::filesystem::path {
    return app_configure_path_;
  }

  /**
   * @brief Get the Standalone Gpg Bin Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetStandaloneGpgBinDir() const -> std::filesystem::path {
    return app_resource_path_ / "gpg1.4" / "gpg";
  }

  /**
   * @brief Get the Locale Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetLocaleDir() const -> std::filesystem::path {
    return app_locale_path_;
  }

  /**
   * @brief Get the Resource Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetResourceDir() const -> std::filesystem::path {
    return app_resource_path_;
  }

  /**
   * @brief Get the Certs Dir object
   *
   * @return std::filesystem::path
   */
  [[nodiscard]] auto GetCertsDir() const -> std::filesystem::path {
    return app_resource_path_ / "certs";
  }

  /**
   * @brief sync the settings to the file
   *
   */
  void SyncSettings() noexcept {
    try {
      ui_cfg_.writeFile(main_config_path_.u8string().c_str());
      SPDLOG_DEBUG("updated ui configuration successfully written to {}",
                   main_config_path_.u8string());

    } catch (const libconfig::FileIOException &fioex) {
      SPDLOG_ERROR("i/o error while writing ui configuration file: {}",
                   main_config_path_.u8string());
    }
  }

 private:
  std::filesystem::path app_path_ =
      std::filesystem::path(boost::dll::program_location().string())
          .parent_path();

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
  std::filesystem::path config_dir_path_ =
      app_configure_path_ / "conf";  ///< Configure File Directory Location
  std::filesystem::path main_config_path_ =
      config_dir_path_ / "main.cfg";  ///< Main Configure File Location
  std::filesystem::path module_config_path_ =
      config_dir_path_ / "module.cfg";  ///< Main Configure File Location

  libconfig::Config ui_cfg_;  ///< UI Configure File

  /**
   * @brief
   *
   */
  void init_app_secure_key() {}
};

GlobalSettingStation::GlobalSettingStation(int channel) noexcept
    : SingletonFunctionObject<GlobalSettingStation>(channel),
      p_(std::make_unique<Impl>()) {}

GlobalSettingStation::~GlobalSettingStation() noexcept = default;

void GlobalSettingStation::SyncSettings() noexcept { p_->SyncSettings(); }

auto GlobalSettingStation::GetMainSettings() noexcept -> libconfig::Setting & {
  return p_->GetMainSettings();
}

auto GlobalSettingStation::GetAppDir() const -> std::filesystem::path {
  return p_->GetAppDir();
}

auto GlobalSettingStation::GetAppDataPath() const -> std::filesystem::path {
  return p_->GetAppDataPath();
}

[[nodiscard]] auto GlobalSettingStation::GetLogDir() const
    -> std::filesystem::path {
  return p_->GetLogDir();
}

auto GlobalSettingStation::GetStandaloneDatabaseDir() const
    -> std::filesystem::path {
  return p_->GetStandaloneDatabaseDir();
}

auto GlobalSettingStation::GetAppConfigPath() const -> std::filesystem::path {
  return p_->GetAppConfigPath();
}

auto GlobalSettingStation::GetStandaloneGpgBinDir() const
    -> std::filesystem::path {
  return p_->GetStandaloneGpgBinDir();
}

auto GlobalSettingStation::GetLocaleDir() const -> std::filesystem::path {
  return p_->GetLocaleDir();
}

auto GlobalSettingStation::GetResourceDir() const -> std::filesystem::path {
  return p_->GetResourceDir();
}

auto GlobalSettingStation::GetCertsDir() const -> std::filesystem::path {
  return p_->GetCertsDir();
}

auto GlobalSettingStation::GetLogFilesSize() const -> std::string {
  return p_->GetLogFilesSize();
}

auto GlobalSettingStation::GetDataObjectsFilesSize() const -> std::string {
  return p_->GetDataObjectsFilesSize();
}

void GlobalSettingStation::ClearAllLogFiles() const { p_->ClearAllLogFiles(); }

void GlobalSettingStation::ClearAllDataObjects() const {
  p_->ClearAllDataObjects();
}

}  // namespace GpgFrontend