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

#include <optional>

#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

class GF_CORE_EXPORT DataObjectOperator
    : public SingletonFunctionObject<DataObjectOperator> {
 public:
  /**
   * @brief DataObjectOperator constructor
   *
   * @param channel channel
   */
  explicit DataObjectOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @param key
   * @param value
   * @return QString
   */
  auto StoreDataObj(const QString &key, const QJsonDocument &value) -> QString;

  /**
   * @brief Get the Data Object object
   *
   * @param key
   * @return std::optional<QJsonDocument>
   */
  auto GetDataObject(const QString &key) -> std::optional<QJsonDocument>;

  /**
   * @brief Get the Data Object By Ref object
   *
   * @param ref
   * @return std::optional<QJsonDocument>
   */
  auto GetDataObjectByRef(const QString &ref) -> std::optional<QJsonDocument>;

  /**
   * @brief
   *
   * @param key
   * @param value
   * @return QString
   */
  auto StoreSecDataObj(const QString &key, const GFBuffer &value) -> QString;

  /**
   * @brief Get the Sec Data Object object
   *
   * @param key
   * @return GFBufferOrNone
   */
  auto GetSecDataObject(const QString &key) -> GFBufferOrNone;

  /**
   * @brief Get the Sec Data Object By Ref object
   *
   * @param ref
   * @return GFBufferOrNone
   */
  auto GetSecDataObjectByRef(const QString &ref) -> GFBufferOrNone;

 private:
  GlobalSettingStation &gss_ =
      GlobalSettingStation::GetInstance();  ///< GlobalSettingStation
  GFBuffer key_;                            ///< Raw key

  /**
   * @brief Get the object ref object
   *
   * @param key
   * @return QByteArray
   */
  auto get_object_ref(const QString &obj_name) -> GFBuffer;

  /**
   * @brief
   *
   * @param ref
   * @return GFBufferOrNone
   */
  auto read_decr_object(const GFBuffer &ref) -> GFBufferOrNone;

  /**
   * @brief
   *
   * @param ref
   * @return std::optional<QJsonDocument>
   */
  auto read_decr_json_object(const GFBuffer &ref)
      -> std::optional<QJsonDocument>;
};

}  // namespace GpgFrontend
