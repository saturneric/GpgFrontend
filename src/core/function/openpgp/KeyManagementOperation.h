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

#include "core/function/gpg/GpgContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GF_CORE_EXPORT KeyManagementOperation
    : public SingletonFunctionObject<KeyManagementOperation> {
 public:
  /**
   * @brief Construct a new Gpg Key Opera object
   *
   * @param channel
   */
  explicit KeyManagementOperation(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @param key_ids
   */
  void DeleteKeys(const GpgAbstractKeyPtrList& keys);

  /**
   * @brief
   *
   * @param key_id
   */
  void DeleteKey(const GpgAbstractKeyPtr& key_id);

  /**
   * @brief Set the Expire object
   *
   * @param key
   * @param skey_fpr
   * @param expires
   * @return GpgError
   */
  auto SetExpire(const GpgKeyPtr& key, const SubkeyId& skey_fpr,
                 const std::optional<QDateTime>& expires) -> GpgError;
  /**
   * @brief
   *
   * @param key
   * @param output_path
   * @param reason_code
   * @param reason_text
   */
  void GenerateRevokeCert(const GpgKeyPtr& key, const QString& output_path,
                          int reason_code, const QString& reason_text);

  /**
   * @brief
   *
   * @param key
   * @return GpgFrontend::GpgError
   */
  void ModifyPassword(const GpgKeyPtr& key, const GpgOperationCallback&);

  /**
   * @brief
   *
   * @param key
   * @param adsk
   */
  void AddADSK(const GpgKeyPtr& key, const GpgSubKey& adsk,
               const GpgOperationCallback&);

  /**
   * @brief
   *
   * @param key
   * @param adsk
   * @return GpgError
   */
  auto AddADSKSync(const GpgKeyPtr& key, const GpgSubKey& adsk)
      -> std::tuple<GpgError, DataObjectPtr>;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};
}  // namespace GpgFrontend
