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

#include "core/function/basic/ChannelObject.h"
#include "core/function/basic/SingletonStorage.h"
#include "core/function/basic/SingletonStorageCollection.h"
#include "core/utils/MemoryUtils.h"
#include "spdlog/spdlog.h"

namespace GpgFrontend {

/**
 * @brief
 *
 * @tparam T
 */
template <typename T>
class SingletonFunctionObject : public ChannelObject {
 public:
  /**
   * @brief prohibit copy
   *
   */
  SingletonFunctionObject(const SingletonFunctionObject<T>&) = delete;

  /**
   * @brief prohibit copy
   *
   * @return SingletonFunctionObject&
   */
  auto operator=(const SingletonFunctionObject<T>&)
      -> SingletonFunctionObject& = delete;

  /**
   * @brief Get the Instance object
   *
   * @param channel
   * @return T&
   */
  static auto GetInstance(int channel = GpgFrontend::kGpgFrontendDefaultChannel)
      -> T& {
    static std::mutex g_channel_mutex_map_lock;
    static std::map<int, std::mutex> g_channel_mutex_map;

    SPDLOG_TRACE("try to get instance of type: {} at channel: {}",
                 typeid(T).name(), channel);

    {
      std::lock_guard<std::mutex> guard(g_channel_mutex_map_lock);
      if (g_channel_mutex_map.find(channel) == g_channel_mutex_map.end()) {
        g_channel_mutex_map[channel];
      }
    }

    static_assert(std::is_base_of_v<SingletonFunctionObject<T>, T>,
                  "T not derived from SingletonFunctionObject<T>");

    auto* p_storage =
        SingletonStorageCollection::GetInstance(false)->GetSingletonStorage(
            typeid(T));
    SPDLOG_TRACE("get singleton storage result, p_storage: {}",
                 static_cast<void*>(p_storage));

    auto* p_pbj = static_cast<T*>(p_storage->FindObjectInChannel(channel));
    SPDLOG_TRACE("find channel object result, channel {}, p_pbj: {}", channel,
                 static_cast<void*>(p_pbj));

    if (p_pbj == nullptr) {
      SPDLOG_TRACE("create channel object, channel {}, type: {}", channel,
                   typeid(T).name());

      // lock this channel
      std::lock_guard<std::mutex> guard(g_channel_mutex_map[channel]);

      // double check
      if (p_pbj = static_cast<T*>(p_storage->FindObjectInChannel(channel));
          p_pbj != nullptr) {
        return *p_pbj;
      }

      // do create object of this channel
      auto new_obj =
          ConvertToChannelObjectPtr<>(SecureCreateUniqueObject<T>(channel));
      return *static_cast<T*>(
          p_storage->SetObjectInChannel(channel, std::move(new_obj)));
    }
    return *p_pbj;
  }

  /**
   * @brief Create a Instance object
   *
   * @param channel
   * @param factory
   * @return T&
   */
  static auto CreateInstance(
      int channel, const std::function<ChannelObjectPtr(void)>& factory) -> T& {
    static_assert(std::is_base_of_v<SingletonFunctionObject<T>, T>,
                  "T not derived from SingletonFunctionObject<T>");

    auto* p_storage =
        SingletonStorageCollection::GetInstance(false)->GetSingletonStorage(
            typeid(T));
    SPDLOG_TRACE("get singleton storage result, p_storage: {}",
                 static_cast<void*>(p_storage));

    auto p_pbj = static_cast<T*>(p_storage->FindObjectInChannel(channel));
    SPDLOG_TRACE("find channel object result, channel {}, p_pbj: {}", channel,
                 static_cast<void*>(p_pbj));

    if (p_pbj == nullptr) {
      SPDLOG_TRACE("create channel object, channel {}, type: {}", channel,
                   typeid(T).name());
      return *static_cast<T*>(
          p_storage->SetObjectInChannel(channel, factory()));
    }
    return *p_pbj;
  }

  /**
   * @brief
   *
   * @param channel
   * @return T&
   */
  static void ReleaseChannel(int channel) {
    SingletonStorageCollection::GetInstance(false)
        ->GetSingletonStorage(typeid(T))
        ->ReleaseChannel(channel);
  }

  /**
   * @brief Get the Default Channel object
   *
   * @return int
   */
  static auto GetDefaultChannel() -> int {
    return ChannelObject::GetDefaultChannel();
  }

  /**
   * @brief Get the Channel object
   *
   * @return int
   */
  [[nodiscard]] auto GetChannel() const -> int {
    return ChannelObject::GetChannel();
  }

  /**
   * @brief Get all the channel ids
   *
   * @return std::vector<int>
   */
  static auto GetAllChannelId() -> std::vector<int> {
    return SingletonStorageCollection::GetInstance(false)
        ->GetSingletonStorage(typeid(T))
        ->GetAllChannelId();
  }

  /**
   * @brief Construct a new Singleton Function Object object
   *
   */
  SingletonFunctionObject(T&&) = delete;

  /**
   * @brief Construct a new Singleton Function Object object
   *
   */
  SingletonFunctionObject(const T&) = delete;

  /**
   * @brief
   *
   */
  void operator=(const T&) = delete;

 protected:
  /**
   * @brief Construct a new Singleton Function Object object
   *
   */
  SingletonFunctionObject() = default;

  /**
   * @brief Construct a new Singleton Function Object object
   *
   * @param channel
   */
  explicit SingletonFunctionObject(int channel)
      : ChannelObject(channel, typeid(T).name()) {}

  /**
   * @brief Destroy the Singleton Function Object object
   *
   */
  virtual ~SingletonFunctionObject() = default;
};
}  // namespace GpgFrontend
