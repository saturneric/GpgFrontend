/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT DataObjectOperator
    : public SingletonFunctionObject<DataObjectOperator> {
 public:
  /**
   * @brief DataObjectOperator constructor
   *
   * @param channel channel
   */
  explicit DataObjectOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  auto SaveDataObj(const QString &_key, const QJsonDocument &value) -> QString;

  auto GetDataObject(const QString &_key) -> std::optional<QJsonDocument>;

  auto GetDataObjectByRef(const QString &_ref) -> std::optional<QJsonDocument>;

 private:
  /**
   * @brief init the secure key of application data object
   *
   */
  void init_app_secure_key();

  GlobalSettingStation &global_setting_station_ =
      GlobalSettingStation::GetInstance();  ///< GlobalSettingStation
  QString app_secure_path_ =
      global_setting_station_.GetAppConfigPath() +
      "/secure";  ///< Where sensitive information is stored
  QString app_secure_key_path_ =
      app_secure_path_ +
      "/app.key";  ///< Where the key of data object is stored
  QString app_data_objs_path_ =
      global_setting_station_.GetAppDataPath() + "/data_objs";

  QByteArray hash_key_;  ///< Hash key
};

}  // namespace GpgFrontend
