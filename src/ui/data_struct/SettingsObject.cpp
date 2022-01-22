/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "SettingsObject.h"

nlohmann::json& GpgFrontend::UI::SettingsObject::Check(
    const std::string& key, nlohmann::json default_value) {
  if (!nlohmann::json::contains(key))
    nlohmann::json::operator[](key) = std::move(default_value);
  return nlohmann::json::operator[](key);
}

GpgFrontend::UI::SettingsObject GpgFrontend::UI::SettingsObject::Check(
    const std::string& key) {
  if (!nlohmann::json::contains(key)) nlohmann::json::operator[](key) = {};
  return SettingsObject{nlohmann::json::operator[](key), false};
}

GpgFrontend::UI::SettingsObject::SettingsObject(std::string settings_name)
    : settings_name_(std::move(settings_name)) {
  try {
    auto _json_optional =
        GlobalSettingStation::GetInstance().GetDataObject(settings_name_);

    if (_json_optional.has_value()) {
      nlohmann::json::operator=(_json_optional.value());
    }

  } catch (...) {
  }
}

GpgFrontend::UI::SettingsObject::SettingsObject(nlohmann::json _sub_json, bool)
    : nlohmann::json(std::move(_sub_json)), settings_name_({}) {}

GpgFrontend::UI::SettingsObject::~SettingsObject() {
  if (!settings_name_.empty())
    GpgFrontend::UI::GlobalSettingStation::GetInstance().SaveDataObj(
        settings_name_, *this);
}
