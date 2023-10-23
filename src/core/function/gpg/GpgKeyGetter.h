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

#include <mutex>
#include <vector>

#include "core/GpgContext.h"
#include "core/GpgFunctionObject.h"
#include "core/GpgModel.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeyGetter
    : public SingletonFunctionObject<GpgKeyGetter> {
 public:
  /**
   * @brief Construct a new Gpg Key Getter object
   *
   * @param channel
   */
  explicit GpgKeyGetter(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Get the Key object
   *
   * @param fpr
   * @return GpgKey
   */
  GpgKey GetKey(const std::string& id, bool use_cache = true);

  /**
   * @brief Get the Keys object
   *
   * @param ids
   * @return KeyListPtr
   */
  KeyListPtr GetKeys(const KeyIdArgsListPtr& ids);

  /**
   * @brief Get the Pubkey object
   *
   * @param fpr
   * @return GpgKey
   */
  GpgKey GetPubkey(const std::string& id, bool use_cache = true);

  /**
   * @brief Get all the keys by receiving a linked list
   *
   * @return KeyLinkListPtr
   */
  KeyLinkListPtr FetchKey();

  /**
   * @brief flush the keys in the cache
   *
   */
  void FlushKeyCache();

  /**
   * @brief Get the Keys Copy object
   *
   * @param keys
   * @return KeyListPtr
   */
  KeyListPtr GetKeysCopy(const KeyListPtr& keys);

  /**
   * @brief Get the Keys Copy object
   *
   * @param keys
   * @return KeyLinkListPtr
   */
  KeyLinkListPtr GetKeysCopy(const KeyLinkListPtr& keys);

 private:
  /**
   * @brief Get the gpgme context object
   *
   */
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());

  /**
   * @brief shared mutex for the keys cache
   *
   */
  mutable std::mutex ctx_mutex_;

  /**
   * @brief cache the keys with key id
   *
   */
  std::map<std::string, GpgKey> keys_cache_;

  /**
   * @brief shared mutex for the keys cache
   *
   */
  mutable std::mutex keys_cache_mutex_;

  /**
   * @brief Get the Key object
   *
   * @param id
   * @return GpgKey
   */
  GpgKey get_key_in_cache(const std::string& id);
};
}  // namespace GpgFrontend
