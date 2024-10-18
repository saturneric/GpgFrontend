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

#include <functional>

#include "core/function/gpg/GpgContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GenKeyInfo;

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeyOpera
    : public SingletonFunctionObject<GpgKeyOpera> {
 public:
  /**
   * @brief Construct a new Gpg Key Opera object
   *
   * @param channel
   */
  explicit GpgKeyOpera(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @param key_ids
   */
  void DeleteKeys(KeyIdArgsListPtr key_ids);

  /**
   * @brief
   *
   * @param key_id
   */
  void DeleteKey(const KeyId& key_id);

  /**
   * @brief Set the Expire object
   *
   * @param key
   * @param subkey_fpr
   * @param expires
   * @return GpgError
   */
  auto SetExpire(const GpgKey& key, const SubkeyId& subkey_fpr,
                 std::unique_ptr<QDateTime>& expires) -> GpgError;

  /**
   * @brief
   *
   * @param key
   * @param output_file_name
   */
  void GenerateRevokeCert(const GpgKey& key, const QString& output_path,
                          int revocation_reason_code,
                          const QString& revocation_reason_text);

  /**
   * @brief
   *
   * @param key
   * @return GpgFrontend::GpgError
   */
  void ModifyPassword(const GpgKey& key, const GpgOperationCallback&);

  /**
   * @brief
   *
   * @param key
   * @param tofu_policy
   * @return GpgFrontend::GpgError
   */
  auto ModifyTOFUPolicy(const GpgKey& key, gpgme_tofu_policy_t tofu_policy)
      -> GpgFrontend::GpgError;
  /**
   * @brief
   *
   * @param params
   * @param result
   * @return GpgFrontend::GpgError
   */
  void GenerateKey(const std::shared_ptr<GenKeyInfo>&,
                   const GpgOperationCallback&);

  /**
   * @brief
   *
   * @param params
   */
  auto GenerateKeySync(const std::shared_ptr<GenKeyInfo>& params)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief
   *
   * @param key
   * @param params
   * @return GpgFrontend::GpgError
   */
  void GenerateSubkey(const GpgKey& key,
                      const std::shared_ptr<GenKeyInfo>& params,
                      const GpgOperationCallback&);

  /**
   * @brief
   *
   * @param key
   * @param params
   */
  auto GenerateSubkeySync(const GpgKey& key,
                          const std::shared_ptr<GenKeyInfo>& params)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief
   *
   * @param params
   * @param subkey_params
   * @param callback
   */
  void GenerateKeyWithSubkey(const std::shared_ptr<GenKeyInfo>& params,
                             const std::shared_ptr<GenKeyInfo>& subkey_params,
                             const GpgOperationCallback& callback);

  /**
   * @brief
   *
   * @param params
   * @param subkey_params
   * @param callback
   */
  auto GenerateKeyWithSubkeySync(
      const std::shared_ptr<GenKeyInfo>& params,
      const std::shared_ptr<GenKeyInfo>& subkey_params)
      -> std::tuple<GpgError, DataObjectPtr>;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};
}  // namespace GpgFrontend
