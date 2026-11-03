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

#include "core/function/SettingItem.h"

#include "core/function/SettingCatalog.h"

namespace GpgFrontend {

namespace {

/// Where the build puts the flavor's settings policy (src/core/CMakeLists.txt).
constexpr auto kFlavorSettingsPolicy = ":/gf/settings_policy.json";

auto IsCatalogKey(const QString &key) -> bool {
  return std::any_of(SettingItems::kAll.cbegin(), SettingItems::kAll.cend(),
                     [&key](const SettingMeta *meta) {
                       return QLatin1StringView(meta->key) == key;
                     });
}

}  // namespace

auto ParseSettingsPolicy(const QByteArray &json) -> QHash<QString, QVariant> {
  QHash<QString, QVariant> defaults;

  QJsonParseError error{};
  const auto document = QJsonDocument::fromJson(json, &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    LOG_W() << "settings policy is not a JSON object:" << error.errorString();
    return defaults;
  }

  const auto entries = document.object().value("defaults").toObject();
  for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
    if (!IsCatalogKey(it.key())) {
      LOG_W() << "settings policy names an unknown setting:" << it.key();
      continue;
    }
    if (!it->isBool() && !it->isDouble() && !it->isString()) {
      LOG_W() << "settings policy gives an unusable value for:" << it.key();
      continue;
    }
    defaults.insert(it.key(), it->toVariant());
  }
  return defaults;
}

auto FlavorSettingDefault(const char *key) -> QVariant {
  static const auto kDefaults = []() -> QHash<QString, QVariant> {
    QFile file(QString::fromLatin1(kFlavorSettingsPolicy));
    if (!file.open(QIODevice::ReadOnly)) return {};
    return ParseSettingsPolicy(file.readAll());
  }();
  return kDefaults.value(QLatin1StringView(key));
}

}  // namespace GpgFrontend
