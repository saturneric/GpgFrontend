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

#include "core/GpgFunctionObject.h"

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>

void GpgFrontend::ChannelObject::SetChannel(int channel) {
  this->channel_ = channel;
}

int GpgFrontend::ChannelObject::GetChannel() const { return channel_; }

int GpgFrontend::ChannelObject::GetDefaultChannel() { return _default_channel; }

void GpgFrontend::SingletonStorage::ReleaseChannel(int channel) {
  decltype(instances_map_.end()) _it;
  {
    std::shared_lock<std::shared_mutex> lock(instances_mutex_);
    _it = instances_map_.find(channel);
  }
  if (_it != instances_map_.end()) instances_map_.erase(_it);
}

GpgFrontend::ChannelObject* GpgFrontend::SingletonStorage::FindObjectInChannel(
    int channel) {
  // read instances_map_
  decltype(instances_map_.end()) _it;
  {
    std::shared_lock<std::shared_mutex> lock(instances_mutex_);
    _it = instances_map_.find(channel);
    if (_it == instances_map_.end()) {
      return nullptr;
    } else {
      return _it->second.get();
    }
  }
}

std::vector<int> GpgFrontend::SingletonStorage::GetAllChannelId() {
  std::vector<int> _channels;
  for (const auto& [key, value] : instances_map_) {
    _channels.push_back(key);
  }
  return _channels;
}

GpgFrontend::ChannelObject* GpgFrontend::SingletonStorage::SetObjectInChannel(
    int channel, std::unique_ptr<ChannelObject> p_obj) {
  {
    SPDLOG_TRACE("set channel: {} instance address: {}", channel,
                 static_cast<void*>(&instances_map_));

    assert(p_obj != nullptr);
    if (p_obj == nullptr) return nullptr;

    auto raw_obj = p_obj.get();
    p_obj->SetChannel(channel);
    {
      std::unique_lock<std::shared_mutex> lock(instances_mutex_);
      instances_map_.insert({channel, std::move(p_obj)});
    }
    return raw_obj;
  }
}

GpgFrontend::SingletonStorage*
GpgFrontend::SingletonStorageCollection::GetSingletonStorage(
    const std::type_info& type_id) {
  const auto hash = type_id.hash_code();

  while (true) {
    decltype(storages_map_.end()) _it;
    {
      std::shared_lock<std::shared_mutex> lock(storages_mutex_);
      _it = storages_map_.find(hash);
    }
    if (_it == storages_map_.end()) {
      {
        std::unique_lock<std::shared_mutex> lock(storages_mutex_);
        storages_map_.insert({hash, std::make_unique<SingletonStorage>()});
      }
      SPDLOG_TRACE("hash: {} created, storage address: {} type_name: {}", hash,
                   static_cast<void*>(&storages_map_), type_id.name());
      continue;
    } else {
      return _it->second.get();
    }
  }
}

GpgFrontend::SingletonStorageCollection*
GpgFrontend::SingletonStorageCollection::GetInstance(
    bool force_refresh = false) {
  static SingletonStorageCollection* instance = nullptr;

  if (force_refresh || instance == nullptr) {
    instance = new SingletonStorageCollection();
    SPDLOG_DEBUG("new single storage collection created: {}",
                 static_cast<void*>(instance));
  }

  return instance;
}

GpgFrontend::ChannelObject::ChannelObject() noexcept = default;

GpgFrontend::ChannelObject::ChannelObject(int channel) : channel_(channel) {}
