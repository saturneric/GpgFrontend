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

#include "GpgKeyRepository.h"

#include <gpg-error.h>

#include <mutex>

#include "core/function/GFKeyDatabase.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/KeyStorageTraits.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

class GpgKeyRepository::Impl
    : public SingletonFunctionObject<GpgKeyRepository::Impl> {
 public:
  explicit Impl(int channel)
      : SingletonFunctionObject<GpgKeyRepository::Impl>(channel),
        keys_search_cache_(
            QSharedPointer<QMap<QString, GpgAbstractKeyPtr>>::create()),
        keys_cache_(QSharedPointer<GpgKeyPtrList>::create()) {}

  auto GetKeyPtr(const QString& key_id, bool cache) -> GpgKeyPtr {
    // find in cache first
    if (cache) {
      auto key = get_key_in_cache(key_id);
      if (key != nullptr) return qSharedPointerDynamicCast<GpgKey>(key);

      LOG_W() << "get gpg key" << key_id
              << "from cache failed, channel: " << GetChannel();
    }

    auto key_ptr = RunRegisteredForward<GetKeyPtrOpTag>(ctx_, key_id, true);
    if (key_ptr == nullptr) {
      key_ptr = GetPubkeyPtr(key_id, cache);
    }
    return key_ptr;
  }

  auto GetKey(const QString& key_id, bool cache) -> GpgKey {
    auto key = GetKeyPtr(key_id, cache);

    if (key != nullptr) return *key;
    return {};
  }

  auto GetPubkey(const QString& key_id, bool cache) -> GpgKey {
    auto key = GetKeyPtr(key_id, cache);

    if (key != nullptr) return *key;
    return {};
  }

  auto GetPubkeyPtr(const QString& key_id, bool cache) -> GpgKeyPtr {
    // find in cache first
    if (cache) {
      auto key = get_key_in_cache(key_id);
      if (key != nullptr) return qSharedPointerDynamicCast<GpgKey>(key);

      LOG_W() << "get public gpg key" << key_id
              << "from cache failed, channel: " << GetChannel();
    }

    return RunRegisteredForward<GetKeyPtrOpTag>(ctx_, key_id, false);
  }

  auto FetchKey() -> GpgKeyPtrList {
    if (keys_cache_->empty() || keys_search_cache_->empty()) {
      FlushKeyCache();
    }

    auto keys_list = GpgKeyPtrList{};
    {
      // get the lock
      std::lock_guard<std::mutex> lock(keys_cache_mutex_);
      for (const auto& key : *keys_cache_) {
        keys_list.push_back(key);
      }
    }
    return keys_list;
  }

  auto FetchGpgKeyList() -> GpgAbstractKeyPtrList {
    if (keys_search_cache_->empty()) {
      FlushKeyCache();
    }

    auto keys_list = GpgAbstractKeyPtrList{};
    {
      // get the lock
      std::lock_guard<std::mutex> lock(keys_cache_mutex_);
      for (const auto& key : *keys_cache_) {
        keys_list.push_back(key);
      }
    }

    return keys_list;
  }

  auto FlushKeyCache() -> bool {
    // clear the keys cache
    keys_cache_->clear();
    keys_search_cache_->clear();

    if (first_flush_) {
      LOG_D() << "flush key cache for the first time, channel: "
              << GetChannel();
      RunRegisteredForward<FlushKeyDatabaseOpTag>(ctx_);
      first_flush_ = false;
    }

    return RunRegisteredForward<FlushKeyCacheOpTag>(ctx_, keys_cache_,
                                                    keys_search_cache_);
  }

  auto GetKeys(const KeyIdArgsList& ids) -> GpgKeyList {
    auto keys = GpgKeyList{};
    for (const auto& key_id : ids) keys.push_back(GetKey(key_id, true));
    return keys;
  }

  auto Fetch() -> QContainer<QSharedPointer<GpgKey>> {
    auto keys = FetchKey();

    auto ret = QContainer<QSharedPointer<GpgKey>>();
    for (const auto& key : keys) {
      ret.append(key);
    }
    return ret;
  }

  auto GetKeyORSubkeyPtr(const QString& key_id) -> GpgAbstractKeyPtr {
    return RunRegisteredForward<GetKeyPtrOpTag>(ctx_, key_id, true);
  }

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());
  bool first_flush_ = true;
  mutable std::mutex ctx_mutex_;
  QSharedPointer<QMap<QString, GpgAbstractKeyPtr>> keys_search_cache_;
  QSharedPointer<GpgKeyPtrList> keys_cache_;
  mutable std::mutex keys_cache_mutex_;

  /**
   * @brief Get the Key object
   *
   * @param id
   * @return GpgKey
   */
  auto get_key_in_cache(const QString& key_id) -> GpgAbstractKeyPtr {
    std::lock_guard<std::mutex> lock(keys_cache_mutex_);

    if (keys_search_cache_->contains(key_id)) {
      std::lock_guard<std::mutex> lock(ctx_mutex_);
      // return a copy of the key in cache
      return keys_search_cache_->value(key_id);
    }

    // return a bad key
    return {};
  }
};

GpgKeyRepository::GpgKeyRepository(int channel)
    : SingletonFunctionObject<GpgKeyRepository>(channel),
      p_(SecureCreateUniqueObject<Impl>(channel)) {}

GpgKeyRepository::~GpgKeyRepository() = default;

auto GpgKeyRepository::GetKey(const QString& key_id, bool use_cache) -> GpgKey {
  return p_->GetKey(key_id, use_cache);
}

auto GpgKeyRepository::GetKeyPtr(const QString& key_id, bool use_cache)
    -> QSharedPointer<GpgKey> {
  return p_->GetKeyPtr(key_id, use_cache);
}

auto GpgKeyRepository::GetPubkey(const QString& key_id, bool use_cache)
    -> GpgKey {
  return p_->GetPubkey(key_id, use_cache);
}

auto GpgKeyRepository::GetPubkeyPtr(const QString& key_id, bool use_cache)
    -> GpgKeyPtr {
  return p_->GetPubkeyPtr(key_id, use_cache);
}

auto GpgKeyRepository::FlushKeyCache() -> bool { return p_->FlushKeyCache(); }

auto GpgKeyRepository::GetKeys(const KeyIdArgsList& ids) -> GpgKeyList {
  return p_->GetKeys(ids);
}

auto GpgKeyRepository::Fetch() -> GpgKeyPtrList { return p_->Fetch(); }

auto GpgKeyRepository::GetKeyORSubkeyPtr(const QString& key_id)
    -> GpgAbstractKeyPtr {
  return p_->GetKeyORSubkeyPtr(key_id);
}
}  // namespace GpgFrontend
