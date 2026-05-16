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

#include <mutex>

#include "core/function/basic/ChannelObject.h"
#include "core/function/basic/SingletonStorage.h"
#include "core/function/basic/SingletonStorageCollection.h"
#include "core/typedef/CoreTypedef.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

/**
 * @brief Look up an existing singleton instance for the given type and channel.
 *
 * Acquires the per-channel lock before querying SingletonStorageCollection.
 *
 * @param type type_info of the singleton type
 * @param channel channel ID to look up
 * @return pointer to the existing ChannelObject, or nullptr if not found
 */
auto GF_CORE_EXPORT GetChannelObjectInstance(const std::type_info& type,
                                             int channel) -> ChannelObject*;

/**
 * @brief Store a new singleton instance for the given type and channel.
 *
 * Acquires the per-channel lock before inserting into
 * SingletonStorageCollection.
 *
 * @param type type_info of the singleton type
 * @param channel channel ID to assign
 * @param channel_object owning pointer to the new instance
 * @return raw pointer to the stored instance
 */
auto GF_CORE_EXPORT CreateChannelObjectInstance(
    const std::type_info& type, int channel,
    SecureUniquePtr<ChannelObject> channel_object) -> ChannelObject*;

/**
 * @brief Return the per-type mutex used to guard GetInstance create-or-get.
 *
 * Each type has its own mutex so that concurrent GetInstance() calls for
 * different types do not block each other.
 *
 * @param type type_info of the singleton type
 * @return reference to the per-type mutex
 */
auto GF_CORE_EXPORT GetGlobalFunctionObjectTypeLock(const std::type_info& type)
    -> std::mutex&;

/**
 * @brief CRTP base that provides per-channel singleton lifecycle management.
 *
 * Derive from SingletonFunctionObject<T> to gain GetInstance(),
 * CreateInstance(), ReleaseChannel(), and related helpers. Instances are
 * created on first access, stored in SingletonStorage, and retrieved by channel
 * ID. Copy and move are deleted; construction is protected.
 *
 * @tparam T the concrete derived type (CRTP)
 */
template <typename T>
class SingletonFunctionObject : public ChannelObject {
 public:
  SingletonFunctionObject(const SingletonFunctionObject<T>&) = delete;
  auto operator=(const SingletonFunctionObject<T>&)
      -> SingletonFunctionObject& = delete;
  SingletonFunctionObject(T&&) = delete;
  SingletonFunctionObject(const T&) = delete;
  void operator=(const T&) = delete;

  /**
   * @brief Return the T singleton for the given channel, creating it if needed.
   *
   * Thread-safe: acquires the per-type lock to check for an existing instance,
   * then the per-channel lock to create one if absent.
   *
   * @param channel channel ID to retrieve (defaults to
   * kGpgFrontendDefaultChannel)
   * @return reference to the T singleton on that channel
   */
  static auto GetInstance(int channel = GpgFrontend::kGpgFrontendDefaultChannel)
      -> T& {
    static_assert(std::is_base_of_v<SingletonFunctionObject<T>, T>,
                  "T not derived from SingletonFunctionObject<T>");

    const auto& type = typeid(T);
    std::lock_guard<std::mutex> guard(GetGlobalFunctionObjectTypeLock(type));
    auto* channel_object = GetChannelObjectInstance(type, channel);
    if (channel_object == nullptr) {
      channel_object = CreateChannelObjectInstance(
          type, channel,
          ConvertToChannelObjectPtr(SecureCreateUniqueObject<T>(channel)));
    }
    return *static_cast<T*>(channel_object);
  }

  /**
   * @brief Create or replace the T singleton for the given channel using a
   * custom factory.
   *
   * The factory must return a ChannelObjectPtr owning a T instance.
   *
   * @param channel channel ID to assign
   * @param factory callable returning a ChannelObjectPtr
   * @return reference to the newly stored T instance
   */
  static auto CreateInstance(
      int channel, const std::function<ChannelObjectPtr(void)>& factory) -> T& {
    static_assert(std::is_base_of_v<SingletonFunctionObject<T>, T>,
                  "T not derived from SingletonFunctionObject<T>");

    const auto& type = typeid(T);
    std::lock_guard<std::mutex> guard(GetGlobalFunctionObjectTypeLock(type));
    return *static_cast<T*>(
        CreateChannelObjectInstance(type, channel, factory()));
  }

  /**
   * @brief Destroy and remove the T singleton for the given channel.
   *
   * @param channel channel ID whose instance should be released
   */
  static void ReleaseChannel(int channel) {
    SingletonStorageCollection::GetInstance()
        ->GetSingletonStorage(typeid(T))
        ->ReleaseChannel(channel);
  }

  /**
   * @brief Return the default channel identifier (kGpgFrontendDefaultChannel).
   *
   * @return default channel ID
   */
  static auto GetDefaultChannel() -> int {
    return ChannelObject::GetDefaultChannel();
  }

  /**
   * @brief Return the channel ID this instance is associated with.
   *
   * @return channel ID
   */
  [[nodiscard]] auto GetChannel() const -> int {
    return ChannelObject::GetChannel();
  }

  /**
   * @brief Return all channel IDs that currently have a live T instance.
   *
   * @return list of active channel IDs for type T
   */
  static auto GetAllChannelId() -> QContainer<int> {
    return SingletonStorageCollection::GetInstance()
        ->GetSingletonStorage(typeid(T))
        ->GetAllChannelId();
  }

 protected:
  /**
   * @brief Construct on the default channel.
   */
  SingletonFunctionObject() = default;

  /**
   * @brief Construct on the given channel.
   *
   * @param channel channel ID for this instance
   */
  explicit SingletonFunctionObject(int channel)
      : ChannelObject(channel, typeid(T).name()) {}

  /**
   * @brief Virtual destructor to allow correct cleanup of derived types.
   */
  virtual ~SingletonFunctionObject() = default;
};
}  // namespace GpgFrontend
