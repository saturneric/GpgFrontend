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

#include "core/function/SecureMemoryAllocator.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class ChannelObject;

using ChannelObjectPtr = SecureUniquePtr<ChannelObject>;

class GF_CORE_EXPORT SingletonStorage {
 public:
  /**
   * @brief
   *
   */
  SingletonStorage() noexcept;

  /**
   * @brief
   *
   */
  ~SingletonStorage();

  /**
   * @brief
   *
   * @param channel
   */
  void ReleaseChannel(int channel);

  /**
   * @brief
   *
   * @param channel
   * @return T*
   */
  auto FindObjectInChannel(int channel) -> ChannelObject*;

  /**
   * @brief Get the All Channel Id object
   *
   * @return QContainer<int>
   */
  auto GetAllChannelId() -> QContainer<int>;

  /**
   * @brief Set a new object in channel object
   *
   * @param channel
   * @param p_obj
   * @return T*
   */
  auto SetObjectInChannel(int channel, ChannelObjectPtr p_obj)
      -> ChannelObject*;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend