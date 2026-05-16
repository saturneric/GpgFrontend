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
 * @brief Unified singleton repository for looking up both GPG keys and key
 * groups.
 *
 * Delegates to GpgKeyRepository and KeyGroupRepository. Callers that need
 * a key or key group should use this class rather than accessing the
 * underlying repositories directly.
 */
class GF_CORE_EXPORT AbstractKeyRepository
    : public SingletonFunctionObject<AbstractKeyRepository> {
 public:
  /**
   * @brief Construct the repository for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit AbstractKeyRepository(int channel = kGpgFrontendDefaultChannel);

  ~AbstractKeyRepository();

  /**
   * @brief Look up a key (GPG key or key group) by its key ID or fingerprint.
   *
   * @param key_id key ID or fingerprint string
   * @return shared pointer to the abstract key, or nullptr if not found
   */
  auto GetKey(const QString& key_id) -> GpgAbstractKeyPtr;

  /**
   * @brief Look up multiple keys by their key IDs or fingerprints.
   *
   * @param key_ids list of key ID or fingerprint strings
   * @return list of abstract key pointers (nullptr entries for missing keys)
   */
  auto GetKeys(const QStringList& key_ids) -> GpgAbstractKeyPtrList;

  /**
   * @brief Return all keys and key groups known to this channel.
   *
   * @return list of all abstract key pointers
   */
  auto Fetch() -> GpgAbstractKeyPtrList;

  /**
   * @brief Flush the key cache, forcing the next Fetch() to reload from
   * storage.
   *
   * @return true if the cache was flushed successfully
   */
  auto FlushCache() -> bool;

  /**
   * @brief Return the QAbstractTableModel for displaying GPG keys in a view.
   *
   * @return shared pointer to the GpgKeyTableModel for this channel
   */
  auto GetGpgKeyTableModel() -> QSharedPointer<GpgKeyTableModel>;

 private:
  // Underlying GPG key repository for this channel.
  GpgKeyRepository& key_ =
      GpgKeyRepository::GetInstance(SingletonFunctionObject::GetChannel());
  // Underlying key group repository for this channel.
  KeyGroupRepository& kg_ =
      KeyGroupRepository::GetInstance(SingletonFunctionObject::GetChannel());
};
}  // namespace GpgFrontend
