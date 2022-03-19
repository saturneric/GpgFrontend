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

#ifndef GPGFRONTEND_ZH_CN_TS_GPGKEYGETTER_H
#define GPGFRONTEND_ZH_CN_TS_GPGKEYGETTER_H

#include "core/GpgContext.h"
#include "core/GpgFunctionObject.h"
#include "core/GpgModel.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GpgKeyGetter : public SingletonFunctionObject<GpgKeyGetter> {
 public:
  /**
   * @brief Construct a new Gpg Key Getter object
   *
   * @param channel
   */
  explicit GpgKeyGetter(
      int channel = SingletonFunctionObject::GetDefaultChannel())
      : SingletonFunctionObject<GpgKeyGetter>(channel) {}

  /**
   * @brief Get the Key object
   *
   * @param fpr
   * @return GpgKey
   */
  GpgKey GetKey(const std::string& fpr);

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
  GpgKey GetPubkey(const std::string& fpr);

  /**
   * @brief
   *
   * @return KeyLinkListPtr
   */
  KeyLinkListPtr FetchKey();

  /**
   * @brief Get the Keys Copy object
   *
   * @param keys
   * @return KeyListPtr
   */
  static KeyListPtr GetKeysCopy(const KeyListPtr& keys);

  /**
   * @brief Get the Keys Copy object
   *
   * @param keys
   * @return KeyLinkListPtr
   */
  static KeyLinkListPtr GetKeysCopy(const KeyLinkListPtr& keys);

 private:
  /**
   * @brief
   *
   */
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_GPGKEYGETTER_H
