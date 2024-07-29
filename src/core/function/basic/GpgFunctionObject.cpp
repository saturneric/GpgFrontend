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

#include "GpgFunctionObject.h"

#include <map>
#include <mutex>
#include <typeinfo>

#include "core/function/SecureMemoryAllocator.h"
#include "core/function/basic/ChannelObject.h"

struct FunctionObjectTypeLockInfo {
  std::map<int, std::mutex> channel_lock_map;
  std::mutex type_lock;
};

std::mutex g_function_object_mutex_map_lock;
std::map<size_t, FunctionObjectTypeLockInfo> g_function_object_mutex_map;

namespace GpgFrontend {
auto GetGlobalFunctionObjectChannelLock(const std::type_info& type,
                                        int channel) -> std::mutex& {
  std::lock_guard<std::mutex> lock_guard(g_function_object_mutex_map_lock);
  auto& channel_map = g_function_object_mutex_map[type.hash_code()];
  return channel_map.channel_lock_map[channel];
}

auto GetGlobalFunctionObjectTypeLock(const std::type_info& type)
    -> std::mutex& {
  std::lock_guard<std::mutex> lock_guard(g_function_object_mutex_map_lock);
  auto& channel_map = g_function_object_mutex_map[type.hash_code()];
  return channel_map.type_lock;
}

/**
 * @brief Get the Instance object
 *
 * @param channel
 * @return T&
 */
auto GetChannelObjectInstance(const std::type_info& type,
                              int channel) -> ChannelObject* {
  // lock this channel
  std::lock_guard<std::mutex> guard(
      GetGlobalFunctionObjectChannelLock(type, channel));

  auto* p_storage =
      SingletonStorageCollection::GetInstance(false)->GetSingletonStorage(type);

  auto* p_pbj =
      static_cast<ChannelObject*>(p_storage->FindObjectInChannel(channel));

  return p_pbj;
}

auto CreateChannelObjectInstance(const std::type_info& type, int channel,
                                 SecureUniquePtr<ChannelObject> channel_object)
    -> ChannelObject* {
  // lock this channel
  std::lock_guard<std::mutex> guard(
      GetGlobalFunctionObjectChannelLock(type, channel));

  auto* p_storage =
      SingletonStorageCollection::GetInstance(false)->GetSingletonStorage(type);

  // do create object of this channel
  return p_storage->SetObjectInChannel(channel, std::move(channel_object));
}

}  // namespace GpgFrontend