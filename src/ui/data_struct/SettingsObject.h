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

#ifndef GPGFRONTEND_SETTINGSOBJECT_H
#define GPGFRONTEND_SETTINGSOBJECT_H

#include <utility>

#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class SettingsObject : public nlohmann::json {
 public:
  /**
   * @brief Construct a new Settings Object object
   *
   * @param settings_name
   */
  explicit SettingsObject(std::string settings_name);

  /**
   * @brief Construct a new Settings Object object
   *
   * @param _sub_json
   */
  explicit SettingsObject(nlohmann::json _sub_json, bool);

  /**
   * @brief Destroy the Settings Object object
   *
   */
  ~SettingsObject();

  /**
   * @brief
   *
   * @param key
   * @param default_value
   * @return nlohmann::json&
   */
  nlohmann::json& Check(const std::string& key, nlohmann::json default_value);

  /**
   * @brief
   *
   * @param key
   * @return SettingsObject
   */
  SettingsObject Check(const std::string& key);

 private:
  std::string settings_name_;  ///<
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSOBJECT_H
