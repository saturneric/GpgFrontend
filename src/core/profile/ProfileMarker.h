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

#include <optional>

namespace GpgFrontend {

/**
 * @brief Whether this build may safely use the profile it found.
 */
enum class ProfileCompatibility : std::uint8_t {
  kOK,       ///< safe to read and write
  kMISSING,  ///< first run, or a profile written before markers existed
  kTOO_NEW,  ///< written by a newer build; must not be touched
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
  /// credential-store account derivation in ProfileSecureKeyManager.
  QString profile_uuid;

  QString profile_id;    ///< slug, matching the directory name
  QString display_name;  ///< free-form, user-chosen
  QString created;       ///< ISO-8601
  QString created_by_version;
  QString kind;  ///< ProfileKindToString()

  /**
   * @brief When this profile was last opened, ISO-8601.
   *
   * Written by ProfileLoader once a profile really has opened, so every process
   * records itself — including one started from a shell with `--profile`. It
   * used to be kept in the machine registry and stamped by whichever *other*
   * window did the launching, which meant a direct launch was never recorded.
   */
  QString last_opened;

  /// Identity of the package this profile came from, if any. Location-free on
  /// purpose: the local path lives in the machine's registry.
  QString package_id;

  /// Resolved credential-store account, cached so a later build never has to
  /// re-derive it from a formula that may have changed.
  QString credential_account;

  bool self_contained = false;  ///< ProfilePolicy, §1a

  /**
   * @brief Knob values this profile pins, overriding the user's own settings.
   *
   * The deployment overrides that used to live in an ENV.ini beside — in
   * practice, in whatever directory the process happened to start in. Keeping
   * them here makes them a property of the profile they govern, which is the
   * only scope at which they were ever meaningful: settings are per-profile, so
   * an override that was not was overriding several stores at once.
   *
   * Recognised keys mirror the Advanced tab: `SelfCheck`, `SecureLevel`,
   * `AppKeyProtection`, `OSSecretStore`, `LogLevel`, `LogRingBufferCapacity`,
   * `GnuPGOfflineMode`, `PinentryProgramPath`.
   *
   * A **missing** key must stay distinguishable from one set to a falsy value:
   * ResolveLayeredValue() and the whole ResolveAppKeyProtection() ladder treat
   * an invalid QVariant as "this layer has no opinion", and an explicit `false`
   * as an answer that stops the ladder.
   */
  QVariantMap deployment;

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

}  // namespace GpgFrontend
