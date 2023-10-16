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
#include <string>

#include "function/DataObjectOperator.h"
#include "spdlog/spdlog.h"

GpgFrontend::CacheManager::CacheManager(int channel)
    : m_timer_(new QTimer(this)),
      SingletonFunctionObject<CacheManager>(channel) {
  connect(m_timer_, &QTimer::timeout, this, &CacheManager::flush_cache_storage);
  m_timer_->start(15000);

  load_all_cache_storage();
}

void GpgFrontend::CacheManager::SaveCache(std::string key,
                                          const nlohmann::json& value,
                                          bool flush) {
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

nlohmann::json GpgFrontend::CacheManager::LoadCache(std::string key) {
  auto data_object_key = get_data_object_key(key);

  if (!cache_storage_.exists(key)) {
    cache_storage_.insert(key, load_cache_storage(key, {}));
  }

  auto cache = cache_storage_.get(key);
  if (cache)
    return *cache;
  else
    return {};
}

nlohmann::json GpgFrontend::CacheManager::LoadCache(
    std::string key, nlohmann::json default_value) {
  auto data_object_key = get_data_object_key(key);
  if (!cache_storage_.exists(key)) {
    cache_storage_.insert(key, load_cache_storage(key, default_value));
  }

  auto cache = cache_storage_.get(key);
  if (cache)
    return *cache;
  else
    return {};
}

std::string GpgFrontend::CacheManager::get_data_object_key(std::string key) {
  return (boost::format("__cache_data_%1%") % key).str();
}

nlohmann::json GpgFrontend::CacheManager::load_cache_storage(
    std::string key, nlohmann::json default_value) {
  auto data_object_key = get_data_object_key(key);
  auto stored_data =
      GpgFrontend::DataObjectOperator::GetInstance().GetDataObject(
          data_object_key);

  if (stored_data.has_value()) {
    return stored_data.value();
  } else {
    return default_value;
  }
}

void GpgFrontend::CacheManager::flush_cache_storage() {
  for (auto cache : cache_storage_.mirror()) {
    auto key = get_data_object_key(cache.first);
    SPDLOG_DEBUG("save cache into filesystem, key {}, value size: {}", key,
                 cache.second.size());
    GpgFrontend::DataObjectOperator::GetInstance().SaveDataObj(key,
                                                               cache.second);
  }
  GpgFrontend::DataObjectOperator::GetInstance().SaveDataObj(drk_key_,
                                                             key_storage_);
}

void GpgFrontend::CacheManager::register_cache_key(std::string key) {}

void GpgFrontend::CacheManager::load_all_cache_storage() {
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

  for (auto key : registered_key_list) {
    load_cache_storage(key, {});
  }

  key_storage_ = registered_key_list;
}