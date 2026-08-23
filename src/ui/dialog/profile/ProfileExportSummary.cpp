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

#include "ui/dialog/profile/ProfileExportSummary.h"

namespace GpgFrontend::UI {

namespace {

auto HumanSize(qint64 bytes) -> QString {
  return QLocale().formattedDataSize(bytes, 1,
                                     QLocale::DataSizeTraditionalFormat);
}

}  // namespace

auto BuildProfileExportContents(const QMap<QString, qint64>& areas,
                                bool include_workspace)
    -> QVector<ProfileExportContentsRow> {
  // A missing key measures zero rather than dropping its row, so renaming an
  // area in the core cannot make a line silently disappear from this list.
  const auto size = [&areas](const QString& key) {
    return areas.value(key, 0);
  };

  QVector<ProfileExportContentsRow> rows;

  rows.append({ProfileExportArea::kSettings, QObject::tr("Settings"),
               ":/icons/setting.png", size("config"), false, true});

  rows.append({ProfileExportArea::kSavedState,
               QObject::tr("Saved state, key groups and categories"),
               ":/icons/database.png", size("data_objs"), false, true});

  // Listed because it is the whole reason protection matters. Everything the
  // profile ever encrypted is encrypted with this key, and it is inside the
  // file — so a list that named the settings and the keyrings but not this one
  // was describing the least dangerous part of what it was about to write.
  rows.append({ProfileExportArea::kProfileKey,
               QObject::tr("This profile's own key"), ":/icons/lock.png",
               size("secure"), false, true});

  rows.append({ProfileExportArea::kKeyDatabases,
               QObject::tr("Keys stored inside this profile"),
               ":/icons/key.png", size("key_databases"), false, true});

  rows.append({ProfileExportArea::kWorkspace, QObject::tr("My workspace files"),
               ":/icons/workspace.png", size("workspace"), true,
               include_workspace});

  return rows;
}

auto TotalProfileExportBytes(const QVector<ProfileExportContentsRow>& rows)
    -> qint64 {
  qint64 total = 0;
  for (const auto& row : rows) {
    if (row.included) total += row.bytes;
  }
  return total;
}

auto EvaluateProfileExport(const ProfileExportChoice& choice)
    -> ProfileExportReadiness {
  ProfileExportReadiness readiness;

  readiness.can_export =
      choice.has_destination &&
      (!choice.protect_with_passphrase || choice.passphrase_acceptable);

  // Ordered by what it would cost to ignore. Handing somebody the profile's
  // key outranks running out of room, which is merely inconvenient and may not
  // even happen.
  if (!choice.protect_with_passphrase) {
    readiness.warnings.append(
        choice.include_workspace
            ? ProfileExportWarning::kUnprotectedWithWorkspace
            : ProfileExportWarning::kUnprotected);
  }

  // Only judged when the volume could actually be read; a destination
  // QStorageInfo knows nothing about must not produce a scare.
  if (choice.free_bytes >= 0 && choice.total_bytes > choice.free_bytes) {
    readiness.warnings.append(ProfileExportWarning::kMayNotFit);
  }

  return readiness;
}

auto DescribeProfileExportWarning(ProfileExportWarning warning) -> QString {
  switch (warning) {
    case ProfileExportWarning::kUnprotected:
      return QObject::tr(
          "Anyone who gets this file can read your keys and everything in the "
          "profile, and can change it before you import it.");
    case ProfileExportWarning::kUnprotectedWithWorkspace:
      return QObject::tr(
          "Anyone who gets this file can read your keys, everything in the "
          "profile, and your workspace files — including anything you meant to "
          "encrypt but had not yet.");
    case ProfileExportWarning::kMayNotFit:
      return QObject::tr(
          "There may not be room for this where you are saving it. The file is "
          "compressed as it is written, so it may still fit.");
  }
  return {};
}

auto DescribeProfileExport(const ProfileExportChoice& choice,
                           const QString& destination) -> QString {
  // Nothing to say until there is somewhere to say it about. The label keeps
  // its reserved height either way, so an empty sentence leaves a gap rather
  // than a claim.
  if (!choice.has_destination || destination.isEmpty()) return {};

  return choice.protect_with_passphrase
             ? (choice.include_workspace
                    ? QObject::tr(
                          "A passphrase-protected file holding your keys, "
                          "settings and workspace files — about %1 before "
                          "compression — will be written to %2.")
                    : QObject::tr(
                          "A passphrase-protected file holding your keys and "
                          "settings — about %1 before compression — will be "
                          "written to %2."))
                   .arg(HumanSize(choice.total_bytes), destination)
             : (choice.include_workspace
                    ? QObject::tr(
                          "An unprotected file holding your keys, settings and "
                          "workspace files — about %1 before compression — "
                          "will be written to %2.")
                    : QObject::tr(
                          "An unprotected file holding your keys and settings "
                          "— about %1 before compression — will be written to "
                          "%2."))
                   .arg(HumanSize(choice.total_bytes), destination);
}

}  // namespace GpgFrontend::UI
