/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "SettingsObj.h"

nlohmann::json& GpgFrontend::UI::SettingsObj::Check(
    const std::string& key, nlohmann::json default_value) {
  if (!nlohmann::json::contains(key))
    nlohmann::json::operator[](key) = std::move(default_value);
  return nlohmann::json::operator[](key);
}

GpgFrontend::UI::SettingsObj GpgFrontend::UI::SettingsObj::Check(
    const std::string& key) {
  if (!nlohmann::json::contains(key)) nlohmann::json::operator[](key) = {};
  return SettingsObj{nlohmann::json::operator[](key), false};
}

GpgFrontend::UI::SettingsObj::SettingsObj(std::string settings_name)
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

GpgFrontend::UI::SettingsObj::SettingsObj(nlohmann::json _sub_json, bool)
    : nlohmann::json(std::move(_sub_json)), settings_name_({}) {}
