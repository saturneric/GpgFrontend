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

#include "ui/dialog/profile/ProfilePackageMeta.h"

#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

/// A moment as this machine words it. The header stores an ISO 8601 instant in
/// UTC, which is the right thing to write into a file and the wrong thing to
/// put in front of a person -- but a claim this application cannot parse is
/// still a claim worth showing, so an unreadable one is shown verbatim.
auto HumanInstant(const QString &iso) -> QString {
  const auto stamp = QDateTime::fromString(iso, Qt::ISODate).toLocalTime();
  return stamp.isValid() ? QLocale().toString(stamp, QLocale::ShortFormat)
                         : iso;
}

}  // namespace

auto BuildProfilePackageRows(const QFileInfo &file,
                             const ProfilePackageHeader &header,
                             bool header_readable) -> QVector<MetaListRow> {
  QVector<MetaListRow> rows;

  rows.append({.caption = QObject::tr("File"),
               .value = file.fileName(),
               .emphasis = true});
  rows.append({.caption = QObject::tr("Folder"),
               .value = QDir::toNativeSeparators(file.absolutePath()),
               .path = true});

  // A file that is not there yet, or was moved out from under this dialog, has
  // no size and no date. Saying nothing about them beats printing a zero.
  if (file.exists()) {
    rows.append(
        {.caption = QObject::tr("Size"), .value = HumanSize(file.size())});
    rows.append({.caption = QObject::tr("Modified"),
                 .value = QLocale().toString(file.lastModified(),
                                             QLocale::ShortFormat)});
  }

  if (!header_readable) return rows;

  QVector<MetaListRow> claims;

  // The one claim worth interrupting someone over. Every file this application
  // writes is sealed, so a file saying it is not either came from an older
  // build or is not what it appears to be -- and either way what is inside it
  // can be read by anyone holding it.
  if (header.protection == ProfilePackageProtection::kNONE) {
    claims.append({.caption = QObject::tr("Protection"),
                   .value = QObject::tr("Not sealed"),
                   .detail = QObject::tr(
                       "Anyone holding this file can read what is in it."),
                   .danger = true,
                   .unverified = true});
  }

  if (!header.created.isEmpty()) {
    claims.append({.caption = QObject::tr("Created"),
                   .value = HumanInstant(header.created),
                   .unverified = true});
  }

  if (!header.writer.isEmpty()) {
    claims.append(
        {.caption = QObject::tr("Written by"),
         .value = header.writer_stable
                      ? QObject::tr("GpgFrontend %1").arg(header.writer)
                      : QObject::tr("GpgFrontend %1 (development build)")
                            .arg(header.writer),
         .unverified = true});
  }

  // A header that claims nothing has nothing to quote, and a heading over an
  // empty group would only be noise above the passphrase field.
  if (claims.isEmpty()) return rows;

  rows.append({.kind = MetaRowKind::kSection,
               .caption = QObject::tr("From the file's header (unverified)")});
  rows.append(claims);

  return rows;
}

auto BuildProfilePackageDestinationRows(const QFileInfo &file,
                                        qint64 free_bytes)
    -> QVector<MetaListRow> {
  QVector<MetaListRow> rows;

  rows.append({.caption = QObject::tr("File"),
               .value = file.fileName(),
               .emphasis = true});
  rows.append({.caption = QObject::tr("Folder"),
               .value = QDir::toNativeSeparators(file.absolutePath()),
               .path = true});

  // Free space as a fact, not a prediction. What the file will actually occupy
  // is unknowable before it is packed, because the payload is compressed on the
  // way out; what the volume has right now is simply true. A volume this
  // application could not read says nothing rather than scaring anyone.
  if (free_bytes >= 0) {
    rows.append(
        {.caption = QObject::tr("Free space"), .value = HumanSize(free_bytes)});
  }

  return rows;
}

}  // namespace GpgFrontend::UI
