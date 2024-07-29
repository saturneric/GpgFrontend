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
    qCInfo(core) << "app path: " << GetAppDir();
    qCInfo(core) << "app working path: " << working_path_;

    auto portable_file_path = working_path_ + "/PORTABLE.txt";
    if (QFileInfo(portable_file_path).exists()) {
      Module::UpsertRTValue("core", "env.state.portable", 1);

      app_data_path_ = working_path_;
      app_log_path_ = app_data_path_ + "/logs";
      app_data_objs_path_ = app_data_path_ + "/data_objs";

      portable_mode_ = true;
    }

    qCInfo(core) << "app data path: " << app_data_path_;
    qCInfo(core) << "app log path: " << app_log_path_;

#ifdef WINDOWS
    qCInfo(core) << "app config path: " << app_config_path_;
#endif

#ifdef WINDOWS
    if (!QDir(app_config_path_).exists()) QDir(app_config_path_).mkpath(".");
#endif

    if (!QDir(app_data_path_).exists()) QDir(app_data_path_).mkpath(".");
    if (!QDir(app_log_path_).exists()) QDir(app_log_path_).mkpath(".");
    if (!QDir(GetModulesDir()).exists()) QDir(GetModulesDir()).mkpath(".");
  }

  [[nodiscard]] auto GetSettings() -> QSettings {
    if (!portable_mode_) {
#ifdef WINDOWS
      return QSettings(app_config_target_path_, QSettings::IniFormat);
#else
      return QSettings();
#endif
    }
    return {app_portable_config_path_, QSettings::IniFormat};
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
   * @brief Get the App Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetAppDir() const -> QString {
    return QCoreApplication::applicationDirPath();
  }

  /**
   * @brief Get the App Data Path object
   *
   * @return QString
   */
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
   * @brief Get the Modules Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetModulesDir() const -> QString {
    return GetAppDataPath() + "/mods";
  }

 private:
  QString working_path_ = QDir::currentPath();

  QString app_data_path_ = QString{QStandardPaths::writableLocation(
      QStandardPaths::AppLocalDataLocation)};  ///< Program Data Location

  QString app_config_path_ = QString{
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)};

  QString app_log_path_ = app_data_path_ + "/logs";  ///< Program Data Location

  QString app_data_objs_path_ =
      app_data_path_ + "/data_objs";  ///< Object storage path

  QString app_config_target_path_ =
      app_config_path_ + "/config.ini";  ///< take effect only in portable mode

  bool portable_mode_ = false;  ///<
  QString app_portable_config_path_ =
      working_path_ + "/config.ini";  ///< take effect only in portable mode

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

auto GlobalSettingStation::GetSettings() const -> QSettings {
  return p_->GetSettings();
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

[[nodiscard]] auto GlobalSettingStation::GetModulesDir() const -> QString {
  return p_->GetModulesDir();
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