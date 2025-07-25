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

#include "CacheManager.h"

#include <algorithm>
#include <shared_mutex>

#include "core/function/DataObjectOperator.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

template <typename Key, typename Value>
class ThreadSafeMap {
 public:
  using MapType = std::map<Key, Value>;
  using IteratorType = typename MapType::iterator;

  void insert(const Key& key, const Value& value) {
    std::unique_lock lock(mutex_);
    (*map_)[key] = value;
  }

  auto get(const Key& key) -> std::optional<Value> {
    std::shared_lock lock(mutex_);
    auto it = map_->find(key);
    if (it != map_->end()) {
      return it->second;
    }
    return std::nullopt;
  }

  auto exists(const Key& key) -> bool {
    std::shared_lock lock(mutex_);
    return map_->count(key) > 0;
  }

  auto begin() -> IteratorType { return map_mirror_->begin(); }

  auto end() -> IteratorType { return map_mirror_->end(); }

  auto mirror() -> ThreadSafeMap& {
    std::shared_lock lock(mutex_);
    *map_mirror_ = *map_;
    return *this;
  }

  auto remove(const QString& key) -> bool {
    std::unique_lock lock(mutex_);
    auto it = map_->find(key);
    if (it != map_->end()) {
      map_->erase(it);
      return true;
    }
    return false;
  }

 private:
  std::unique_ptr<MapType, SecureObjectDeleter<MapType>> map_mirror_ =
      std::move(SecureCreateUniqueObject<MapType>());
  std::unique_ptr<MapType, SecureObjectDeleter<MapType>> map_ =
      std::move(SecureCreateUniqueObject<MapType>());
  mutable std::shared_mutex mutex_;
};

class CacheManager::Impl : public QObject {
  Q_OBJECT
 public:
  explicit Impl(int channel)
      : channel_(channel), flush_timer_(new QTimer(this)) {
    connect(flush_timer_, &QTimer::timeout, this,
            &Impl::slot_flush_cache_storage);
    flush_timer_->start(15000);

    // load data from storage
    load_all_cache_storage();
  }

  void SaveDurableCache(const QString& key, const QJsonDocument& value,
                        bool flush) {
    SaveSecDurableCache(key, GFBuffer{value.toJson()}, flush);
  }

  void SaveSecDurableCache(const QString& key, const GFBuffer& value,
                           bool flush) {
    auto data_object_key = get_data_object_key(key);
    durable_cache_storage_.insert(key, value);

    if (!key_storage_.contains(key)) {
      key_storage_.push_back(key);
    }

    durable_cache_modified_ = true;
    if (flush) slot_flush_cache_storage();
  }

  auto LoadDurableCache(const QString& key) -> QJsonDocument {
    auto data_object_key = get_data_object_key(key);

    if (!durable_cache_storage_.exists(key)) {
      durable_cache_storage_.insert(key, load_cache_storage(key, {}));
      durable_cache_modified_ = true;
    }

    auto cache = LoadSecDurableCache(key);
    if (!cache.Empty()) {
      try {
        return QJsonDocument::fromJson(cache.ConvertToQByteArray());
      } catch (...) {
        LOG_W() << "failed to get cache object:" << key
                << " caught json exception.";
        return {};
      }
    }

    return {};
  }

  auto LoadDurableCache(const QString& key, QJsonDocument default_value)
      -> QJsonDocument {
    auto cache = LoadDurableCache(key);
    if (cache.isEmpty()) return default_value;
    return cache;
  }

  auto LoadSecDurableCache(const QString& key) -> GFBuffer {
    auto data_object_key = get_data_object_key(key);

    if (!durable_cache_storage_.exists(key)) {
      durable_cache_storage_.insert(key, load_cache_storage(key, {}));
      durable_cache_modified_ = true;
    }

    auto cache = durable_cache_storage_.get(key);
    if (cache) return *cache;

    return {};
  }

  auto LoadSecDurableCache(const QString& key, const GFBuffer& default_value)
      -> GFBuffer {
    auto cache = LoadSecDurableCache(key);
    if (cache.Empty()) return default_value;
    return cache;
  }

  auto ResetDurableCache(const QString& key) -> bool {
    auto data_object_key = get_data_object_key(key);
    durable_cache_modified_ = true;
    return durable_cache_storage_.remove(key);
  }

  void FlushCacheStorage() { this->slot_flush_cache_storage(); }

  void SaveCache(const QString& key, const QString& value, qint64 ttl) {
    SaveSecCache(key, GFBuffer{value}, ttl);
  }

  void SaveSecCache(const QString& key, const GFBuffer& value, qint64 ttl) {
    LOG_D() << "save cache, key: " << key << "ttl: " << ttl;
    runtime_cache_storage_.insert(
        key,
        new CacheObject(
            value, ttl < 0 ? -1 : QDateTime::currentSecsSinceEpoch() + ttl));
  }

  auto LoadCache(const QString& key) -> QString {
    return LoadSecCache(key).ConvertToQString();
  }

  auto LoadSecCache(const QString& key) -> GFBuffer {
    if (!runtime_cache_storage_.contains(key)) return {};
    LOG_D() << "hit cache, key: " << key;

    auto* value = runtime_cache_storage_.object(key);
    if (value == nullptr) {
      LOG_E() << "hit cache but got nullptr by value, key" << key;
      return {};
    }

    if (value->ttl < 0) return value->value;

    // deal with expiration
    auto current_timestamp = QDateTime::currentSecsSinceEpoch();
    if (current_timestamp > value->ttl) {
      LOG_D() << "hit cache but expired, key: " << key
              << "expiration timestamp:" << value->ttl;
      ResetCache(key);
      return {};
    }

    return value->value;
  }

  void ResetCache(const QString& key) { runtime_cache_storage_.remove(key); }

 private slots:

  /**
   * @brief
   *
   */
  void slot_flush_cache_storage() {
    if (!durable_cache_modified_) return;

    FLOG_D() << "flushing durable cache to disk...";

    for (const auto& cache : durable_cache_storage_.mirror()) {
      if (cache.second.Empty()) continue;

      auto key = get_data_object_key(cache.first);
      opera_.StoreSecDataObj(key, cache.second);
    }

    opera_.StoreDataObj(drk_key_, QJsonDocument(key_storage_));
    durable_cache_modified_ = false;
  }

 private:
  /**
   * @brief Get the data object key object
   *
   * @param key
   * @return QString
   */
  static auto get_data_object_key(const QString& key) -> QString {
    return QString("__cache_data_%1").arg(key);
  }

  /**
   * @brief
   *
   * @param key
   * @param default_value
   * @return QJsonObject
   */
  auto load_cache_storage(const QString& key, const GFBuffer& default_value)
      -> GFBuffer {
    auto data_object_key = get_data_object_key(key);
    auto stored_data = opera_.GetSecDataObject(data_object_key);

    if (stored_data) return *stored_data;
    return default_value;
  }

  /**
   * @brief
   *
   */
  void load_all_cache_storage() {
    FLOG_D("start to load all cache from file system");
    auto stored_data = opera_.GetDataObject(drk_key_);

    // get cache data list from file system
    QJsonArray registered_key_list;
    if (stored_data.has_value() && stored_data->isArray()) {
      registered_key_list = stored_data->array();
    } else {
      opera_.StoreDataObj(drk_key_, QJsonDocument(QJsonArray()));
    }

    for (const auto& key : registered_key_list) {
      load_cache_storage(key.toString(), {});
    }

    key_storage_ = registered_key_list;
  }

  /**
   * @brief
   *
   * @param key
   */
  void register_cache_key(const QString& key) {}

  struct CacheObject {
    GFBuffer value;
    qint64 ttl;

    CacheObject(const GFBuffer& value, qint64 ttl) : value(value), ttl(ttl) {}
  };

  int channel_;
  GpgFrontend::DataObjectOperator& opera_ =
      GpgFrontend::DataObjectOperator::GetInstance(channel_);

  QCache<QString, CacheObject> runtime_cache_storage_;
  ThreadSafeMap<QString, GFBuffer> durable_cache_storage_;
  QJsonArray key_storage_;
  QTimer* flush_timer_;
  const QString drk_key_ = "__cache_manage_data_register_key_list";
  std::atomic<bool> durable_cache_modified_{};
};

CacheManager::CacheManager(int channel)
    : SingletonFunctionObject<CacheManager>(channel),
      p_(SecureCreateUniqueObject<Impl>(GetChannel())) {}

CacheManager::~CacheManager() = default;

void CacheManager::SaveDurableCache(const QString& key,
                                    const QJsonDocument& value, bool flush) {
  p_->SaveDurableCache(key, value, flush);
}

auto CacheManager::LoadDurableCache(const QString& key) -> QJsonDocument {
  return p_->LoadDurableCache(key);
}

auto CacheManager::LoadDurableCache(const QString& key,
                                    QJsonDocument default_value)
    -> QJsonDocument {
  return p_->LoadDurableCache(key, std::move(default_value));
}

auto CacheManager::ResetDurableCache(const QString& key) -> bool {
  return p_->ResetDurableCache(key);
}

void CacheManager::SaveCache(const QString& key, QString value, qint64 ttl) {
  p_->SaveCache(key, std::move(value), ttl);
}

auto CacheManager::LoadCache(const QString& key) -> QString {
  return p_->LoadCache(key);
}

void CacheManager::ResetCache(const QString& key) { p_->ResetCache(key); }

auto CacheManager::LoadSecDurableCache(const QString& key) -> GFBuffer {
  return p_->LoadSecDurableCache(key);
}

auto CacheManager::LoadSecDurableCache(const QString& key,
                                       const GFBuffer& default_value)
    -> GFBuffer {
  return p_->LoadSecDurableCache(key, default_value);
}

void CacheManager::SaveSecDurableCache(const QString& key,
                                       const GFBuffer& value, bool flush) {
  p_->SaveSecDurableCache(key, value, flush);
}

void CacheManager::SaveSecCache(const QString& key, const GFBuffer& value,
                                qint64 ttl) {
  p_->SaveSecCache(key, value, ttl);
}

auto CacheManager::LoadSecCache(const QString& key) -> GFBuffer {
  return p_->LoadSecCache(key);
}

void CacheManager::FlushCacheStorage() { p_->FlushCacheStorage(); }
}  // namespace GpgFrontend

#include "CacheManager.moc"