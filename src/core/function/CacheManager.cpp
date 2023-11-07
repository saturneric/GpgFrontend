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

#include "CacheManager.h"

#include <algorithm>
#include <boost/format.hpp>
#include <shared_mutex>
#include <utility>

#include "core/function/DataObjectOperator.h"

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

  auto get(const Key& key) -> std::optional<Value> {
    std::shared_lock lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  auto exists(const Key& key) -> bool {
    std::shared_lock lock(mutex_);
    return map_.count(key) > 0;
  }

  auto begin() -> IteratorType { return map_mirror_.begin(); }

  auto end() -> IteratorType { return map_mirror_.end(); }

  auto mirror() -> ThreadSafeMap& {
    std::shared_lock lock(mutex_);
    map_mirror_ = map_;
    return *this;
  }

  auto remove(std::string key) -> bool {
    std::unique_lock lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end()) {
      map_.erase(it);
      return true;
    }
    return false;
  }

 private:
  MapType map_mirror_;
  MapType map_;
  mutable std::shared_mutex mutex_;
};

class CacheManager::Impl : public SingletonFunctionObject<CacheManager::Impl> {
 public:
  Impl(CacheManager* parent, int channel)
      : SingletonFunctionObject<CacheManager::Impl>(channel),
        parent_(parent),
        m_timer_(new QTimer(parent)) {
    connect(m_timer_, &QTimer::timeout, parent_,
            [this]() { flush_cache_storage(); });
    m_timer_->start(15000);

    load_all_cache_storage();
  }

  void SaveCache(std::string key, const nlohmann::json& value, bool flush) {
    auto data_object_key = get_data_object_key(key);
    cache_storage_.insert(key, value);

    if (std::find(key_storage_.begin(), key_storage_.end(), key) ==
        key_storage_.end()) {
      SPDLOG_DEBUG("register new key of cache", key);
      key_storage_.push_back(key);
    }

    if (flush) {
      flush_cache_storage();
    }
  }

  auto LoadCache(const std::string& key) -> nlohmann::json {
    auto data_object_key = get_data_object_key(key);

    if (!cache_storage_.exists(key)) {
      cache_storage_.insert(key, load_cache_storage(key, {}));
    }

    auto cache = cache_storage_.get(key);
    if (cache) {
      return *cache;
    }
    return {};
  }

  auto LoadCache(const std::string& key, nlohmann::json default_value)
      -> nlohmann::json {
    auto data_object_key = get_data_object_key(key);
    if (!cache_storage_.exists(key)) {
      cache_storage_.insert(key,
                            load_cache_storage(key, std::move(default_value)));
    }

    auto cache = cache_storage_.get(key);
    if (cache) {
      return *cache;
    }
    return {};
  }

  auto ResetCache(const std::string& key) -> bool {
    auto data_object_key = get_data_object_key(key);
    return cache_storage_.remove(key);
  }

 private:
  CacheManager* parent_;
  ThreadSafeMap<std::string, nlohmann::json> cache_storage_;
  nlohmann::json key_storage_;
  QTimer* m_timer_;
  const std::string drk_key_ = "__cache_manage_data_register_key_list";

  /**
   * @brief Get the data object key object
   *
   * @param key
   * @return std::string
   */
  static auto get_data_object_key(std::string key) -> std::string {
    return (boost::format("__cache_data_%1%") % key).str();
  }

  /**
   * @brief
   *
   * @param key
   * @param default_value
   * @return nlohmann::json
   */
  static auto load_cache_storage(std::string key, nlohmann::json default_value)
      -> nlohmann::json {
    auto data_object_key = get_data_object_key(std::move(key));
    auto stored_data =
        GpgFrontend::DataObjectOperator::GetInstance().GetDataObject(
            data_object_key);

    if (stored_data.has_value()) {
      return stored_data.value();
    }
    return default_value;
  }

  /**
   * @brief
   *
   */
  void load_all_cache_storage() {
    SPDLOG_DEBUG("start to load all cache from file system");
    auto stored_data =
        GpgFrontend::DataObjectOperator::GetInstance().GetDataObject(drk_key_);

    // get cache data list from file system
    nlohmann::json registered_key_list;
    if (stored_data.has_value()) {
      registered_key_list = std::move(stored_data.value());
    }

    if (!registered_key_list.is_array()) {
      GpgFrontend::DataObjectOperator::GetInstance().SaveDataObj(
          drk_key_, nlohmann::json::array());
      SPDLOG_ERROR("drk_key_ is not an array, abort.");
      return;
    }

    for (const auto& key : registered_key_list) {
      load_cache_storage(key, {});
    }

    key_storage_ = registered_key_list;
  }

  /**
   * @brief
   *
   */
  void flush_cache_storage() {
    for (const auto& cache : cache_storage_.mirror()) {
      auto key = get_data_object_key(cache.first);
      SPDLOG_DEBUG("save cache into filesystem, key {}, value size: {}", key,
                   cache.second.size());
      GpgFrontend::DataObjectOperator::GetInstance().SaveDataObj(key,
                                                                 cache.second);
    }
    GpgFrontend::DataObjectOperator::GetInstance().SaveDataObj(drk_key_,
                                                               key_storage_);
  }

  /**
   * @brief
   *
   * @param key
   */
  void register_cache_key(const std::string& key) {}
};

CacheManager::CacheManager(int channel)
    : SingletonFunctionObject<CacheManager>(channel),
      p_(std::make_unique<Impl>(this, channel)) {}

CacheManager::~CacheManager() = default;

void CacheManager::SaveCache(std::string key, const nlohmann::json& value,
                             bool flush) {
  p_->SaveCache(std::move(key), value, flush);
}

auto CacheManager::LoadCache(std::string key) -> nlohmann::json {
  return p_->LoadCache(key);
}

auto CacheManager::LoadCache(std::string key, nlohmann::json default_value)
    -> nlohmann::json {
  return p_->LoadCache(key, std::move(default_value));
}

auto CacheManager::ResetCache(std::string key) -> bool {
  return p_->ResetCache(key);
}

}  // namespace GpgFrontend
