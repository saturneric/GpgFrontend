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

#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

auto BuildProfileExportContents(const QMap<QString, qint64>& areas,
                                bool include_workspace)
    -> QList<ProfileExportContentsRow> {
  // A missing key measures zero rather than dropping its row, so renaming an
  // area in the core cannot make a line silently disappear from this list.
  const auto size = [&areas](const QString& key) {
    return areas.value(key, 0);
  };

  QList<ProfileExportContentsRow> rows;

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

auto TotalProfileExportBytes(const QList<ProfileExportContentsRow>& rows)
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

  // A profile file is always sealed, so there is nothing left to decide about
  // protection: an acceptable passphrase is simply part of being ready.
  readiness.can_export = choice.has_destination && choice.passphrase_acceptable;

  // Only judged when the volume could actually be read; a destination
  // QStorageInfo knows nothing about must not produce a scare.
  if (choice.free_bytes >= 0 && choice.total_bytes > choice.free_bytes) {
    readiness.warnings.append(ProfileExportWarning::kMayNotFit);
  }

  return readiness;
}

auto DescribeProfileExportWarning(ProfileExportWarning warning) -> QString {
  switch (warning) {
    case ProfileExportWarning::kMayNotFit:
      return QObject::tr(
          "There may not be room for this where you are saving it. The file is "
          "compressed as it is written, so it may still fit.");
  }
  return {};
}

auto ToMetaListRows(const QList<ProfileExportContentsRow>& rows)
    -> QList<MetaListRow> {
  QList<MetaListRow> out;

  for (const auto& row : rows) {
    // Dimmed when the row carries nothing -- either because it is empty or
    // because it was left out. Rows at equal weight make the reader work out
    // which ones matter; dimming the rest puts the emphasis where the bytes
    // actually are.
    out.append({.caption = row.label,
                .icon = row.icon,
                .bytes = row.bytes,
                .dimmed = !row.included || row.bytes <= 0});
  }

  out.append({.kind = MetaRowKind::kRule});
  out.append({.caption = QObject::tr("Total"),
              .bytes = TotalProfileExportBytes(rows),
              .emphasis = true});

  // What a package carries is an allow-list, so saying what never travels is
  // part of saying what does -- and it belongs in the list rather than in a
  // paragraph under it.
  out.append(
      {.kind = MetaRowKind::kNote,
       .caption = QObject::tr(
           "Never included: logs, modules, and keys kept elsewhere."),
       .detail = QObject::tr(
           "Keys kept outside this profile, such as the system GnuPG keyring, "
           "stay where they are.")});

  return out;
}

auto DescribeExportProtection() -> QString {
  return QObject::tr(
      "Encrypted with XChaCha20-Poly1305, using a key derived from your "
      "passphrase with Argon2id. The system keychain is not used, so the file "
      "opens on any computer.");
}

auto ProfileExportStepTitle(ProfileExportStep step) -> QString {
  switch (step) {
    case ProfileExportStep::kContents:
      return QObject::tr("Contents");
    case ProfileExportStep::kProtection:
      return QObject::tr("Protection");
    case ProfileExportStep::kSave:
      return QObject::tr("Save");
  }
  return {};
}

auto CanAdvanceProfileExport(ProfileExportStep step,
                             const ProfileExportChoice& choice) -> bool {
  switch (step) {
    case ProfileExportStep::kContents:
      return true;
    case ProfileExportStep::kProtection:
      return choice.passphrase_acceptable;
    case ProfileExportStep::kSave:
      return EvaluateProfileExport(choice).can_export;
  }
  return false;
}

auto DescribeProfileExportContents(const QList<ProfileExportContentsRow>& rows)
    -> QString {
  const auto workspace =
      std::any_of(rows.begin(), rows.end(), [](const auto& row) {
        return row.area == ProfileExportArea::kWorkspace && row.included;
      });

  return (workspace ? QObject::tr("Settings, keys, this profile's own key and "
                                  "workspace files, %1 before compression.")
                    : QObject::tr("Settings, keys and this profile's own key, "
                                  "%1 before compression."))
      .arg(HumanSize(TotalProfileExportBytes(rows)));
}

auto HasProfileExportWorkspace(const QList<ProfileExportContentsRow>& rows)
    -> bool {
  return std::any_of(rows.begin(), rows.end(), [](const auto& row) {
    return row.area == ProfileExportArea::kWorkspace && row.bytes > 0;
  });
}

auto DescribeProfileExportWorkspaceOption(
    const QList<ProfileExportContentsRow>& rows) -> QString {
  for (const auto& row : rows) {
    if (row.area != ProfileExportArea::kWorkspace) continue;
    if (row.bytes <= 0) break;
    return QObject::tr("Include workspace files (%1)")
        .arg(HumanSize(row.bytes));
  }
  return QObject::tr("No workspace files to include");
}

auto BuildProfileExportReviewRows(const QString& display_name,
                                  const ProfileExportChoice& choice)
    -> QList<MetaListRow> {
  QList<MetaListRow> rows;

  rows.append({.caption = QObject::tr("Profile"), .value = display_name});

  rows.append({.caption = QObject::tr("Size"),
               .value = choice.include_workspace
                            ? QObject::tr("%1, including workspace files")
                                  .arg(HumanSize(choice.total_bytes))
                            : HumanSize(choice.total_bytes),
               .detail = QObject::tr("Before compression.")});

  // Never a row that reads as protected when it is not. The wizard does not
  // reach this step without an acceptable passphrase, but the rows are built
  // from the choice, not from where the wizard happens to be.
  rows.append({.caption = QObject::tr("Protection"),
               .value = choice.passphrase_acceptable
                            ? QObject::tr("Encrypted with your passphrase")
                            : QObject::tr("No passphrase chosen yet"),
               .danger = !choice.passphrase_acceptable});

  return rows;
}

auto DescribeProfileExportFreeSpace(qint64 free_bytes) -> QString {
  // A volume this application could not read says nothing rather than
  // guessing.
  if (free_bytes < 0) return {};
  return QObject::tr("%1 free on this drive").arg(HumanSize(free_bytes));
}

auto BuildProfileExportDoneRows(const QFileInfo& file, qint64 file_bytes)
    -> QList<MetaListRow> {
  QList<MetaListRow> rows;

  rows.append({.caption = QObject::tr("File"),
               .value = file.fileName(),
               .emphasis = true});
  rows.append({.caption = QObject::tr("Folder"),
               .value = QDir::toNativeSeparators(file.absolutePath()),
               .path = true});
  if (file_bytes >= 0) {
    rows.append(
        {.caption = QObject::tr("Size"), .value = HumanSize(file_bytes)});
  }

  return rows;
}

}  // namespace GpgFrontend::UI
