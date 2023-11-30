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

#include "SingletonStorage.h"

#include <shared_mutex>

#include "core/function/basic/ChannelObject.h"

namespace GpgFrontend {

class SingletonStorage::Impl {
 public:
  void ReleaseChannel(int channel) {
    decltype(instances_map_.end()) ins_it;
    {
      std::shared_lock<std::shared_mutex> lock(instances_mutex_);
      ins_it = instances_map_.find(channel);
    }
    if (ins_it != instances_map_.end()) instances_map_.erase(ins_it);
  }

  auto FindObjectInChannel(int channel) -> GpgFrontend::ChannelObject* {
    // read instances_map_
    decltype(instances_map_.end()) ins_it;
    {
      std::shared_lock<std::shared_mutex> lock(instances_mutex_);
      ins_it = instances_map_.find(channel);
      if (ins_it == instances_map_.end()) {
        return nullptr;
      }
      return ins_it->second.get();
    }
  }

  auto GetAllChannelId() -> std::vector<int> {
    std::vector<int> channels;
    channels.reserve(instances_map_.size());
    for (const auto& [key, value] : instances_map_) {
      channels.push_back(key);
    }
    return channels;
  }

  auto SetObjectInChannel(int channel, std::unique_ptr<ChannelObject> p_obj)
      -> GpgFrontend::ChannelObject* {
    {
      SPDLOG_TRACE("set channel: {} instance address: {}", channel,
                   static_cast<void*>(&instances_map_));

      assert(p_obj != nullptr);
      if (p_obj == nullptr) return nullptr;

      auto* raw_obj = p_obj.get();
      p_obj->SetChannel(channel);
      {
        std::unique_lock<std::shared_mutex> lock(instances_mutex_);
        instances_map_.insert({channel, std::move(p_obj)});
      }
      return raw_obj;
    }
  }

 private:
  std::shared_mutex instances_mutex_;  ///< mutex for _instances_map
  std::map<int, std::unique_ptr<ChannelObject>>
      instances_map_;  ///< map of singleton instances
};

SingletonStorage::SingletonStorage() noexcept : p_(std::make_unique<Impl>()) {}

SingletonStorage::~SingletonStorage() = default;

void SingletonStorage::ReleaseChannel(int channel) {
  p_->ReleaseChannel(channel);
}

auto SingletonStorage::FindObjectInChannel(int channel)
    -> GpgFrontend::ChannelObject* {
  return p_->FindObjectInChannel(channel);
}

auto SingletonStorage::GetAllChannelId() -> std::vector<int> {
  return p_->GetAllChannelId();
}

auto SingletonStorage::SetObjectInChannel(int channel,
                                          std::unique_ptr<ChannelObject> p_obj)
    -> GpgFrontend::ChannelObject* {
  return p_->SetObjectInChannel(channel, std::move(p_obj));
}

};  // namespace GpgFrontend