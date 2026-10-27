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

#include "ui/function/KeyGenSettings.h"

#include "core/model/CacheObject.h"
#include "core/model/SettingsObject.h"

namespace GpgFrontend::UI {

auto KeyGenSettingsFromStored(const QJsonObject& stored,
                              const QJsonArray& legacy) -> KeyGenSO {
  KeyGenSO settings(stored);
  if (settings.profiles_stored) return settings;

  settings.profiles = KeyGenProfilesFromJson(legacy);
  if (settings.profiles.isEmpty()) {
    settings.profiles = MakeDefaultKeyGenProfiles();
  }
  return settings;
}

auto LoadKeyGenSettings() -> KeyGenSO {
  SettingsObject so(QString::fromLatin1(kKeyGenSettingsObject));

  QJsonArray legacy;
  if (!so.contains("profiles")) {
    const CacheObject cache(QString::fromLatin1(kLegacyKeyGenProfileCache));
    if (cache.isArray()) legacy = cache.array();
  }

  auto settings = KeyGenSettingsFromStored(so, legacy);
  if (!settings.profiles_stored && !so.LoadFailed()) {
    // Written once, so the old cache is read only the first time. The cache is
    // left alone: a downgraded build still finds its profiles there.
    settings.profiles_stored = true;
    so.Store(settings.ToJson());
  }
  return settings;
}

void StoreKeyGenSettings(const KeyGenSO& settings) {
  SettingsObject so(QString::fromLatin1(kKeyGenSettingsObject));
  auto json = settings.ToJson();

  // Keys a newer build added are kept rather than dropped by an older one.
  for (auto it = so.constBegin(); it != so.constEnd(); ++it) {
    if (!json.contains(it.key())) json[it.key()] = it.value();
  }
  so.Store(json);
}

void StoreKeyGenExpertMode(bool on) {
  auto settings = LoadKeyGenSettings();
  if (settings.expert_mode == on) return;
  settings.expert_mode = on;
  StoreKeyGenSettings(settings);
}

auto OfferedKeyGenProfiles(int channel, const QList<KeyGenProfile>& profiles)
    -> QList<ResolvedKeyGenProfile> {
  return ResolveKeyGenProfiles(
      profiles, KeyGenerateInfo::GetSupportedKeyAlgo(channel),
      KeyGenerateInfo::GetSupportedSubkeyAlgo(channel),
      [channel](const KeyAlgo& algo) { return algo.SubAlgos(channel); });
}

}  // namespace GpgFrontend::UI
