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

#include "core/model/KeyDatabaseInfo.h"

namespace GpgFrontend {

struct KeyDatabaseItemSO {
  QString name;
  QString path;
  int channel = 0;  ///< a JSON object without "channel" must not leave this
                    ///< indeterminate -- it feeds channel normalization
  QString backend_type;

  /// What this database is, and so whether a package may carry it. Empty for an
  /// entry written before the field existed; ResolveKeyDatabaseKinds() fills
  /// those in from the path, which is what every earlier build inferred.
  std::optional<KeyDatabaseKind> kind;

  KeyDatabaseItemSO() = default;

  explicit KeyDatabaseItemSO(KeyDatabaseInfo i) {
    name = i.name;
    path = i.origin_path.isEmpty() ? i.path : i.origin_path;
    channel = i.channel;
    backend_type = i.backend_type;
    kind = i.kind;
  }

  explicit KeyDatabaseItemSO(const QJsonObject& j) {
    if (const auto v = j["name"]; v.isString()) {
      name = v.toString();
    }
    if (const auto v = j["path"]; v.isString()) {
      path = v.toString();
    }
    if (const auto v = j["channel"]; v.isDouble()) {
      channel = v.toInt();
    }
    if (const auto v = j["backend_type"]; v.isString()) {
      backend_type = v.toString();
    }
    if (const auto v = j["kind"]; v.isString()) {
      kind = ConvertString2KeyDatabaseKind(v.toString());
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    j["name"] = name;
    j["path"] = path;
    j["channel"] = channel;
    j["backend_type"] = backend_type;

    // Omitted rather than written as a placeholder when it is not yet settled:
    // a "kind" key that is present but meaningless is one an older reader and a
    // newer one would disagree about.
    if (kind) j["kind"] = ConvertKeyDatabaseKind2String(*kind);
    return j;
  }
};

}  // namespace GpgFrontend