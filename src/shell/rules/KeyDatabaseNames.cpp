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

#include "shell/rules/KeyDatabaseNames.h"

#include "core/model/KeyDatabaseInfo.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto IsKeyDatabaseNameText(const QString& name) -> bool {
  static const QRegularExpression kAllowed(
      QStringLiteral("^%1$").arg(QLatin1StringView(kKeyDatabaseNamePattern)));
  return kAllowed.match(name).hasMatch();
}

auto IsKeyDatabaseNameTaken(const QString& name,
                            const QList<KeyDatabaseInfo>& infos) -> bool {
  const auto trimmed = name.trimmed();
  return std::any_of(
      infos.cbegin(), infos.cend(), [&trimmed](const KeyDatabaseInfo& info) {
        return info.name.compare(trimmed, Qt::CaseInsensitive) == 0 ||
               KeyDatabaseDisplayName(info.id, info.name)
                       .compare(trimmed, Qt::CaseInsensitive) == 0;
      });
}

auto SuggestKeyDatabaseName(const QString& prefix,
                            const QList<KeyDatabaseInfo>& infos,
                            const QString& app_data_path) -> QString {
  // The first unused number, rather than one more than the count: after the
  // first of two is removed, the count names the one that is left.
  for (int n = 1;; ++n) {
    const auto candidate = u"%1 %2"_s.arg(prefix).arg(n);
    if (IsKeyDatabaseNameTaken(candidate, infos)) continue;
    if (!app_data_path.isEmpty() &&
        QFileInfo::exists(ManagedKeyDatabasePath(app_data_path, candidate))) {
      continue;
    }
    return candidate;
  }
}

}  // namespace GpgFrontend
