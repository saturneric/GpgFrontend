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

#include "core/function/ProfileBootstrap.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton managing application configuration and directory paths.
 *
 * Provides access to the platform-appropriate QSettings store and resolves all
 * application directory paths (data, logs, config, modules, secure storage),
 * creating them when absent. In portable mode, data is placed alongside the
 * executable instead of the OS user-data directory.
 *
 * The application secure key itself belongs to AppSecureKeyManager; this class
 * only provisions the directory it lives in.
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
   * @brief Whether this profile keeps its keys to itself.
   *
   * Portable mode's other half, now available to any profile. It decides
   * whether the default key database comes from the profile or from
   * `gpgconf --list-dirs homedir`, and whether database paths are recorded
   * relative to the profile.
   *
   * It deliberately says nothing about how the gpg binary is found: a
   * self-contained profile still runs a system or bundled gpg.
   *
   * @return true when the profile is self-contained
   */
  [[nodiscard]] auto IsSelfContainedProfile() const -> bool;

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
auto GF_CORE_EXPORT GetGSS() -> GlobalSettingStation &;

/**
 * @brief Convenience wrapper that returns the application QSettings via the
 * singleton.
 *
 * @return QSettings configured for the current platform and mode
 */
auto GF_CORE_EXPORT GetSettings() -> QSettings;

/**
 * @brief Open the application QSettings without constructing the singleton.
 *
 * Resolves the same settings file as GetSettings(), but is safe to call during
 * very early startup — before InitAppSecureKey() and the module manager exist,
 * where touching GlobalSettingStation would drag the secure allocator up too
 * soon. Requires ProfileRuntime to already be established. Read-only use only;
 * everything after early startup should use GetSettings().
 *
 * @return QSettings configured for the current platform and mode
 */
auto GF_CORE_EXPORT GetEarlySettings() -> QSettings;

/**
 * @brief Where the settings for a given profile live, or nowhere.
 *
 * A rooted profile is INI-backed on every platform. A native QSettings store is
 * keyed only by organization and application name, so every profile would share
 * one registry key or plist, and the file would sit outside the profile root
 * where no package could carry it.
 *
 * Shared by the singleton and by GetEarlySettings() so the two can never
 * disagree about which file they mean, and exported so the rule is assertable
 * on every platform rather than only where the branch happens to compile.
 *
 * @param kind which shape of profile is in effect
 * @param root the profile root; ignored for kCLASSIC
 * @return the INI path, or an empty string meaning "use the native store"
 */
auto GF_CORE_EXPORT ResolveSettingsFilePath(ProfileRootKind kind,
                                            const QString &root) -> QString;

/**
 * @brief Whether this build may safely use the profile it found.
 */
enum class ProfileCompatibility : std::uint8_t {
  kOK,         ///< safe to read and write
  kMISSING,    ///< first run, or a profile written before markers existed
  kTOO_NEW,    ///< written by a newer build; must not be touched
  kMALFORMED,  ///< present but unparsable
};

/**
 * @brief One applied — or deliberately skipped — migration rung.
 */
struct ProfileMigrationRecord {
  int from = 0;
  int to = 0;
  QString name;  ///< stable rung id
  QString at;    ///< ISO-8601 timestamp
  QString by;    ///< application version that ran it
  bool skipped = false;
  QString reason;  ///< why, when skipped
};

/**
 * @brief Identity, layout version and history of an on-disk profile.
 *
 * This travels with the profile and goes inside a package. Anything specific to
 * one machine — where a package file happens to live, a security-scoped
 * bookmark — belongs in the registry instead, never here: a path from one
 * computer is meaningless and mildly disclosive on another.
 */
struct ProfileMarker {
  int schema_version = 0;           ///< layout version that was written
  int min_reader_version = 0;       ///< oldest build allowed to touch it
  QString profile;                  ///< profile name that owns the directory
  QString last_writer_version;      ///< app version that last wrote it
  bool last_writer_stable = false;  ///< whether that build was a release

  /// Minted once at creation. What makes deleting and recreating a profile a
  /// genuinely different identity rather than a colliding one — see the
  /// credential-store account derivation in AppSecureKeyManager.
  QString profile_uuid;

  QString profile_id;    ///< slug, matching the directory name
  QString display_name;  ///< free-form, user-chosen
  QString created;       ///< ISO-8601
  QString created_by_version;
  QString kind;  ///< ProfileRootKindToString()

  /// Identity of the package this profile came from, if any. Location-free on
  /// purpose: the local path lives in the machine's registry.
  QString package_id;

  /// Resolved credential-store account, cached so a later build never has to
  /// re-derive it from a formula that may have changed.
  QString credential_account;

  /// Per-area versions, because data-object content and the settings key layout
  /// evolve independently and a rung touching one should not bump the other.
  QMap<QString, int> components;

  bool self_contained = false;  ///< ProfilePolicy, §1a

  QList<ProfileMigrationRecord> migrations;

  /**
   * @brief Keys this build did not recognise, preserved verbatim.
   *
   * A newer build's extra fields must survive an older build touching the
   * file, or opening a profile with the wrong version quietly destroys
   * information the newer one depends on.
   */
  QJsonObject unknown_fields;
};

/**
 * @brief Decide whether a build may use a profile, from the marker alone.
 *
 * Two version numbers because they answer different questions: @c
 * schema_version records what was written, @c min_reader_version records the
 * oldest build that may safely touch it. A newer layout that stayed
 * backwards-compatible can therefore keep older builds working, while a
 * breaking one locks them out.
 *
 * Pure, and exposed primarily for unit testing.
 *
 * @param marker the parsed marker
 * @param marker_present whether a marker was found at all
 * @param this_schema_version the schema version of the running build
 */
auto GF_CORE_EXPORT CheckProfileCompatibility(const ProfileMarker &marker,
                                              bool marker_present,
                                              int this_schema_version)
    -> ProfileCompatibility;

/**
 * @brief Read the profile marker from disk.
 *
 * Deliberately plain, unencrypted JSON: the application secure key is exactly
 * what fails in the situation this guard exists to catch, so anything that
 * needed the key to be readable would be unreadable when it mattered.
 *
 * @param path marker file path
 * @return the marker, or nothing when absent or unparsable
 */
auto GF_CORE_EXPORT ReadProfileMarker(const QString &path)
    -> std::optional<ProfileMarker>;

/**
 * @brief Write the profile marker to disk.
 *
 * @param path marker file path
 * @param marker marker to persist
 * @return true on success
 */
auto GF_CORE_EXPORT WriteProfileMarker(const QString &path,
                                       const ProfileMarker &marker) -> bool;

/**
 * @brief Where a profile root keeps its marker.
 *
 * Alongside the data it describes, and deliberately outside data_objs/ so the
 * garbage collector never sees it.
 *
 * @param profile_root the root
 * @return absolute path of profile.json
 */
auto GF_CORE_EXPORT ProfileMarkerPathFor(const QString &profile_root)
    -> QString;

/**
 * @brief The marker path of the profile this process is running against.
 *
 * @return absolute path of profile.json
 */
auto GF_CORE_EXPORT CurrentProfileMarkerPath() -> QString;

}  // namespace GpgFrontend
