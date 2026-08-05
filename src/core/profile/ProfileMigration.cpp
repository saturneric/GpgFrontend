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

#include "ProfileMigration.h"

#include "core/function/GlobalSettingStation.h"
#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend {

namespace {

/**
 * @brief `basic/home_path_as_file_panel_default_path` becomes a three-valued
 * `basic/file_panel_default_path_mode`.
 *
 * Written against GetSettings() rather than against config.ini, which is what
 * lets the same rung be correct for classic's native store as well — hence
 * runs_on_classic staying true.
 *
 * Idempotent: once the new key exists the rung has nothing left to do, so a
 * crash between the work and its commit costs nothing on the re-run.
 */
auto MigrateFilePanelDefaultPathMode(const QString&)
    -> ProfileMigrationOutcome {
  auto settings = GetSettings();

  constexpr auto kNewKey = "basic/file_panel_default_path_mode";
  constexpr auto kOldKey = "basic/home_path_as_file_panel_default_path";

  if (settings.contains(kNewKey)) return {};

  // The old default was true, and an absent key has to keep meaning exactly
  // that: an existing installation must open where it always did.
  //
  // The spellings are literals rather than a call through
  // FilePanelDefaultPathModeToString(): that enum lives in the UI layer, which
  // core cannot call into, and a rung must in any case be frozen against the
  // values as they were when it was written rather than following an enum that
  // may later be renamed underneath it.
  const auto legacy = settings.value(kOldKey, true).toBool();
  settings.setValue(kNewKey,
                    legacy ? QStringLiteral("home") : QStringLiteral("cwd"));
  settings.sync();

  if (settings.status() != QSettings::NoError) {
    return {false, "the settings store could not be written"};
  }
  return {};
}

}  // namespace

auto AllProfileMigrations() -> QList<ProfileMigration> {
  QList<ProfileMigration> ladder;

  ProfileMigration file_panel_mode;
  file_panel_mode.from = 2;
  file_panel_mode.to = 3;
  file_panel_mode.name = "file-panel-default-path-mode";
  file_panel_mode.stage = ProfileMigrationStage::kPRE_KEY;
  file_panel_mode.runs_on_classic = true;
  file_panel_mode.apply = MigrateFilePanelDefaultPathMode;
  ladder.append(file_panel_mode);

  return ladder;
}

auto AllProfileMigrationNames() -> QStringList {
  QStringList names;
  for (const auto& rung : AllProfileMigrations()) names.append(rung.name);
  return names;
}

auto PlanProfileMigration(const ProfileMarker& marker, bool marker_present,
                          int target, const QStringList& known_rung_names)
    -> ProfileMigrationPlan {
  ProfileMigrationPlan plan;
  plan.to = target;
  plan.writer_version = marker.last_writer_version;

  // A profile with no marker is a first run, or one written before markers
  // existed. There is no history to upgrade and nothing to refuse.
  if (!marker_present) {
    plan.from = target;
    plan.verdict = ProfileMigrationVerdict::kNONE;
    return plan;
  }

  plan.from = marker.schema_version;

  if (marker.schema_version > target) {
    plan.verdict = ProfileMigrationVerdict::kTOO_NEW;
    plan.reason =
        QString(
            "This profile was written for layout version %1, but this "
            "build understands only %2.")
            .arg(marker.schema_version)
            .arg(target);
    return plan;
  }

  if (marker.min_reader_version > target) {
    plan.verdict = ProfileMigrationVerdict::kTOO_NEW;
    plan.reason = QString(
                      "This profile requires layout version %1 or newer to be "
                      "opened safely; this build provides %2.")
                      .arg(marker.min_reader_version)
                      .arg(target);
    return plan;
  }

  // A rung this build has never heard of, that claims to have taken the profile
  // past our version, means it went through a fork or a newer branch. Running
  // our own ladder over it would apply rungs on top of changes we cannot see.
  for (const auto& record : marker.migrations) {
    if (record.skipped) continue;
    if (record.to > target && !known_rung_names.contains(record.name)) {
      plan.verdict = ProfileMigrationVerdict::kREFUSE;
      plan.reason =
          QString(
              "This profile has been through a migration this build does "
              "not know ('%1').")
              .arg(record.name);
      return plan;
    }
  }

  if (marker.schema_version < kOldestSupportedProfileSchema) {
    plan.verdict = ProfileMigrationVerdict::kREFUSE;
    plan.reason =
        QString(
            "This profile uses layout version %1, which is too old for "
            "this build to upgrade automatically.")
            .arg(marker.schema_version);
    return plan;
  }

  plan.verdict = marker.schema_version == target
                     ? ProfileMigrationVerdict::kNONE
                     : ProfileMigrationVerdict::kUPGRADE;
  return plan;
}

auto ProfileMigrationsFor(int from, int to, ProfileMigrationStage stage,
                          const QList<ProfileMigration>& ladder)
    -> QList<ProfileMigration> {
  QList<ProfileMigration> out;
  if (from >= to) return out;

  for (auto v = from; v < to; ++v) {
    for (const auto& rung : ladder) {
      if (rung.from == v && rung.to == v + 1 && rung.stage == stage) {
        out.append(rung);
      }
    }
  }
  return out;
}

auto RunProfileMigration(const QString& profile_root,
                         const QString& marker_path,
                         const ProfileMigrationPlan& plan,
                         ProfileMigrationStage stage, bool is_classic,
                         const QString& now_iso,
                         const QList<ProfileMigration>& ladder)
    -> ProfileMigrationResult {
  ProfileMigrationResult result;
  result.reached = plan.from;

  if (plan.verdict != ProfileMigrationVerdict::kUPGRADE) return result;

  const auto rungs = ProfileMigrationsFor(plan.from, plan.to, stage, ladder);
  if (rungs.isEmpty()) {
    // No rung for this stage does not mean no rung at all: the other stage may
    // still have work, and it owns the version bump for its own rungs.
    return result;
  }

  auto marker = ReadProfileMarker(marker_path).value_or(ProfileMarker{});

  // One copy of the marker before the first rung, so a user who has to unpick a
  // bad upgrade by hand has something to unpick it with.
  const auto backup_path = QString("%1.pre-%2").arg(marker_path).arg(plan.from);
  if (!QFileInfo::exists(backup_path)) {
    QFile::copy(marker_path, backup_path);
  }

  for (const auto& rung : rungs) {
    ProfileMigrationRecord record;
    record.from = rung.from;
    record.to = rung.to;
    record.name = rung.name;
    record.at = now_iso;
    record.by = GetProjectVersion();

    if (is_classic && !rung.runs_on_classic) {
      // Recorded rather than silently passed over: a later audit must be able
      // to tell "this did not apply here" from "this ran".
      record.skipped = true;
      record.reason = "classic";
      LOG_I() << "profile migration rung skipped on classic:" << rung.name;
    } else {
      const auto outcome = rung.apply(profile_root);
      if (!outcome.ok) {
        result.ok = false;
        result.failed_rung = rung.name;
        result.detail = outcome.detail;
        LOG_E() << "profile migration rung failed:" << rung.name
                << outcome.detail;
        // Stop here, at the last version actually committed. Never bump past a
        // rung that did not do its work.
        return result;
      }
      LOG_I() << "profile migration rung applied:" << rung.name;
    }

    marker.migrations.append(record);
    marker.schema_version = rung.to;
    if (rung.raises_min_reader_to > marker.min_reader_version) {
      marker.min_reader_version = rung.raises_min_reader_to;
    }

    // Committed before the next rung starts, through QSaveFile. Deferring this
    // to StampProfileMarker() would lose every completed rung if anything
    // between here and there failed.
    if (!WriteProfileMarker(marker_path, marker)) {
      result.ok = false;
      result.failed_rung = rung.name;
      result.detail = "the profile marker could not be committed";
      LOG_E() << "profile migration could not commit rung:" << rung.name;
      return result;
    }

    result.reached = rung.to;
  }

  return result;
}

}  // namespace GpgFrontend
