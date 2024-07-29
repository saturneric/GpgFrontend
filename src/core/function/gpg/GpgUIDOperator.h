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

#include "core/function/gpg/GpgContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgUIDOperator
    : public SingletonFunctionObject<GpgUIDOperator> {
 public:
  /**
   * @brief Construct a new Gpg UID Opera object
   *
   * @param channel
   */
  explicit GpgUIDOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * create a new uid in certain key pair
   * @param key target key pair
   * @param uid uid args(combine name&comment&email)
   * @return if successful
   */
  auto AddUID(const GpgKey& key, const QString& uid) -> bool;

  /**
   * create a new uid in certain key pair
   * @param key target key pair
   * @param name
   * @param comment
   * @param email
   * @return
   */
  auto AddUID(const GpgKey& key, const QString& name, const QString& comment,
              const QString& email) -> bool;

  /**
   * Revoke(Delete) UID from certain key pair
   * @param key target key pair
   * @param uid target uid
   * @return if successful
   */
  auto RevUID(const GpgKey& key, const QString& uid) -> bool;

  /**
   * Set one of a uid of a key pair as primary
   * @param key target key pair
   * @param uid target uid
   * @return if successful
   */
  auto SetPrimaryUID(const GpgKey& key, const QString& uid) -> bool;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
};

}  // namespace GpgFrontend
