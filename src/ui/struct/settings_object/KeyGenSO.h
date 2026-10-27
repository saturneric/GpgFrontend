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

#include "ui/function/KeyGenProfile.h"
#include "ui/function/UidPattern.h"

namespace GpgFrontend::UI {

/// The settings object the key generation settings are stored under.
inline constexpr const char* kKeyGenSettingsObject = "key_gen_settings";

/// The durable cache the profiles lived in before they moved to Settings.
inline constexpr const char* kLegacyKeyGenProfileCache =
    "key_gen_easy_mode_config";

/**
 * @brief The key generation settings: the profiles, which one Quick Generate
 * uses, and how it makes up a user id.
 */
struct KeyGenSO {
  QList<KeyGenProfile> profiles;
  QString default_profile;  ///< a profile's stored name; empty for the built-in
  UidPattern uid_pattern;
  /// Whether the key and subkey dialogs open with every choice shown. One
  /// switch for all of them: whoever wants it in one wants it in the others.
  bool expert_mode = false;
  /// Whether `profiles` was ever written. An installation that has deleted
  /// every profile is not one that never had any.
  bool profiles_stored = false;

  explicit KeyGenSO(const QJsonObject& j) {
    if (const auto v = j["profiles"]; v.isArray()) {
      profiles = KeyGenProfilesFromJson(v.toArray());
      profiles_stored = true;
    }
    if (const auto v = j["default_profile"]; v.isString()) {
      default_profile = v.toString();
    }
    if (const auto v = j["uid_pattern"]; v.isObject()) {
      uid_pattern = UidPatternFromJson(v.toObject());
    }
    if (const auto v = j["expert_mode"]; v.isBool()) {
      expert_mode = v.toBool();
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    j["profiles"] = KeyGenProfilesToJson(profiles);
    j["default_profile"] = default_profile;
    j["uid_pattern"] = UidPatternToJson(uid_pattern);
    j["expert_mode"] = expert_mode;
    return j;
  }
};

}  // namespace GpgFrontend::UI
