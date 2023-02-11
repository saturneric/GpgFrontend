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
    const std::string& key, const nlohmann::json& default_value) {
  // check if the self null
  if (this->nlohmann::json::is_null()) {
    SPDLOG_DEBUG("settings object is null, creating new one");
    this->nlohmann::json::operator=(nlohmann::json::object());
  }

  try {
    if (!this->nlohmann::json::contains(key) ||
        this->nlohmann::json::at(key).is_null() ||
        this->nlohmann::json::at(key).type_name() !=
            default_value.type_name()) {
      SPDLOG_DEBUG("added missing key: {}", key);
      if (default_value.is_null()) {
        SPDLOG_WARN("default value is null, using empty object");
        this->nlohmann::json::operator[](key) = nlohmann::json::object();
      } else {
        this->nlohmann::json::operator[](key) = default_value;
      }
    }
    return this->nlohmann::json::at(key);
  } catch (nlohmann::json::exception& e) {
    SPDLOG_ERROR(e.what());
    throw e;
  }
}

GpgFrontend::UI::SettingsObject GpgFrontend::UI::SettingsObject::Check(
    const std::string& key) {
  // check if the self null
  if (this->nlohmann::json::is_null()) {
    SPDLOG_DEBUG("settings object is null, creating new one");
    this->nlohmann::json::operator=(nlohmann::json::object());
  }

  if (!nlohmann::json::contains(key) ||
      this->nlohmann::json::at(key).is_null() ||
      this->nlohmann::json::at(key).type() != nlohmann::json::value_t::object) {
    SPDLOG_DEBUG("added missing key: {}", key);
    this->nlohmann::json::operator[](key) = nlohmann::json::object();
  }
  return SettingsObject{nlohmann::json::operator[](key), false};
}

GpgFrontend::UI::SettingsObject::SettingsObject(std::string settings_name)
    : settings_name_(std::move(settings_name)) {
  try {
    SPDLOG_DEBUG("loading settings from: {}", this->settings_name_);
    auto _json_optional =
        GpgFrontend::DataObjectOperator::GetInstance().GetDataObject(
            settings_name_);

    if (_json_optional.has_value()) {
      SPDLOG_DEBUG("settings object: {} loaded.", settings_name_);
      nlohmann::json::operator=(_json_optional.value());
    } else {
      SPDLOG_DEBUG("settings object: {} not found.", settings_name_);
      nlohmann::json::operator=({});
    }

  } catch (std::exception& e) {
    SPDLOG_ERROR(e.what());
  }
}

GpgFrontend::UI::SettingsObject::SettingsObject(nlohmann::json _sub_json, bool)
    : nlohmann::json(std::move(_sub_json)), settings_name_({}) {}

GpgFrontend::UI::SettingsObject::~SettingsObject() {
  if (!settings_name_.empty())
    GpgFrontend::DataObjectOperator::GetInstance().SaveDataObj(settings_name_,
                                                               *this);
}