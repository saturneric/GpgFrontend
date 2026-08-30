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

auto DescribeProfileExport(const ProfileExportChoice& choice,
                           const QString& destination) -> QString {
  // Nothing to say until there is somewhere to say it about. The label keeps
  // its reserved height either way, so an empty sentence leaves a gap rather
  // than a claim.
  if (!choice.has_destination || destination.isEmpty()) return {};

  // One line, because everything else it used to spell out -- what is inside,
  // what it comes to, that it is sealed -- is now a row above it that the
  // reader can check. What is left is the act itself.
  return (choice.include_workspace
              ? QObject::tr("Writes %1: keys, settings and workspace files, "
                            "about %2 before compression.")
              : QObject::tr("Writes %1: keys and settings, about %2 before "
                            "compression."))
      .arg(destination, HumanSize(choice.total_bytes));
}

auto ToMetaListRows(const QVector<ProfileExportContentsRow>& rows)
    -> QVector<MetaListRow> {
  QVector<MetaListRow> out;

  for (const auto& row : rows) {
    // Dimmed when the row carries nothing -- either because it is empty or
    // because it was left out. Rows at equal weight make the reader work out
    // which ones matter; dimming the rest puts the emphasis where the bytes
    // actually are.
    out.append({.caption = row.label,
                .icon = row.icon,
                .bytes = row.bytes,
                .dimmed = !row.included || row.bytes <= 0,
                .checkable = row.optional,
                .checked = row.included});
  }

  out.append({.kind = MetaRowKind::kRule});
  out.append({.caption = QObject::tr("Total"),
              .bytes = TotalProfileExportBytes(rows),
              .emphasis = true});

  // What a package carries is an allow-list, so saying what never travels is
  // part of saying what does -- and it belongs in the list rather than in a
  // paragraph under it.
  out.append(
      {.caption = QObject::tr("Never included"),
       .value = QObject::tr("Logs, modules, keys kept elsewhere"),
       .detail = QObject::tr(
           "Keys kept outside this profile, such as the system GnuPG keyring, "
           "stay where they are."),
       .dimmed = true});

  return out;
}

auto BuildExportProtectionRows() -> QVector<MetaListRow> {
  return {
      {.caption = QObject::tr("Cipher"), .value = "XChaCha20-Poly1305"},
      {.caption = QObject::tr("Key derivation"),
       .value = QObject::tr("Argon2id, from your passphrase")},
      {.caption = QObject::tr("Keychain"),
       .value = QObject::tr("Not used: the file has to open on another "
                            "computer")},
  };
}

}  // namespace GpgFrontend::UI
