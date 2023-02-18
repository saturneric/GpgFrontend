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

#ifndef GPGFRONTEND_ZH_CN_TS_FUNCTIONOBJECT_H
#define GPGFRONTEND_ZH_CN_TS_FUNCTIONOBJECT_H

#include <mutex>

#include "GpgConstants.h"

namespace GpgFrontend {

/**
 * @brief object which in channel system
 *
 */
class GPGFRONTEND_CORE_EXPORT ChannelObject {
 public:
  /**
   * @brief Construct a new Default Channel Object object
   *
   */
  ChannelObject() noexcept;

  /**
   * @brief Construct a new Channel Object object
   *
   * @param channel
   */
  ChannelObject(int channel);

  /**
   * @brief Get the Default Channel object
   *
   * @return int
   */
  static int GetDefaultChannel();

  /**
   * @brief Get the Channel object
   *
   * @return int
   */
  [[nodiscard]] int GetChannel() const;

  /**
   * @brief Set the Channel object
   *
   * @param channel
   */
  void SetChannel(int channel);

 private:
  int channel_ = _default_channel;            ///< The channel id
  static constexpr int _default_channel = 0;  ///< The default channel id
};

class GPGFRONTEND_CORE_EXPORT SingletonStorage {
 public:
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
  ChannelObject* FindObjectInChannel(int channel);

  /**
   * @brief Get all the channel ids
   *
   * @return std::vector<int>
   */
  std::vector<int> GetAllChannelId();

  /**
   * @brief Set a new object in channel object
   *
   * @param channel
   * @param p_obj
   * @return T*
   */
  ChannelObject* SetObjectInChannel(int channel,
                                    std::unique_ptr<ChannelObject> p_obj);

 private:
  std::shared_mutex instances_mutex_;  ///< mutex for _instances_map
  std::map<int, std::unique_ptr<ChannelObject>>
      instances_map_;  ///< map of singleton instances
};

class GPGFRONTEND_CORE_EXPORT SingletonStorageCollection {
 public:
  /**
   * @brief Get the Instance object
   *
   * @return SingletonStorageCollection*
   */
  static SingletonStorageCollection* GetInstance(bool force_refresh);

  /**
   * @brief Get the Singleton Storage object
   *
   * @param singleton_function_object
   * @return SingletonStorage*
   */
  SingletonStorage* GetSingletonStorage(const std::type_info&);

 private:
  std::shared_mutex storages_mutex_;  ///< mutex for storages_map_
  std::map<size_t, std::unique_ptr<SingletonStorage>> storages_map_;
};
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
  SingletonFunctionObject& operator=(const SingletonFunctionObject<T>&) =
      delete;

  /**
   * @brief Get the Instance object
   *
   * @param channel
   * @return T&
   */
  static T& GetInstance(
      int channel = GpgFrontend::GPGFRONTEND_DEFAULT_CHANNEL) {
    static std::mutex g_channel_mutex_map_lock;
    static std::map<int, std::mutex> g_channel_mutex_map;

    {
      std::lock_guard<std::mutex> guard(g_channel_mutex_map_lock);
      if (g_channel_mutex_map.find(channel) == g_channel_mutex_map.end()) {
        g_channel_mutex_map[channel];
      }
    }

    static_assert(std::is_base_of<SingletonFunctionObject<T>, T>::value,
                  "T not derived from SingletonFunctionObject<T>");

    auto* p_storage =
        SingletonStorageCollection::GetInstance(false)->GetSingletonStorage(
            typeid(T));
    auto* _p_pbj = (T*)(p_storage->FindObjectInChannel(channel));

    if (_p_pbj == nullptr) {
      // lock this channel
      std::lock_guard<std::mutex> guard(g_channel_mutex_map[channel]);

      // double check
      if ((_p_pbj = (T*)(p_storage->FindObjectInChannel(channel))) != nullptr)
        return *_p_pbj;

      // do create object of this channel
      auto new_obj = std::unique_ptr<ChannelObject>(new T(channel));
      return *(T*)(p_storage->SetObjectInChannel(channel, std::move(new_obj)));
    } else {
      return *_p_pbj;
    }
  }

  /**
   * @brief Create a Instance object
   *
   * @param channel
   * @param factory
   * @return T&
   */
  static T& CreateInstance(
      int channel,
      std::function<std::unique_ptr<ChannelObject>(void)> factory) {
    static_assert(std::is_base_of<SingletonFunctionObject<T>, T>::value,
                  "T not derived from SingletonFunctionObject<T>");

    auto p_storage =
        SingletonStorageCollection::GetInstance(false)->GetSingletonStorage(
            typeid(T));

    auto _p_pbj = (T*)(p_storage->FindObjectInChannel(channel));

    if (_p_pbj == nullptr) {
      return *(
          T*)(p_storage->SetObjectInChannel(channel, std::move(factory())));
    } else
      return *_p_pbj;
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
  static int GetDefaultChannel() { return ChannelObject::GetDefaultChannel(); }

  /**
   * @brief Get the Channel object
   *
   * @return int
   */
  [[nodiscard]] int GetChannel() const { return ChannelObject::GetChannel(); }

  /**
   * @brief Get all the channel ids
   *
   * @return std::vector<int>
   */
  static std::vector<int> GetAllChannelId() {
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
  explicit SingletonFunctionObject(int channel) : ChannelObject(channel) {}

  /**
   * @brief Destroy the Singleton Function Object object
   *
   */
  virtual ~SingletonFunctionObject() = default;
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_FUNCTIONOBJECT_H
