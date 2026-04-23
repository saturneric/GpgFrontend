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
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGroupRepository.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GpgKeyTableModel;

/**
 * @brief
 *
 */
class GF_CORE_EXPORT AbstractKeyRepository
    : public SingletonFunctionObject<AbstractKeyRepository> {
 public:
  /**
   * @brief Construct a new Gpg Key Getter object
   *
   * @param channel
   */
  explicit AbstractKeyRepository(int channel = kGpgFrontendDefaultChannel);

  /**
   * @brief Destroy the Gpg Key Getter object
   *
   */
  ~AbstractKeyRepository();

  /**
   * @brief Get the Key object
   *
   * @param fpr
   * @return GpgKey
   */
  auto GetKey(const QString& key_id) -> GpgAbstractKeyPtr;

  /**
   * @brief Get the Keys object
   *
   * @param key_ids
   * @return auto
   */
  auto GetKeys(const QStringList& key_ids) -> GpgAbstractKeyPtrList;

  /**
   * @brief Get all the keys by receiving a linked list
   *
   * @return KeyLinkListPtr
   */
  auto Fetch() -> GpgAbstractKeyPtrList;

  /**
   * @brief flush the keys in the cache
   *
   */
  auto FlushCache() -> bool;

  /**
   * @brief
   *
   * @return GpgKeyTableModel
   */
  auto GetGpgKeyTableModel() -> QSharedPointer<GpgKeyTableModel>;

 private:
  GpgKeyRepository& key_ =
      GpgKeyRepository::GetInstance(SingletonFunctionObject::GetChannel());
  KeyGroupRepository& kg_ =
      KeyGroupRepository::GetInstance(SingletonFunctionObject::GetChannel());
};
}  // namespace GpgFrontend
