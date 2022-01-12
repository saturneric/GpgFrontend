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

#ifndef GPGFRONTEND_SETTINGSOBJ_H
#define GPGFRONTEND_SETTINGSOBJ_H

#include <utility>

#include "ui/settings/GlobalSettingStation.h"

namespace GpgFrontend::UI {

class SettingsObj : public nlohmann::json {
 public:
  explicit SettingsObj(std::string settings_name);

  explicit SettingsObj(nlohmann::json _sub_json, bool);

  ~SettingsObj() {
    GpgFrontend::UI::GlobalSettingStation::GetInstance().SaveDataObj(
        settings_name_, *this);
  }

  nlohmann::json& Check(const std::string& key, nlohmann::json default_value);

  SettingsObj Check(const std::string& key);

 private:
  std::string settings_name_;
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSOBJ_H
