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

#include <nlohmann/json.hpp>
#include <optional>
#include <shared_mutex>

#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

template <typename Key, typename Value>
class ThreadSafeMap {
 public:
  using MapType = std::map<Key, Value>;
  using IteratorType = typename MapType::iterator;

  void insert(const Key& key, const Value& value) {
    std::unique_lock lock(mutex_);
    map_[key] = value;
  }

  std::optional<Value> get(const Key& key) {
    std::shared_lock lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  bool exists(const Key& key) {
    std::shared_lock lock(mutex_);
    return map_.count(key) > 0;
  }

  IteratorType begin() { return map_mirror_.begin(); }

  IteratorType end() { return map_mirror_.end(); }

  ThreadSafeMap& mirror() {
    std::shared_lock lock(mutex_);
    map_mirror_ = map_;
    return *this;
  }

 private:
  MapType map_mirror_;
  MapType map_;
  mutable std::shared_mutex mutex_;
};

class GPGFRONTEND_CORE_EXPORT CacheManager
    : public QObject,
      public SingletonFunctionObject<CacheManager> {
  Q_OBJECT
 public:
  CacheManager(int channel = SingletonFunctionObject::GetDefaultChannel());

  void SaveCache(std::string key, const nlohmann::json& value,
                 bool flush = false);

  nlohmann::json LoadCache(std::string key);

  nlohmann::json LoadCache(std::string key, nlohmann::json default_value);

 private:
  std::string get_data_object_key(std::string key);

  nlohmann::json load_cache_storage(std::string key,
                                    nlohmann::json default_value);

  void load_all_cache_storage();

  void flush_cache_storage();

  void register_cache_key(std::string key);

  ThreadSafeMap<std::string, nlohmann::json> cache_storage_;
  nlohmann::json key_storage_;
  QTimer* m_timer_;
  const std::string drk_key_ = "__cache_manage_data_register_key_list";
};

}  // namespace GpgFrontend
