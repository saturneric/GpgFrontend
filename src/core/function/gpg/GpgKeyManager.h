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

#ifndef GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
#define GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H

#include "core/GpgContext.h"
#include "core/GpgFunctionObject.h"
#include "core/GpgModel.h"

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
  bool SignKey(const GpgKey& target, KeyArgsList& keys, const std::string& uid,
               const std::unique_ptr<boost::posix_time::ptime>& expires);

  /**
   * @brief
   *
   * @param key
   * @param signature_id
   * @return true
   * @return false
   */
  bool RevSign(const GpgFrontend::GpgKey& key,
               const GpgFrontend::SignIdArgsListPtr& signature_id);

  /**
   * @brief Set the Expire object
   *
   * @param key
   * @param subkey
   * @param expires
   * @return true
   * @return false
   */
  bool SetExpire(const GpgKey& key, std::unique_ptr<GpgSubKey>& subkey,
                 std::unique_ptr<boost::posix_time::ptime>& expires);

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_GPGKEYMANAGER_H
