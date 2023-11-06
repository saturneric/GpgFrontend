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

#include "GpgFrontendBuildInstallInfo.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

class GlobalSettingStation::Impl {
 public:
  /**
   * @brief Construct a new Global Setting Station object
   *
   */
  explicit Impl(int channel) noexcept {
    using namespace std::filesystem;
    using namespace libconfig;

    SPDLOG_INFO("app path: {}", app_path_.u8string());
    SPDLOG_INFO("app configure path: {}", app_configure_path_.u8string());
    SPDLOG_INFO("app data path: {}", app_data_path_.u8string());
    SPDLOG_INFO("app log path: {}", app_log_path_.u8string());
    SPDLOG_INFO("app locale path: {}", app_locale_path_.u8string());
    SPDLOG_INFO("app conf path: {}", main_config_path_.u8string());

    SPDLOG_INFO("app log files total size: {}", GetLogFilesSize());
    SPDLOG_INFO("app data objects files total size: {}",
                GetDataObjectsFilesSize());

    if (!is_directory(app_configure_path_))
      create_directory(app_configure_path_);
    if (!is_directory(app_data_path_)) create_directory(app_data_path_);
    if (!is_directory(app_log_path_)) create_directory(app_log_path_);
    if (!is_directory(config_dir_path_)) create_directory(config_dir_path_);

    if (!exists(main_config_path_)) {
      try {
        this->ui_cfg_.writeFile(main_config_path_.u8string().c_str());
        SPDLOG_DEBUG("user interface configuration successfully written to {}",
                     main_config_path_.u8string());

      } catch (const FileIOException &fioex) {
        SPDLOG_DEBUG(
            "i/o error while writing UserInterface configuration file {}",
            main_config_path_.u8string());
      }
    } else {
      try {
        this->ui_cfg_.readFile(main_config_path_.u8string().c_str());
        SPDLOG_DEBUG("user interface configuration successfully read from {}",
                     main_config_path_.u8string());
      } catch (const FileIOException &fioex) {
        SPDLOG_ERROR("i/o error while reading UserInterface configure file");
      } catch (const ParseException &pex) {
        SPDLOG_ERROR("parse error at {} : {} - {}", pex.getFile(),
                     pex.getLine(), pex.getError());
      }
    }
  }

  libconfig::Setting &GetMainSettings() noexcept { return ui_cfg_.getRoot(); }

  std::string GetLogFilesSize() const {
    return get_human_readable_size(
        get_files_size_at_path(app_log_path_, "*.log"));
  }

  std::string GetDataObjectsFilesSize() const {
    return get_human_readable_size(
        get_files_size_at_path(app_data_objs_path_, "*"));
  }

  void ClearAllLogFiles() const { delete_all_files(app_log_path_, "*.log"); }

  void ClearAllDataObjects() const {
    delete_all_files(app_data_objs_path_, "*");
  }

  /**
   * @brief
   *
   * @return libconfig::Setting&
   */
  template <typename T>
  T LookupSettings(std::string path, T default_value) noexcept {
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

  /**
   * @brief sync the settings to the file
   *
   */
  void SyncSettings() noexcept {
    using namespace libconfig;
    try {
      ui_cfg_.writeFile(main_config_path_.u8string().c_str());
      SPDLOG_DEBUG("updated ui configuration successfully written to {}",
                   main_config_path_.u8string());

    } catch (const FileIOException &fioex) {
      SPDLOG_ERROR("i/o error while writing ui configuration file: {}",
                   main_config_path_.u8string());
    }
  }

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

  /**
   * @brief
   *
   */
  int64_t get_files_size_at_path(std::filesystem::path path,
                                 std::string filename_pattern) const {
    auto dir = QDir(QString::fromStdString(path.u8string()));
    QFileInfoList fileList = dir.entryInfoList(
        QStringList() << QString::fromStdString(filename_pattern), QDir::Files);
    qint64 totalSize = 0;

    for (const QFileInfo &fileInfo : fileList) {
      totalSize += fileInfo.size();
    }
    return totalSize;
  }

  /**
   * @brief
   *
   */
  std::string get_human_readable_size(int64_t size) const {
    double num = size;
    QStringList list;
    list << "KB"
         << "MB"
         << "GB"
         << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while (num >= 1024.0 && i.hasNext()) {
      unit = i.next();
      num /= 1024.0;
    }
    return (QString().setNum(num, 'f', 2) + " " + unit).toStdString();
  }

  /**
   * @brief
   *
   */
  void delete_all_files(std::filesystem::path path,
                        std::string filename_pattern) const {
    auto dir = QDir(QString::fromStdString(path.u8string()));

    QStringList logFiles = dir.entryList(
        QStringList() << QString::fromStdString(filename_pattern), QDir::Files);

    for (const auto &file : logFiles) {
      QFile::remove(dir.absoluteFilePath(file));
    }
  }
};

GlobalSettingStation::GlobalSettingStation(int channel) noexcept
    : SingletonFunctionObject<GlobalSettingStation>(channel),
      p_(std::make_unique<Impl>(channel)) {}

GlobalSettingStation::~GlobalSettingStation() noexcept = default;

void GlobalSettingStation::SyncSettings() noexcept { p_->SyncSettings(); }

libconfig::Setting &GlobalSettingStation::GetMainSettings() noexcept {
  return p_->GetMainSettings();
}

std::filesystem::path GlobalSettingStation::GetAppDir() const {
  return p_->GetAppDir();
}

std::filesystem::path GlobalSettingStation::GetAppDataPath() const {
  return p_->GetAppDataPath();
}

[[nodiscard]] std::filesystem::path GlobalSettingStation::GetLogDir() const {
  return p_->GetLogDir();
}

std::filesystem::path GlobalSettingStation::GetStandaloneDatabaseDir() const {
  return p_->GetStandaloneDatabaseDir();
}

std::filesystem::path GlobalSettingStation::GetAppConfigPath() const {
  return p_->GetAppConfigPath();
}

std::filesystem::path GlobalSettingStation::GetStandaloneGpgBinDir() const {
  return p_->GetStandaloneGpgBinDir();
}

std::filesystem::path GlobalSettingStation::GetLocaleDir() const {
  return p_->GetLocaleDir();
}

std::filesystem::path GlobalSettingStation::GetResourceDir() const {
  return p_->GetResourceDir();
}

std::filesystem::path GlobalSettingStation::GetCertsDir() const {
  return p_->GetCertsDir();
}

std::string GlobalSettingStation::GetLogFilesSize() const {
  return p_->GetLogFilesSize();
}

std::string GlobalSettingStation::GetDataObjectsFilesSize() const {
  return p_->GetDataObjectsFilesSize();
}

void GlobalSettingStation::ClearAllLogFiles() const { p_->ClearAllLogFiles(); }

void GlobalSettingStation::ClearAllDataObjects() const {
  p_->ClearAllDataObjects();
}

}  // namespace GpgFrontend