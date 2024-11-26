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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/gpg/GpgContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeyManager
    : public SingletonFunctionObject<GpgKeyManager> {
 public:
  /**
   * @brief Construct a new Gpg Key Manager object
   *
   * @param channel
   */
  explicit GpgKeyManager(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Sign a key pair(actually a certain uid)
   * @param target target key pair
   * @param uid target
   * @param expires expire date and time of the signature
   * @return if successful
   */
  auto SignKey(const GpgKey& target, KeyArgsList& keys, const QString& uid,
               const std::unique_ptr<QDateTime>& expires) -> bool;

  /**
   * @brief
   *
   * @param key
   * @param signature_id
   * @return true
   * @return false
   */
  auto RevSign(const GpgFrontend::GpgKey& key,
               const GpgFrontend::SignIdArgsListPtr& signature_id) -> bool;

  /**
   * @brief Set the Expire object
   *
   * @param key
   * @param subkey
   * @param expires
   * @return true
   * @return false
   */
  auto SetExpire(const GpgKey& key, std::unique_ptr<GpgSubKey>& subkey,
                 std::unique_ptr<QDateTime>& expires) -> bool;

  /**
   * @brief
   *
   * @return
   */
  auto SetOwnerTrustLevel(const GpgKey& key, int trust_level) -> bool;

  /**
   * @brief
   *
   * @param key
   * @param subkey_index
   * @return true
   * @return false
   */
  auto DeleteSubkey(const GpgKey& key, int subkey_index) -> bool;

  /**
   * @brief
   *
   * @param key
   * @param subkey_index
   * @return true
   * @return false
   */
  auto RevokeSubkey(const GpgKey& key, int subkey_index, int reason_code,
                    const QString& reason_text) -> bool;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

}  // namespace GpgFrontend
