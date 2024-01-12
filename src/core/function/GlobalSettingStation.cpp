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

#include <filesystem>

#include "core/module/ModuleManager.h"
#include "core/utils/FilesystemUtils.h"

namespace GpgFrontend {

class GlobalSettingStation::Impl {
 public:
  /**
   * @brief Construct a new Global Setting Station object
   *
   */
  explicit Impl() noexcept {
    GF_CORE_LOG_INFO("app path: {}", working_path_);
    auto portable_file_path = working_path_ + "/PORTABLE.txt";
    if (QFileInfo(portable_file_path).exists()) {
      GF_CORE_LOG_INFO(
          "dectected portable mode, reconfiguring config and data path...");
      Module::UpsertRTValue("core", "env.state.portable", 1);

      app_configure_path_ = working_path_;
      config_dir_path_ = app_configure_path_ + "/conf";
      main_config_path_ = config_dir_path_ + "/main.cfg";
      module_config_path_ = config_dir_path_ + "/module.cfg";

      app_data_path_ = working_path_;
      app_log_path_ = app_data_path_ + "/logs";
      app_data_objs_path_ = app_data_path_ + "/data_objs";
    }

    GF_CORE_LOG_INFO("app configure path: {}", app_configure_path_);
    GF_CORE_LOG_INFO("app data path: {}", app_data_path_);
    GF_CORE_LOG_INFO("app log path: {}", app_log_path_);
    GF_CORE_LOG_INFO("app locale path: {}", app_locale_path_);
    GF_CORE_LOG_INFO("app conf path: {}", main_config_path_);

    GF_CORE_LOG_INFO("app log files total size: {}", GetLogFilesSize());
    GF_CORE_LOG_INFO("app data objects files total size: {}",
                     GetDataObjectsFilesSize());

    if (!QDir(app_configure_path_).exists()) {
      QDir(app_configure_path_).mkpath(".");
    }
    if (!QDir(app_data_path_).exists()) QDir(app_data_path_).mkpath(".");
    if (!QDir(app_log_path_).exists()) QDir(app_log_path_).mkpath(".");
    if (!QDir(config_dir_path_).exists()) QDir(config_dir_path_).mkpath(".");

    if (!QDir(main_config_path_).exists()) {
      try {
        this->ui_cfg_.writeFile(main_config_path_.toUtf8());
        GF_CORE_LOG_DEBUG(
            "user interface configuration successfully written to {}",
            main_config_path_);

      } catch (const libconfig::FileIOException &fioex) {
        GF_CORE_LOG_DEBUG(
            "i/o error while writing UserInterface configuration file {}",
            main_config_path_);
      }
    } else {
      try {
        this->ui_cfg_.readFile(main_config_path_.toUtf8());
        GF_CORE_LOG_DEBUG(
            "user interface configuration successfully read from {}",
            main_config_path_);
      } catch (const libconfig::FileIOException &fioex) {
        GF_CORE_LOG_ERROR(
            "i/o error while reading UserInterface configure file");
      } catch (const libconfig::ParseException &pex) {
        GF_CORE_LOG_ERROR("parse error at {} : {} - {}", pex.getFile(),
                          pex.getLine(), pex.getError());
      }
    }
  }

  auto GetMainSettings() noexcept -> libconfig::Setting & {
    return ui_cfg_.getRoot();
  }

  [[nodiscard]] auto GetLogFilesSize() const -> QString {
    return GetHumanFriendlyFileSize(GetFileSizeByPath(app_log_path_, "*.log"));
  }

  [[nodiscard]] auto GetDataObjectsFilesSize() const -> QString {
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
  auto LookupSettings(QString path, T default_value) noexcept -> T {
    T value = default_value;
    try {
      value = static_cast<T>(GetMainSettings().lookup(path.toStdString()));
    } catch (...) {
      GF_CORE_LOG_WARN("setting not found: {}", path);
    }
    return value;
  }

  /**
   * @brief Get the App Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetAppDir() const -> QString { return working_path_; }

  [[nodiscard]] auto GetAppDataPath() const -> QString {
    return app_data_path_;
  }

  /**
   * @brief Get the Log Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetLogDir() const -> QString { return app_log_path_; }

  /**
   * @brief Get the App Config Path object
   *
   * @return QString
   */
  [[nodiscard]] auto GetAppConfigPath() const -> QString {
    return app_configure_path_;
  }

  /**
   * @brief Get the Locale Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetLocaleDir() const -> QString {
    return app_locale_path_;
  }

  /**
   * @brief Get the Resource Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetResourceDir() const -> QString {
    return app_resource_path_;
  }

  /**
   * @brief Get the Certs Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetCertsDir() const -> QString {
    return app_resource_path_ + "/certs";
  }

  /**
   * @brief sync the settings to the file
   *
   */
  void SyncSettings() noexcept {
    try {
      ui_cfg_.writeFile(main_config_path_.toUtf8());
      GF_CORE_LOG_DEBUG("updated ui configuration successfully written to {}",
                        main_config_path_);

    } catch (const libconfig::FileIOException &fioex) {
      GF_CORE_LOG_ERROR("i/o error while writing ui configuration file: {}",
                        main_config_path_);
    }
  }

 private:
  QString working_path_ = QDir::currentPath();

  QString app_data_path_ = QString{QStandardPaths::writableLocation(
                               QStandardPaths::AppLocalDataLocation)} +
                           "/GpgFrontend";  ///< Program Data Location

  QString app_log_path_ = app_data_path_ + "/logs";  ///< Program Data Location

  QString app_data_objs_path_ =
      app_data_path_ + "/data_objs";  ///< Object storage path

#ifdef LINUX_INSTALL_BUILD
  QString app_resource_path_ =
      QString(APP_LOCALSTATE_PATH) / "gpgfrontend";  ///< Program Data Location
#else
  QString app_resource_path_ =
      RESOURCE_DIR_BOOST_PATH(working_path_);  ///< Program Data Location
#endif

#ifdef LINUX_INSTALL_BUILD
  QString app_locale_path_ =
      QString(APP_LOCALE_PATH);  ///< Program Data Location
#else
  QString app_locale_path_ =
      app_resource_path_ + "/locales";  ///< Program Data Location
#endif

  QString app_configure_path_ = QString{QStandardPaths::writableLocation(
                                    QStandardPaths::AppConfigLocation)} +
                                "/GpgFrontend";  ///< Program Configure Location
  QString config_dir_path_ =
      app_configure_path_ + "/conf";  ///< Configure File Directory Location
  QString main_config_path_ =
      config_dir_path_ + "/main.cfg";  ///< Main Configure File Location
  QString module_config_path_ =
      config_dir_path_ + "/module.cfg";  ///< Main Configure File Location

  libconfig::Config ui_cfg_;  ///< UI Configure File

  /**
   * @brief
   *
   */
  void init_app_secure_key() {}
};

GlobalSettingStation::GlobalSettingStation(int channel) noexcept
    : SingletonFunctionObject<GlobalSettingStation>(channel),
      p_(SecureCreateUniqueObject<Impl>()) {}

GlobalSettingStation::~GlobalSettingStation() noexcept = default;

void GlobalSettingStation::SyncSettings() noexcept { p_->SyncSettings(); }

auto GlobalSettingStation::GetMainSettings() noexcept -> libconfig::Setting & {
  return p_->GetMainSettings();
}

auto GlobalSettingStation::GetAppDir() const -> QString {
  return p_->GetAppDir();
}

auto GlobalSettingStation::GetAppDataPath() const -> QString {
  return p_->GetAppDataPath();
}

[[nodiscard]] auto GlobalSettingStation::GetLogDir() const -> QString {
  return p_->GetLogDir();
}

auto GlobalSettingStation::GetAppConfigPath() const -> QString {
  return p_->GetAppConfigPath();
}

auto GlobalSettingStation::GetLocaleDir() const -> QString {
  return p_->GetLocaleDir();
}

auto GlobalSettingStation::GetResourceDir() const -> QString {
  return p_->GetResourceDir();
}

auto GlobalSettingStation::GetCertsDir() const -> QString {
  return p_->GetCertsDir();
}

auto GlobalSettingStation::GetLogFilesSize() const -> QString {
  return p_->GetLogFilesSize();
}

auto GlobalSettingStation::GetDataObjectsFilesSize() const -> QString {
  return p_->GetDataObjectsFilesSize();
}

void GlobalSettingStation::ClearAllLogFiles() const { p_->ClearAllLogFiles(); }

void GlobalSettingStation::ClearAllDataObjects() const {
  p_->ClearAllDataObjects();
}

}  // namespace GpgFrontend