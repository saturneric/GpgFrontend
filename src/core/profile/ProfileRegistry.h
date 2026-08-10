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

#include "core/profile/Profile.h"

namespace GpgFrontend {

/**
 * @brief One profile, as found on disk.
 *
 * Every field is either the directory itself or something read out of that
 * directory's `profile.json`. There is deliberately no second copy of any of
 * it: a machine-level index that can disagree with the filesystem eventually
 * will, and the disagreement is only ever discovered by a user who has lost
 * track of a profile.
 */
struct GF_CORE_EXPORT ProfileRegistryEntry {
  QString id;
  QString root;  ///< absolute; explicit because the two roots are not under
                 ///< the profiles root
  QString name;  ///< display name from the marker, else the folder name
  QString last_opened;  ///< ISO-8601, stamped by ProfileLoader
  ProfileKind kind = ProfileKind::kPERSIST;

  /// Identity of the package this profile came from, if any. Location-free on
  /// purpose: a path from the machine that made the package is meaningless on
  /// this one.
  QString package_id;

  /// Synthesised rather than found: the two roots exist whether or not the
  /// profiles directory does, and cannot be deleted or renamed.
  bool implicit = false;
};

/**
 * @brief The profiles this machine has, as of the last scan.
 */
struct GF_CORE_EXPORT ProfileRegistryData {
  QList<ProfileRegistryEntry> profiles;

  /**
   * @brief Find an entry by id.
   *
   * @param id profile id
   * @return the entry, or nothing
   */
  [[nodiscard]] auto Find(const QString &id) const
      -> std::optional<ProfileRegistryEntry>;
};

/**
 * @brief Scan a profiles root for profile directories.
 *
 * The filesystem is the whole truth here, which is what lets this be a plain
 * read with nothing to keep in sync and no lock to take. Directories whose name
 * begins with `.` are skipped: those are staging and extraction scratch, and
 * adopting a half-extracted tree as a real profile would be worse than having
 * no profile at all.
 *
 * @param profiles_root directory to scan
 * @return one entry per directory holding a profile.json
 */
auto GF_CORE_EXPORT ScanProfilesRoot(const QString &profiles_root)
    -> QList<ProfileRegistryEntry>;

/**
 * @brief The profile list as the user should see it.
 *
 * The scan, plus the roots that are always there: the installed one always, and
 * the portable one on a portable build.
 *
 * @param profiles_root where persisted profiles live
 * @param classic_root the installed data location
 * @param portable_root the portable data location, or empty when not portable
 * @return the list, implicit entries first
 */
auto GF_CORE_EXPORT LoadProfileRegistry(const QString &profiles_root,
                                        const QString &classic_root,
                                        const QString &portable_root = {})
    -> ProfileRegistryData;

/**
 * @brief Why creating a profile failed.
 */
enum class ProfileCreateStatus {
  kOK,
  kINVALID_ID,
  kALREADY_EXISTS,
  kIO_FAILED,
};

/**
 * @brief Result of CreateProfile().
 */
struct GF_CORE_EXPORT ProfileCreateResult {
  ProfileCreateStatus status = ProfileCreateStatus::kOK;
  QString detail;
  ProfileRegistryEntry entry;

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfileCreateStatus::kOK;
  }
};

/**
 * @brief Mint a short, unused directory name for a new profile.
 *
 * Short on purpose. The profile directory name sits inside the GnuPG home
 * directory path, and gpg-agent has to fit that path plus a socket name into
 * sockaddr_un::sun_path -- 104 bytes on macOS. A full 32-hex uuid pushed the
 * default profile past that limit, so gpg-agent could not create its socket and
 * GnuPG was unreachable for the whole session. Six hex digits give 16.7M values,
 * which is ample for the handful of profiles one person keeps, and collisions
 * are resolved here rather than surfaced.
 *
 * This is only the directory name. ProfileMarker::profile_uuid stays a full
 * uuid: it identifies the credential-store account and never appears in a path,
 * so it has no reason to trade collision resistance for length.
 *
 * @param profiles_root where profiles live
 * @return an id no existing profile directory uses, or empty if none was found
 */
auto GF_CORE_EXPORT MintProfileDirectoryId(const QString &profiles_root)
    -> QString;

/**
 * @brief Create a profile directory and write its marker.
 *
 * Writing the marker is what makes the directory a profile — there is nothing
 * else to register it with. The application secure key is not generated here:
 * it is created lazily by ProfileSecureKeyManager on the first start against
 * the new root, which keeps exactly one code path responsible for key material.
 *
 * @param profiles_root where profiles live
 * @param id profile id, already sanitised
 * @param display_name free-form name
 * @param self_contained whether the profile keeps its own keyring
 * @return the outcome
 */
auto GF_CORE_EXPORT CreateProfile(const QString &profiles_root,
                                  const QString &id,
                                  const QString &display_name,
                                  bool self_contained) -> ProfileCreateResult;

/**
 * @brief Delete a profile directory.
 *
 * Irreversible: everything the profile's key encrypted becomes permanently
 * unreadable, which is why every caller confirms first. The credential-store
 * entry is the caller's to remove — this owns only the filesystem.
 *
 * @param profiles_root where profiles live
 * @param id profile to delete
 * @return true when the directory is gone
 */
auto GF_CORE_EXPORT DeleteProfile(const QString &profiles_root,
                                  const QString &id) -> bool;

}  // namespace GpgFrontend
