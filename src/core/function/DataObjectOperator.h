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

#ifndef GPGFRONTEND_DATAOBJECTOPERATOR_H
#define GPGFRONTEND_DATAOBJECTOPERATOR_H

#include <json/single_include/nlohmann/json.hpp>

#include "core/GpgFrontendCore.h"
#include "core/GpgFunctionObject.h"
#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend {

class DataObjectOperator : public SingletonFunctionObject<DataObjectOperator> {
 public:
  /**
   * @brief DataObjectOperator constructor
   *
   * @param channel channel
   */
  explicit DataObjectOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  std::string SaveDataObj(const std::string &_key, const nlohmann::json &value);

  std::optional<nlohmann::json> GetDataObject(const std::string &_key);

  std::optional<nlohmann::json> GetDataObjectByRef(const std::string &_ref);

 private:
  /**
   * @brief init the secure key of application data object
   *
   */
  void init_app_secure_key();

  GlobalSettingStation &global_setting_station_ =
      GlobalSettingStation::GetInstance();  ///< GlobalSettingStation
  std::filesystem::path app_secure_path_ =
      global_setting_station_.GetAppConfigPath() /
      "secure";  ///< Where sensitive information is stored
  std::filesystem::path app_secure_key_path_ =
      app_secure_path_ / "app.key";  ///< Where the key of data object is stored
  std::filesystem::path app_data_objs_path_ =
      global_setting_station_.GetAppDataPath() / "data_objs";  ///< Where data
                                                               ///< object is
                                                               ///< stored

  std::random_device rd_;                  ///< Random device
  std::mt19937 mt_ = std::mt19937(rd_());  ///< Mersenne twister
  QByteArray hash_key_;                    ///< Hash key
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_DATAOBJECTOPERATOR_H
