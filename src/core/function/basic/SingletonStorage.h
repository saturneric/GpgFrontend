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

#include "core/typedef/CoreTypedef.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

class ChannelObject;

using ChannelObjectPtr = SecureUniquePtr<ChannelObject>;

/**
 * @brief Thread-safe per-type container that maps channel IDs to singleton
 * instances.
 *
 * Each type registered in the channel system has exactly one SingletonStorage.
 * Internally it holds a shared-mutex-protected map from channel ID to
 * ChannelObjectPtr. Typically managed via SingletonStorageCollection.
 */
class GF_CORE_EXPORT SingletonStorage {
 public:
  /**
   * @brief Construct an empty storage.
   */
  SingletonStorage() noexcept;

  /**
   * @brief Destroy the storage and all owned singleton instances.
   */
  ~SingletonStorage();

  /**
   * @brief Destroy and remove the singleton instance for the given channel.
   *
   * @param channel channel ID whose instance should be released
   */
  void ReleaseChannel(int channel);

  /**
   * @brief Return the singleton instance for the given channel, or nullptr if
   * none exists.
   *
   * @param channel channel ID to look up
   * @return pointer to the ChannelObject, or nullptr if not found
   */
  auto FindObjectInChannel(int channel) -> ChannelObject*;

  /**
   * @brief Return all channel IDs that currently have a live instance.
   *
   * @return list of active channel IDs
   */
  auto GetAllChannelId() -> QContainer<int>;

  /**
   * @brief Store a new singleton instance for the given channel.
   *
   * Takes ownership of @p p_obj, sets its channel ID, and returns a raw
   * pointer to the stored object. Any previous instance for that channel is
   * replaced.
   *
   * @param channel channel ID to assign
   * @param p_obj owning pointer to the new instance
   * @return raw pointer to the stored instance, or nullptr if @p p_obj is null
   */
  auto SetObjectInChannel(int channel, ChannelObjectPtr p_obj)
      -> ChannelObject*;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend
