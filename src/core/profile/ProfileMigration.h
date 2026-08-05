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

#include <functional>

#include "core/profile/ProfileMarker.h"

namespace GpgFrontend {

/// The oldest layout this build still carries rungs for. A profile below this
/// cannot be upgraded automatically and must be refused rather than guessed at.
constexpr int kOldestSupportedProfileSchema = 1;

/**
 * @brief When a rung may run, relative to the application secure key.
 *
 * The split is not cosmetic. Everything under `data_objs/` is
 * XChaCha20-Poly1305 sealed and simply does not decrypt until
 * ProfileSecureKeyManager::Initialize() has run, so a data-object rung placed
 * in the early stage fails *silently* — it sees no objects and concludes there
 * is nothing to do.
 */
enum class ProfileMigrationStage {
  kPRE_KEY,   ///< directory moves, renames, settings-key rewrites
  kPOST_KEY,  ///< anything that reads or writes a data object
};

/**
 * @brief What one rung did.
 */
struct ProfileMigrationOutcome {
  bool ok = true;
  QString detail;  ///< why it failed, when it did
};

/**
 * @brief One consecutive step of the layout ladder.
 *
 * Rungs are consecutive and one-way (2→3, 3→4, …) and composed into a ladder,
 * never a switch on "current versus target": a switch has to be re-reasoned
 * every time a version is added, while a ladder only ever gains one rung.
 */
struct ProfileMigration {
  int from = 0;
  int to = 0;

  /// Stable identifier, recorded in profile.json. Never reuse or rename one:
  /// the record is what tells a later build which rungs really ran.
  QString name;

  ProfileMigrationStage stage = ProfileMigrationStage::kPRE_KEY;

  /**
   * @brief Whether this rung can run against classic's split storage.
   *
   * A rung that touches settings should go through GetSettings() and stay
   * true, which makes it correct for both storage shapes. Only a rung needing
   * genuine file-level access — moving a directory, say — sets this false, and
   * it must document what happens to classic instead. The skip is recorded, so
   * a later audit does not mistake it for having run.
   */
  bool runs_on_classic = true;

  /// Raise the minimum reader only when the rung genuinely makes the profile
  /// unreadable to older builds. Raising it gratuitously strands users.
  int raises_min_reader_to = 0;

  /**
   * @brief Do the work.
   *
   * Must be idempotent, so a crash between the work and its commit leaves a
   * re-run harmless. Must write every file it touches through QSaveFile, so
   * "the rung succeeded" and "the bytes are on disk" cannot come apart.
   */
  std::function<ProfileMigrationOutcome(const QString &profile_root)> apply;
};

/**
 * @brief What should happen to a profile before anything touches it.
 */
enum class ProfileMigrationVerdict {
  kNONE,     ///< already current, or brand new; nothing to do
  kUPGRADE,  ///< run the ladder
  kTOO_NEW,  ///< written by a newer build; must not be touched
  kREFUSE,   ///< cannot be upgraded by this build
};

/**
 * @brief The decision, and why.
 */
struct ProfileMigrationPlan {
  ProfileMigrationVerdict verdict = ProfileMigrationVerdict::kNONE;
  int from = 0;
  int to = 0;

  /// Shown to the user for kTOO_NEW and kREFUSE, and logged otherwise.
  QString reason;

  /// The version that wrote the profile, so a refusal can name the build to go
  /// back to rather than leaving the user guessing.
  QString writer_version;
};

/**
 * @brief Decide, without touching anything, whether a profile may be upgraded.
 *
 * Pure and total. It runs before the first rung of each stage, and **nothing on
 * disk is modified unless it returns kUPGRADE** — not the marker, not a
 * timestamp, not a directory. A profile written by a newer build has to survive
 * being opened by an older one exactly as it was, or the act of looking at it
 * is what corrupts the newer installation.
 *
 * @param marker parsed marker
 * @param marker_present whether a marker was found at all
 * @param target this build's layout version
 * @param known_rung_names every rung name this build carries
 * @return the verdict, with a reason when it is not kUPGRADE
 */
auto GF_CORE_EXPORT PlanProfileMigration(const ProfileMarker &marker,
                                         bool marker_present, int target,
                                         const QStringList &known_rung_names)
    -> ProfileMigrationPlan;

/**
 * @brief The rungs that apply to one stage of one upgrade.
 *
 * @param from starting layout version
 * @param to target layout version
 * @param stage which stage is running
 * @param ladder every rung this build carries
 * @return the applicable rungs, in ascending order
 */
auto GF_CORE_EXPORT ProfileMigrationsFor(int from, int to,
                                         ProfileMigrationStage stage,
                                         const QList<ProfileMigration> &ladder)
    -> QList<ProfileMigration>;

/**
 * @brief How far a ladder run got.
 */
struct ProfileMigrationResult {
  bool ok = true;

  /// The layout version actually committed to disk.
  int reached = 0;

  QString failed_rung;
  QString detail;
};

/**
 * @brief Run one stage of the ladder, committing each rung as it completes.
 *
 * Every rung is committed to `profile.json` — appended to `migrations[]` and
 * the version bumped — **before the next one starts**, and never deferred to
 * StampProfileMarker(). That runs once, much later, after the application key
 * loads; a crash or a failing rung in between would lose every completed rung
 * and re-run them against already-migrated data on the next start.
 *
 * A failing rung stops the ladder and leaves the profile at the last
 * successfully committed version, with `migrations[]` listing exactly the rungs
 * that really ran.
 *
 * @param profile_root root to migrate
 * @param marker_path path of profile.json within that root
 * @param plan the decision from PlanProfileMigration()
 * @param stage which stage to run
 * @param is_classic whether this is classic's split storage
 * @param now_iso timestamp to record, passed in so the run stays reproducible
 * @param ladder every rung this build carries
 * @return how far it got
 */
auto GF_CORE_EXPORT RunProfileMigration(const QString &profile_root,
                                        const QString &marker_path,
                                        const ProfileMigrationPlan &plan,
                                        ProfileMigrationStage stage,
                                        bool is_classic, const QString &now_iso,
                                        const QList<ProfileMigration> &ladder)
    -> ProfileMigrationResult;

/**
 * @brief Every rung this build carries.
 *
 * Deliberately empty for now. The empty ladder is not a placeholder to be
 * skipped in testing — it is the path that runs on every start for the rest of
 * the product's life, and it has to be exercised as carefully as any rung.
 *
 * @return the ladder, ascending
 */
auto GF_CORE_EXPORT AllProfileMigrations() -> QList<ProfileMigration>;

/**
 * @brief The names of every rung in @ref AllProfileMigrations.
 *
 * @return rung names
 */
auto GF_CORE_EXPORT AllProfileMigrationNames() -> QStringList;

}  // namespace GpgFrontend
