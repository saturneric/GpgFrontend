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

#include "core/function/ProfileBootstrap.h"

namespace GpgFrontend {

/**
 * @brief One profile, as this machine knows it.
 *
 * Everything here is machine-local. What travels with the profile — its layout
 * version, migration history, policy and uuid — lives in that profile's own
 * `profile.json` instead. Keeping the two apart is what stops a package
 * carrying "I came from /home/eric/work.gfprofile", a path that is meaningless
 * and mildly disclosive on any other computer.
 */
struct GF_CORE_EXPORT ProfileRegistryEntry {
  QString id;
  QString root;  ///< absolute; explicit because classic and portable are not
                 ///< under the profiles root
  QString name;  ///< free-form display name
  QString last_opened;  ///< ISO-8601
  ProfileRootKind kind = ProfileRootKind::kNAMED;

  /// Identity of the package this profile came from. Location-free: the local
  /// path is the field below, and only ever here.
  QString package_id;
  QString source_package;
  QString source_bookmark;  ///< macOS security-scoped bookmark, sandbox only

  /// Synthesised rather than stored: classic and portable exist whether or not
  /// the registry has heard of them, and cannot be deleted or renamed.
  bool implicit = false;
};

/**
 * @brief What this machine knows about its profiles.
 */
struct GF_CORE_EXPORT ProfileRegistryData {
  int schema_version = 1;
  int min_reader_version = 1;

  QString last_used;
  ProfileStartupPolicy startup_policy = ProfileStartupPolicy::kLAST_USED;
  QString startup_profile;

  /// ask | always | never — whether closing a package-linked profile offers to
  /// write it back.
  QString save_on_close = "ask";

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
 * @brief Reconcile stored metadata against what is actually on disk.
 *
 * The filesystem is the source of truth for *existence*; the JSON only carries
 * metadata. A directory that appeared without the registry noticing is adopted,
 * one that vanished is dropped, duplicate ids are collapsed, and a `last_used`
 * pointing at something gone is repointed. The same shape as the key-database
 * sandbox reconciliation, and for the same reason: a list that can disagree
 * with the filesystem eventually will.
 *
 * Pure, so every case is assertable without a directory.
 *
 * @param scanned entries discovered by scanning the profiles root
 * @param stored entries read from profiles.json
 * @return the reconciled registry
 */
auto GF_CORE_EXPORT ReconcileProfileRegistry(
    const QList<ProfileRegistryEntry> &scanned,
    const ProfileRegistryData &stored) -> ProfileRegistryData;

/**
 * @brief Scan a profiles root for profile directories.
 *
 * Directories whose name begins with `.` are skipped: those are staging and
 * extraction scratch, and adopting a half-extracted tree as a real profile
 * would be worse than having no profile at all.
 *
 * @param profiles_root directory to scan
 * @return one entry per directory holding a profile.json
 */
auto GF_CORE_EXPORT ScanProfilesRoot(const QString &profiles_root)
    -> QList<ProfileRegistryEntry>;

/**
 * @brief Read the registry, reconcile it against disk, and add the implicit
 * profiles.
 *
 * Classic is always listed, and portable is listed whenever this installation
 * is portable, whether or not the registry has ever heard of either.
 *
 * @param profiles_root where profiles.json lives
 * @param classic_root the legacy data location
 * @param portable_root the portable data location, or empty when not portable
 * @return the registry as the user should see it
 */
auto GF_CORE_EXPORT LoadProfileRegistry(const QString &profiles_root,
                                        const QString &classic_root,
                                        const QString &portable_root = {})
    -> ProfileRegistryData;

/**
 * @brief Write the registry.
 *
 * Through QSaveFile: a half-written profiles.json loses every profile binding
 * on the machine at once, which is strictly worse than any single profile
 * failing. Implicit entries are not persisted — they are re-synthesised on
 * every load.
 *
 * @param profiles_root where profiles.json lives
 * @param data registry to persist
 * @return true on success
 */
auto GF_CORE_EXPORT SaveProfileRegistry(const QString &profiles_root,
                                        const ProfileRegistryData &data)
    -> bool;

/**
 * @brief Read-modify-write the registry under its lock.
 *
 * Every mutation is a read-modify-write of one shared file, and a deep restart
 * has the old and the new process alive at the same time *by construction* —
 * so without a lock, "profile B created" silently drops "profile A was opened".
 *
 * The lock is held only for the duration of the call, never for the lifetime of
 * the process, so it never blocks a second profile from running. It must never
 * be taken while holding a profile lock: one consistent order, no deadlock.
 *
 * @param profiles_root where profiles.json lives
 * @param classic_root the legacy data location
 * @param portable_root the portable data location, or empty
 * @param mutate returns true when the registry should be written back
 * @return true when the mutation ran and any write succeeded
 */
auto GF_CORE_EXPORT UpdateProfileRegistry(
    const QString &profiles_root, const QString &classic_root,
    const QString &portable_root,
    const std::function<bool(ProfileRegistryData &)> &mutate) -> bool;

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
 * @brief Create an empty profile directory and register it.
 *
 * The application secure key is not generated here: it is created lazily by
 * AppSecureKeyManager on the first start against the new root, which keeps
 * exactly one code path responsible for key material.
 *
 * @param profiles_root where profiles live
 * @param classic_root the legacy data location, for the registry load
 * @param portable_root the portable data location, or empty
 * @param id profile id, already sanitised
 * @param display_name free-form name
 * @param self_contained whether the profile keeps its own keyring
 * @return the outcome
 */
auto GF_CORE_EXPORT CreateProfile(const QString &profiles_root,
                                  const QString &classic_root,
                                  const QString &portable_root,
                                  const QString &id,
                                  const QString &display_name,
                                  bool self_contained) -> ProfileCreateResult;

/**
 * @brief Delete a profile directory and its registry entry.
 *
 * Irreversible: everything the profile's key encrypted becomes permanently
 * unreadable, which is why every caller confirms first. The credential-store
 * entry is the caller's to remove — this owns only the filesystem and the
 * registry.
 *
 * @param profiles_root where profiles live
 * @param classic_root the legacy data location
 * @param portable_root the portable data location, or empty
 * @param id profile to delete
 * @return true when the directory and the entry are both gone
 */
auto GF_CORE_EXPORT DeleteProfile(const QString &profiles_root,
                                  const QString &classic_root,
                                  const QString &portable_root,
                                  const QString &id) -> bool;

/**
 * @brief Record that a profile was opened, and make it the last used.
 *
 * @param profiles_root where profiles live
 * @param classic_root the legacy data location
 * @param portable_root the portable data location, or empty
 * @param id profile that was opened
 * @param now_iso timestamp to record
 */
void GF_CORE_EXPORT TouchProfile(const QString &profiles_root,
                                 const QString &classic_root,
                                 const QString &portable_root,
                                 const QString &id, const QString &now_iso);

}  // namespace GpgFrontend
