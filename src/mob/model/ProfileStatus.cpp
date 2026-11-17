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

#include "model/ProfileStatus.h"

#include "core/profile/ProfileSession.h"
#include "shell/present/ProfileStatus.h"

namespace GpgFrontend::Mobile {

namespace {

auto Current() -> QList<StatusSection> {
  // Before a profile is open (the splash, an error page) there is nothing to
  // report, and the session would abort if asked.
  if (!ProfileSession::Loaded()) return {};
  return ProfileStatusSections(BuildProfileStatus(GatherProfileStatusFacts()));
}

}  // namespace

ProfileStatus::ProfileStatus(QObject* parent) : QObject(parent) {}

auto ProfileStatus::sections() const -> QVariantList {
  QVariantList sections;
  for (const auto& section : Current()) {
    QVariantList rows;
    for (const auto& row : section.rows) {
      // The desktop's captions end in a colon, a form's; a phone's rows
      // do not. Translated, it may be a full-width one or follow a space.
      auto label = row.caption.trimmed();
      while (label.endsWith(u':') || label.endsWith(QChar(0xFF1A))) {
        label.chop(1);
        label = label.trimmed();
      }
      rows.append(QVariantMap{{QStringLiteral("label"), label},
                              {QStringLiteral("value"), row.value},
                              {QStringLiteral("detail"), row.detail},
                              {QStringLiteral("degraded"), row.degraded},
                              {QStringLiteral("dimmed"), row.dimmed},
                              {QStringLiteral("mono"), row.path}});
    }
    sections.append(QVariantMap{{QStringLiteral("title"), section.title},
                                {QStringLiteral("rows"), rows}});
  }
  return sections;
}

auto ProfileStatus::text() const -> QString {
  return ProfileStatusText(Current());
}

}  // namespace GpgFrontend::Mobile
