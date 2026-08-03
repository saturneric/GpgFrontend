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
#include "core/utils/FilesystemUtils.h"
#include "core/utils/GpgUtils.h"

#ifdef Q_OS_LINUX
#include "core/utils/CommonUtils.h"
#endif

namespace GpgFrontend {

auto ResolveSettingsFilePath(ProfileRootKind kind, const QString& root)
    -> QString {
  if (kind != ProfileRootKind::kCLASSIC) return root + "/config/config.ini";

#ifdef Q_OS_WINDOWS
  return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
         "/config.ini";
#else
  // Classic on POSIX keeps Qt's native store, keyed by the organization and
  // application name set in GpgFrontendApplication's constructor. Migrating it
  // would silently orphan every existing installation's settings.
  return {};
#endif
}

class GlobalSettingStation::Impl {
 public:
  /**
   * @brief Construct a new Global Setting Station object
   *
   */
  explicit Impl() noexcept {
    // Reading the runtime rather than a qApp property is what enforces the
    // ordering rule: anything that reaches for the settings station before the
    // profile is resolved dies here, loudly, in every build configuration,
    // rather than quietly resolving against the wrong directory.
    const auto& profile = ProfileRuntime::Instance();
    kind_ = profile.kind;

    // The application directory is AppImage-aware, because a portable install
    // anchors relative key database paths to it and applicationDirPath() points
    // inside the read-only mount.
    app_path_ = ResolveApplicationDirPath();

    LOG_I() << "app path: " << app_path_;
    LOG_I() << "app working path: " << working_path_;
    LOG_I() << "profile: " << profile.id << " ("
            << ProfileRootKindToString(profile.kind) << ")";

    portable_mode_ = profile.kind == ProfileRootKind::kPORTABLE;
    if (portable_mode_) {
      Module::UpsertRTValue("core", "env.state.portable", 1);
      LOG_I() << "GpgFrontend runs in the portable mode now";
    }

    if (profile.kind != ProfileRootKind::kCLASSIC) {
      app_data_path_ = RequireProfileRoot(profile);
      app_config_path_ = app_data_path_ + "/config";
    }

    LOG_I() << "app data path: " << app_data_path_;
    LOG_I() << "app secure path: " << app_secure_path();
    LOG_I() << "app log path: " << app_log_path();
    LOG_I() << "app modules path: " << app_mods_path();

    // The config directory only needs creating where an INI file actually
    // lands in it; a classic POSIX profile writes through the native store and
    // has no directory of its own to make.
    if (!ResolveSettingsFilePath(kind_, app_data_path_).isEmpty()) {
      LOG_I() << "app config path: " << app_config_path_;
      if (!QDir(app_config_path_).exists()) QDir(app_config_path_).mkpath(".");
    }

    if (!QDir(app_data_path_).exists()) QDir(app_data_path_).mkpath(".");
    if (!QDir(app_log_path()).exists()) QDir(app_log_path()).mkpath(".");
    if (!QDir(app_secure_path()).exists()) QDir(app_secure_path()).mkpath(".");
    if (!QDir(app_mods_path()).exists()) QDir(app_mods_path()).mkpath(".");
    if (!QDir(app_data_objs_path()).exists()) {
      QDir(app_data_objs_path()).mkpath(".");
    }
  }

  [[nodiscard]] auto GetSettings() -> QSettings {
    const auto path = ResolveSettingsFilePath(kind_, app_data_path_);
    if (path.isEmpty()) return QSettings();
    return {path, QSettings::IniFormat};
  }

  [[nodiscard]] auto GetLogFilesSize() const -> QString {
    return GetHumanFriendlyFileSize(GetFileSizeByPath(app_log_path(), "*.log"));
  }

  [[nodiscard]] auto GetDataObjectsFilesSize() const -> QString {
    return GetHumanFriendlyFileSize(
        GetFileSizeByPath(app_data_objs_path(), "*"));
  }

  void ClearAllLogFiles() const {
    DeleteAllFilesByPattern(app_log_path(), "*.log");
  }

  void ClearAllDataObjects() const {
    DeleteAllFilesByPattern(app_data_objs_path(), "*");
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
    return app_data_path_;
  }

  /**
   * @brief Get the Log Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetLogDir() const -> QString { return app_log_path(); }

  /**
   * @brief Get the Config Path object
   *
   * @return QString
   */
  [[nodiscard]] auto GetConfigPath() const -> QString {
    return app_config_file_path();
  }

  /**
   * @brief Get the Modules Dir object
   *
   * @return QString
   */
  [[nodiscard]] auto GetModulesDir() const -> QString {
    return app_mods_path();
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
    return app_data_objs_path();
  }

  [[nodiscard]] auto GetConfigDirPath() const -> QString {
    return app_config_path_;
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
  [[nodiscard]] auto app_config_file_path() const -> QString {
    return app_config_path_ + "/config.ini";
  }

  [[nodiscard]] auto app_data_objs_path() const -> QString {
    return app_data_path_ + "/data_objs";
  }

  [[nodiscard]] auto app_log_path() const -> QString {
    return app_data_path_ + "/logs";
  }

  [[nodiscard]] auto app_mods_path() const -> QString {
    return app_data_path_ + "/mods";
  }

  [[nodiscard]] auto app_secure_path() const -> QString {
    return app_data_path_ + "/secure";
  }

  [[nodiscard]] auto app_secure_key_path() const -> QString {
    return app_secure_path() + "/app.key";
  }

  bool portable_mode_ = false;
  ProfileRootKind kind_ = ProfileRootKind::kCLASSIC;
  QString app_path_ = QCoreApplication::applicationDirPath();
  QString working_path_ = QDir::currentPath();
  QString app_data_path_ = QString{
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)};
  QString app_config_path_ = QString{
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)};

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
  return ProfileRuntime::Established() &&
         ProfileRuntime::Instance().policy.self_contained;
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

auto GetEarlySettings() -> QSettings {
  // Deliberately singleton-free: this runs during InitApplication(), long
  // before InitAppSecureKey(), and GlobalSettingStation allocates its Impl
  // through the secure allocator. Constructing the singleton here would pull
  // the secure allocator and the module manager up before the secure key
  // exists. Resolve the very same file by hand instead — through the same
  // ResolveSettingsFilePath() the singleton uses, so the two cannot disagree.
  if (!ProfileRuntime::Established()) {
    // Only reachable from a harness that never bootstrapped a profile. The
    // classic store is the one location that is correct by default.
    return QSettings();
  }

  const auto& profile = ProfileRuntime::Instance();
  const auto path = ResolveSettingsFilePath(profile.kind, profile.root);
  if (path.isEmpty()) return QSettings();
  return {path, QSettings::IniFormat};
}

auto ProfileMarkerPathFor(const QString& profile_root) -> QString {
  return profile_root + "/profile.json";
}

auto CurrentProfileMarkerPath() -> QString {
  return ProfileMarkerPathFor(GetGSS().GetAppDataPath());
}

auto CheckProfileCompatibility(const ProfileMarker& marker, bool marker_present,
                               int this_schema_version)
    -> ProfileCompatibility {
  if (!marker_present) return ProfileCompatibility::kMISSING;

  if (marker.min_reader_version > this_schema_version) {
    return ProfileCompatibility::kTOO_NEW;
  }

  return ProfileCompatibility::kOK;
}

auto ReadProfileMarker(const QString& path) -> std::optional<ProfileMarker> {
  QFile file(path);
  if (!file.exists()) return {};

  if (!file.open(QIODevice::ReadOnly)) {
    LOG_W() << "cannot open profile marker:" << path;
    return {};
  }

  QJsonParseError error{};
  const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject()) {
    LOG_W() << "profile marker is not valid json:" << path
            << error.errorString();
    return {};
  }

  const auto obj = doc.object();

  ProfileMarker marker;
  marker.schema_version = obj.value("schema_version").toInt();
  marker.min_reader_version = obj.value("min_reader_version").toInt();
  marker.profile = obj.value("profile").toString();
  marker.last_writer_version = obj.value("last_writer_version").toString();
  marker.last_writer_stable = obj.value("last_writer_stable").toBool();

  marker.profile_uuid = obj.value("profile_uuid").toString();
  marker.profile_id = obj.value("profile_id").toString();
  marker.display_name = obj.value("display_name").toString();
  marker.created = obj.value("created").toString();
  marker.created_by_version = obj.value("created_by_version").toString();
  marker.kind = obj.value("kind").toString();
  marker.package_id = obj.value("package_id").toString();
  marker.credential_account = obj.value("credential_account").toString();

  const auto components = obj.value("components").toObject();
  for (auto it = components.constBegin(); it != components.constEnd(); ++it) {
    marker.components.insert(it.key(), it.value().toInt());
  }

  marker.self_contained =
      obj.value("policy").toObject().value("self_contained").toBool();

  for (const auto& entry : obj.value("migrations").toArray()) {
    const auto e = entry.toObject();
    ProfileMigrationRecord record;
    record.from = e.value("from").toInt();
    record.to = e.value("to").toInt();
    record.name = e.value("name").toString();
    record.at = e.value("at").toString();
    record.by = e.value("by").toString();
    record.skipped = e.value("skipped").toBool();
    record.reason = e.value("reason").toString();
    marker.migrations.append(record);
  }

  // Anything this build does not know is carried through untouched, so opening
  // a profile with an older version never silently discards what a newer one
  // depends on.
  static const QSet<QString> kKnown = {"schema_version",
                                       "min_reader_version",
                                       "profile",
                                       "last_writer_version",
                                       "last_writer_stable",
                                       "profile_uuid",
                                       "profile_id",
                                       "display_name",
                                       "created",
                                       "created_by_version",
                                       "kind",
                                       "package_id",
                                       "credential_account",
                                       "components",
                                       "policy",
                                       "migrations"};
  for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
    if (!kKnown.contains(it.key()))
      marker.unknown_fields[it.key()] = it.value();
  }

  return marker;
}

auto WriteProfileMarker(const QString& path, const ProfileMarker& marker)
    -> bool {
  // unknown keys first, so a known field always wins a collision rather than
  // being shadowed by a stale copy of itself
  QJsonObject obj = marker.unknown_fields;

  obj["schema_version"] = marker.schema_version;
  obj["min_reader_version"] = marker.min_reader_version;
  obj["profile"] = marker.profile;
  obj["last_writer_version"] = marker.last_writer_version;
  obj["last_writer_stable"] = marker.last_writer_stable;

  const auto put = [&obj](const char* key, const QString& value) {
    if (!value.isEmpty()) obj[QLatin1String(key)] = value;
  };
  put("profile_uuid", marker.profile_uuid);
  put("profile_id", marker.profile_id);
  put("display_name", marker.display_name);
  put("created", marker.created);
  put("created_by_version", marker.created_by_version);
  put("kind", marker.kind);
  put("package_id", marker.package_id);
  put("credential_account", marker.credential_account);

  if (!marker.components.isEmpty()) {
    QJsonObject components;
    for (auto it = marker.components.constBegin();
         it != marker.components.constEnd(); ++it) {
      components[it.key()] = it.value();
    }
    obj["components"] = components;
  }

  QJsonObject policy;
  policy["self_contained"] = marker.self_contained;
  obj["policy"] = policy;

  if (!marker.migrations.isEmpty()) {
    QJsonArray migrations;
    for (const auto& record : marker.migrations) {
      QJsonObject e;
      e["from"] = record.from;
      e["to"] = record.to;
      e["name"] = record.name;
      e["at"] = record.at;
      e["by"] = record.by;
      if (record.skipped) {
        e["skipped"] = true;
        e["reason"] = record.reason;
      }
      migrations.append(e);
    }
    obj["migrations"] = migrations;
  }

  const auto payload = QJsonDocument(obj).toJson(QJsonDocument::Indented);

  // QSaveFile rather than a truncating write: this file is the only record of
  // how far a migration got, and a half-written one after a power cut would
  // make the ladder restart against already-migrated data.
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    LOG_W() << "cannot write profile marker:" << path;
    return false;
  }
  if (file.write(payload) != payload.size()) {
    file.cancelWriting();
    LOG_W() << "short write on profile marker:" << path;
    return false;
  }
  return file.commit();
}

}  // namespace GpgFrontend