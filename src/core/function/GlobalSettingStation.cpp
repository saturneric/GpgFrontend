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

#include "GlobalSettingStation.h"

#include <set>

#include "GpgFrontendBuildInstallInfo.h"
#include "core/module/ModuleManager.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/FilesystemUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

class GlobalSettingStation::Impl {
 public:
  /**
   * @brief Construct a new Global Setting Station object
   *
   */
  explicit Impl() noexcept {
    // Reading the session rather than a qApp property is what enforces the
    // ordering rule: anything that reaches for the settings station before the
    // profile is loaded dies here, loudly, in every build configuration, rather
    // than quietly resolving against the wrong directory.
    const auto& session = ProfileSession::Instance();

    // The application directory is AppImage-aware, because a portable install
    // anchors relative key database paths to it and applicationDirPath() points
    // inside the read-only mount.
    app_path_ = ResolveApplicationDirPath();

    LOG_I() << "app path: " << app_path_;
    LOG_I() << "app working path: " << working_path_;
    LOG_I() << "profile: " << session.Profile().Id() << " ("
            << ProfileKindToString(session.Profile().Kind()) << ")";
    LOG_I() << "profile storage driver: " << session.Accessor().Driver();

    portable_mode_ = session.Profile().Kind() == ProfileKind::kPORTABLE_ROOT;
    if (portable_mode_) {
      Module::UpsertRTValue("core", "env.state.portable", 1);
      LOG_I() << "GpgFrontend runs in the portable mode now";
    }

    LOG_I() << "app data path: " << GetAppDataPath();
    LOG_I() << "app secure path: " << accessor().PathOf(ProfileArea::kSecure);
    LOG_I() << "app log path: " << accessor().PathOf(ProfileArea::kLogs);
    LOG_I() << "app modules path: " << accessor().PathOf(ProfileArea::kModules);
  }

  [[nodiscard]] auto GetSettings() const -> QSettings {
    return accessor().Settings();
  }

  [[nodiscard]] auto GetLogFilesSize() const -> QString {
    return GetHumanFriendlyFileSize(
        accessor().TotalSize(ProfileArea::kLogs, "*.log"));
  }

  [[nodiscard]] auto GetDataObjectsFilesSize() const -> QString {
    return GetHumanFriendlyFileSize(
        accessor().TotalSize(ProfileArea::kDataObjects, "*"));
  }

  void ClearAllLogFiles() const {
    for (const auto& name : accessor().List(ProfileArea::kLogs, "*.log")) {
      accessor().Remove(ProfileArea::kLogs, name);
    }
  }

  void ClearAllDataObjects() const {
    for (const auto& name : accessor().List(ProfileArea::kDataObjects, "*")) {
      accessor().Remove(ProfileArea::kDataObjects, name);
    }
  }

  /**
   * @brief Get the App Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetAppDir() const -> QString { return app_path_; }

  /**
   * @brief Get the App Data Path object
   *
   * @return QString
   */
  [[nodiscard]] auto GetAppDataPath() const -> QString {
    return accessor().PathOf(ProfileArea::kRoot);
  }

  /**
   * @brief Get the Log Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetLogDir() const -> QString {
    return accessor().PathOf(ProfileArea::kLogs);
  }

  /**
   * @brief Get the Config Path object
   *
   * @return QString
   */
  [[nodiscard]] auto GetConfigPath() const -> QString {
    return accessor().PathOf(ProfileArea::kConfig, "config.ini");
  }

  /**
   * @brief Get the Modules Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetModulesDir() const -> QString {
    return accessor().PathOf(ProfileArea::kModules);
  }

  [[nodiscard]] auto GetIntegratedModulePath() const -> QString {
    const auto exec_binary_path = GetAppDir();

#ifdef Q_OS_LINUX
    // AppImage
    if (IsAppImageENV()) {
      return qEnvironmentVariable("APPDIR") + "/usr/lib/modules";
    }
    // Flatpak
    if (IsFlatpakENV()) {
      return "/app/lib/gpgfrontend/modules";
    }
#endif

#ifdef Q_OS_WINDOWS
    return exec_binary_path + "/../modules";
#endif

#ifdef Q_OS_MACOS

#ifndef GF_BUILD_DEBUG
    return exec_binary_path + "/../PlugIns";
#endif

#endif

    // Package or Install
    auto module_install_path = QString(APP_LIB_PATH) + "/gpgfrontend/modules";
    if (QFileInfo(module_install_path).exists()) {
      return module_install_path;
    }

    return exec_binary_path + "/modules";
  }

  [[nodiscard]] auto IsProtableMode() const -> bool { return portable_mode_; }

  [[nodiscard]] auto GetDataObjectsPath() const -> QString {
    return accessor().PathOf(ProfileArea::kDataObjects);
  }

  [[nodiscard]] auto GetConfigDirPath() const -> QString {
    return accessor().PathOf(ProfileArea::kConfig);
  }

  auto IsEngineSupported(OpenPGPEngine engine) -> bool {
    return supported_engines_.count(engine) > 0;
  }

  auto AddSupportedOpenPPGEngine(OpenPGPEngine engine) -> void {
    supported_engines_.insert(engine);
  }

  auto RemoveSupportedOpenPPGEngine(OpenPGPEngine engine) -> void {
    supported_engines_.erase(engine);
  }

  auto HasSupportedEngine() -> bool { return !supported_engines_.empty(); }

  [[nodiscard]] auto AllSupportedEngines() const -> QStringList {
    QStringList engines;
    for (const auto& engine : supported_engines_) {
      engines.append(ConvertOpenPGPEngine2String(engine));
    }
    return engines;
  }

 private:
  /// Every path this class reports comes from here, so there is exactly one
  /// answer to "where does the application keep its data" in the process.
  [[nodiscard]] static auto accessor() -> ProfileAccessor& {
    return ProfileSession::Instance().Accessor();
  }

  bool portable_mode_ = false;
  QString app_path_ = QCoreApplication::applicationDirPath();
  QString working_path_ = QDir::currentPath();

  // we do not use QSet because it has different behavior on qt5 and qt6, which
  // causes compile error on qt5. using std::set for better compatibility.
  std::set<OpenPGPEngine> supported_engines_;
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

[[nodiscard]] auto GlobalSettingStation::GetAppLogPath() const -> QString {
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

auto GlobalSettingStation::GetConfigPath() const -> QString {
  return p_->GetConfigPath();
}

auto GlobalSettingStation::GetIntegratedModulePath() const -> QString {
  return p_->GetIntegratedModulePath();
}

auto GlobalSettingStation::IsProtableMode() const -> bool {
  return p_->IsProtableMode();
}

auto GlobalSettingStation::GetDataObjectsDir() const -> QString {
  return p_->GetDataObjectsPath();
}

auto GlobalSettingStation::GetConfigDirPath() const -> QString {
  return p_->GetConfigDirPath();
}

auto GlobalSettingStation::IsEngineSupported(OpenPGPEngine engine) -> bool {
  return p_->IsEngineSupported(engine);
}

auto GlobalSettingStation::AddSupportedEngine(OpenPGPEngine engine) -> void {
  p_->AddSupportedOpenPPGEngine(engine);
}

auto GlobalSettingStation::IsSelfContainedProfile() const -> bool {
  return ProfileSession::Loaded() &&
         ProfileSession::Instance().Profile().Policy().self_contained;
}

auto GlobalSettingStation::RemoveSupportedEngine(OpenPGPEngine engine) -> void {
  p_->RemoveSupportedOpenPPGEngine(engine);
}

auto GlobalSettingStation::HasSupportedEngine() -> bool {
  return p_->HasSupportedEngine();
}
auto GlobalSettingStation::AllSupportedEngines() -> QStringList {
  return p_->AllSupportedEngines();
}

auto GetGSS() -> GlobalSettingStation& {
  return GlobalSettingStation::GetInstance();
}

auto GetSettings() -> QSettings { return GetGSS().GetSettings(); }

}  // namespace GpgFrontend