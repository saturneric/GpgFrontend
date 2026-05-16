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

#include "core/function/openpgp/OpenPGPContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

// Forward declaration; defined in core/model/KeyGenerateInfo.h.
class KeyGenerateInfo;

/**
 * @brief Singleton for key and subkey generation operations.
 *
 * Wraps the engine-dispatched key generation API, providing both async
 * (callback-based) and synchronous entry points.
 */
class GF_CORE_EXPORT KeyGenerationOperation
    : public SingletonFunctionObject<KeyGenerationOperation> {
 public:
  /**
   * @brief Construct the operation handler for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit KeyGenerationOperation(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Generate a new primary key pair (async).
   *
   * @param params key generation parameters (algorithm, key size, expiry, etc.)
   * @param cb callback invoked on completion with (GpgError, DataObjectPtr)
   */
  void GenerateKey(const QSharedPointer<KeyGenerateInfo>& params,
                   const GpgOperationCallback& cb);

  /**
   * @brief Generate a new primary key pair (sync).
   *
   * @param params key generation parameters
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto GenerateKeySync(const QSharedPointer<KeyGenerateInfo>& params)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Add a new subkey to an existing primary key pair (async).
   *
   * @param key primary key to add the subkey to
   * @param params subkey generation parameters
   * @param cb callback invoked on completion
   */
  void GenerateSubkey(const GpgKeyPtr& key,
                      const QSharedPointer<KeyGenerateInfo>& params,
                      const GpgOperationCallback& cb);

  /**
   * @brief Add a new subkey to an existing primary key pair (sync).
   *
   * @param key primary key to add the subkey to
   * @param params subkey generation parameters
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto GenerateSubkeySync(const GpgKeyPtr& key,
                          const QSharedPointer<KeyGenerateInfo>& params)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Generate a new primary key with an initial subkey (async).
   *
   * @param p_params primary key generation parameters
   * @param s_params subkey generation parameters
   * @param cb callback invoked on completion
   */
  void GenerateKeyWithSubkey(const QSharedPointer<KeyGenerateInfo>& p_params,
                             const QSharedPointer<KeyGenerateInfo>& s_params,
                             const GpgOperationCallback& cb);

  /**
   * @brief Generate a new primary key with an initial subkey (sync).
   *
   * @param p_params primary key generation parameters
   * @param s_params subkey generation parameters
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto GenerateKeyWithSubkeySync(
      const QSharedPointer<KeyGenerateInfo>& p_params,
      const QSharedPointer<KeyGenerateInfo>& s_params)
      -> std::tuple<GpgError, DataObjectPtr>;

 private:
  // OpenPGP context for this channel.
  OpenPGPContext& ctx_ =
      OpenPGPContext::GetInstance(SingletonFunctionObject::GetChannel());
};
}  // namespace GpgFrontend
