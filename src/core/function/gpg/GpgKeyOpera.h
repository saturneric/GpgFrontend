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

#include "core/GpgConstants.h"
#include "core/GpgContext.h"
#include "core/GpgModel.h"

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
  GpgError SetExpire(const GpgKey& key, const SubkeyId& subkey_fpr,
                     std::unique_ptr<boost::posix_time::ptime>& expires);

  /**
   * @brief
   *
   * @param key
   * @param output_file_name
   */
  void GenerateRevokeCert(const GpgKey& key,
                          const std::string& output_file_name);

  /**
   * @brief
   *
   * @param key
   * @return GpgFrontend::GpgError
   */
  GpgFrontend::GpgError ModifyPassword(const GpgKey& key);

  /**
   * @brief
   *
   * @param key
   * @param tofu_policy
   * @return GpgFrontend::GpgError
   */
  GpgFrontend::GpgError ModifyTOFUPolicy(const GpgKey& key,
                                         gpgme_tofu_policy_t tofu_policy);
  /**
   * @brief
   *
   * @param params
   * @param result
   * @return GpgFrontend::GpgError
   */
  GpgFrontend::GpgError GenerateKey(const std::unique_ptr<GenKeyInfo>& params,
                                    GpgGenKeyResult& result);

  /**
   * @brief
   *
   * @param key
   * @param params
   * @return GpgFrontend::GpgError
   */
  GpgFrontend::GpgError GenerateSubkey(
      const GpgKey& key, const std::unique_ptr<GenKeyInfo>& params);

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};
}  // namespace GpgFrontend
