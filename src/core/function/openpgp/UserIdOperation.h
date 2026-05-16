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

/**
 * @brief Singleton for user ID (UID) management operations on OpenPGP keys.
 *
 * Provides adding, deleting, revoking, and setting the primary UID on a key.
 */
class GF_CORE_EXPORT UserIdOperation
    : public SingletonFunctionObject<UserIdOperation> {
 public:
  /**
   * @brief Construct the operation handler for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit UserIdOperation(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Add a new user ID to the given key using a pre-formatted UID string.
   *
   * @param key target key pair
   * @param uid user ID string in "Name (Comment) <email>" format
   * @return true on success
   */
  auto AddUID(const GpgKeyPtr& key, const QString& uid) -> bool;

  /**
   * @brief Add a new user ID to the given key from individual
   * name/comment/email components.
   *
   * @param key target key pair
   * @param name display name
   * @param comment optional comment string
   * @param email email address
   * @return true on success
   */
  auto AddUID(const GpgKeyPtr& key, const QString& name, const QString& comment,
              const QString& email) -> bool;

  /**
   * @brief Delete a user ID from the given key.
   *
   * @param key key to modify
   * @param uid user ID string identifying the UID to delete
   * @return true on success
   */
  auto DeleteUID(const GpgKeyPtr& key, const QString& uid) -> bool;

  /**
   * @brief Revoke a user ID on the given key.
   *
   * @param key key to modify
   * @param uid user ID string identifying the UID to revoke
   * @param reason_code revocation reason code
   * @param reason_text human-readable reason description
   * @return true on success
   */
  auto RevokeUID(const GpgKeyPtr& key, const QString& uid, int reason_code,
                 const QString& reason_text) -> bool;

  /**
   * @brief Set the given user ID as the primary UID of the key.
   *
   * @param key target key pair
   * @param uid user ID string to promote as primary
   * @return true on success
   */
  auto SetPrimaryUID(const GpgKeyPtr& key, const QString& uid) -> bool;

 private:
  // OpenPGP context for this channel.
  OpenPGPContext& ctx_ =
      OpenPGPContext::GetInstance(SingletonFunctionObject::GetChannel());
};

}  // namespace GpgFrontend
