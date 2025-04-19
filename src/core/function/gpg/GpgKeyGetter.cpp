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

#include "GpgKeyGetter.h"

#include <gpg-error.h>

#include <mutex>

#include "core/function/gpg/GpgContext.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

class GpgKeyGetter::Impl : public SingletonFunctionObject<GpgKeyGetter::Impl> {
 public:
  explicit Impl(int channel)
      : SingletonFunctionObject<GpgKeyGetter::Impl>(channel) {}

  auto GetKeyPtr(const QString& key_id, bool cache) -> GpgKeyPtr {
    // find in cache first
    if (cache) {
      auto key = get_key_in_cache(key_id);
      assert(key->KeyType() == GpgAbstractKeyType::kGPG_KEY);

      if (key != nullptr) return qSharedPointerDynamicCast<GpgKey>(key);

      LOG_W() << "get gpg key" << key_id
              << "from cache failed, channel: " << GetChannel();
    }

    gpgme_key_t p_key = nullptr;
    gpgme_get_key(ctx_.DefaultContext(), key_id.toUtf8(), &p_key, 1);
    if (p_key == nullptr) {
      LOG_W() << "GpgKeyGetter GetKey p_key is null, fpr: " << key_id;
      return GetPubkeyPtr(key_id, true);
    }

    return QSharedPointer<GpgKey>::create(p_key);
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

    gpgme_key_t p_key = nullptr;
    gpgme_get_key(ctx_.DefaultContext(), key_id.toUtf8(), &p_key, 0);
    if (p_key == nullptr) {
      LOG_W() << "GpgKeyGetter GetKey p_key is null, key id: " << key_id;
      return nullptr;
    }
    return QSharedPointer<GpgKey>::create(p_key);
  }

  auto FetchKey() -> GpgKeyPtrList {
    if (keys_cache_.empty() || keys_search_cache_.empty()) {
      FlushKeyCache();
    }

    auto keys_list = GpgKeyPtrList{};
    {
      // get the lock
      std::lock_guard<std::mutex> lock(keys_cache_mutex_);
      for (const auto& key : keys_cache_) {
        keys_list.push_back(key);
      }
    }
    return keys_list;
  }

  auto FetchGpgKeyList() -> GpgAbstractKeyPtrList {
    if (keys_search_cache_.empty()) {
      FlushKeyCache();
    }

    auto keys_list = GpgAbstractKeyPtrList{};
    {
      // get the lock
      std::lock_guard<std::mutex> lock(keys_cache_mutex_);
      for (const auto& key : keys_cache_) {
        keys_list.push_back(key);
      }
    }

    return keys_list;
  }

  auto FlushKeyCache() -> bool {
    // clear the keys cache
    keys_cache_.clear();
    keys_search_cache_.clear();

    // init
    GpgError err = gpgme_op_keylist_start(ctx_.DefaultContext(), nullptr, 0);

    // for debug
    assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);

    // return when error
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return false;

    {
      // get the lock
      std::lock_guard<std::mutex> lock(keys_cache_mutex_);
      gpgme_key_t key;
      while ((err = gpgme_op_keylist_next(ctx_.DefaultContext(), &key)) ==
             GPG_ERR_NO_ERROR) {
        auto g_key = QSharedPointer<GpgKey>::create(key);

        // detect if the key is in a smartcard
        // if so, try to get full information using gpgme_get_key()
        // this maybe a bug in gpgme
        if (g_key->IsHasCardKey()) {
          g_key = GetKeyPtr(g_key->ID(), false);
        }

        keys_cache_.push_back(g_key);
        keys_search_cache_.insert(g_key->ID(), g_key);
        keys_search_cache_.insert(g_key->Fingerprint(), g_key);

        for (const auto& s_key : g_key->SubKeys()) {
          if (s_key.ID() == g_key->ID()) continue;
          auto p_s_key = QSharedPointer<GpgSubKey>::create(s_key);

          // don't add adsk key or it will cause bugs
          if (p_s_key->IsADSK()) continue;

          keys_search_cache_.insert(s_key.ID(), p_s_key);
          keys_search_cache_.insert(s_key.Fingerprint(), p_s_key);
        }
      }
    }

    // for debug
    assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_EOF);

    err = gpgme_op_keylist_end(ctx_.DefaultContext());
    assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_NO_ERROR);

    return true;
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
    auto key = get_key_in_cache(key_id);

    if (key != nullptr) return key;

    LOG_W() << "get key" << key_id
            << "from cache failed, channel: " << GetChannel();

    return nullptr;
  }

 private:
  /**
   * @brief Get the gpgme context object
   *
   */
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());

  /**
   * @brief shared mutex for the keys cache
   *
   */
  mutable std::mutex ctx_mutex_;

  /**
   * @brief cache the keys with key id
   *
   */
  QMap<QString, GpgAbstractKeyPtr> keys_search_cache_;

  /**
   * @brief
   *
   */
  QContainer<GpgKeyPtr> keys_cache_;

  /**
   * @brief shared mutex for the keys cache
   *
   */
  mutable std::mutex keys_cache_mutex_;

  /**
   * @brief Get the Key object
   *
   * @param id
   * @return GpgKey
   */
  auto get_key_in_cache(const QString& key_id) -> GpgAbstractKeyPtr {
    std::lock_guard<std::mutex> lock(keys_cache_mutex_);

    if (keys_search_cache_.find(key_id) != keys_search_cache_.end()) {
      std::lock_guard<std::mutex> lock(ctx_mutex_);
      // return a copy of the key in cache
      return keys_search_cache_[key_id];
    }

    // return a bad key
    return {};
  }
};

GpgKeyGetter::GpgKeyGetter(int channel)
    : SingletonFunctionObject<GpgKeyGetter>(channel),
      p_(SecureCreateUniqueObject<Impl>(channel)) {}

GpgKeyGetter::~GpgKeyGetter() = default;

auto GpgKeyGetter::GetKey(const QString& key_id, bool use_cache) -> GpgKey {
  return p_->GetKey(key_id, use_cache);
}

auto GpgKeyGetter::GetKeyPtr(const QString& key_id,
                             bool use_cache) -> QSharedPointer<GpgKey> {
  return p_->GetKeyPtr(key_id, use_cache);
}

auto GpgKeyGetter::GetPubkey(const QString& key_id, bool use_cache) -> GpgKey {
  return p_->GetPubkey(key_id, use_cache);
}

auto GpgKeyGetter::GetPubkeyPtr(const QString& key_id,
                                bool use_cache) -> GpgKeyPtr {
  return p_->GetPubkeyPtr(key_id, use_cache);
}

auto GpgKeyGetter::FlushKeyCache() -> bool { return p_->FlushKeyCache(); }

auto GpgKeyGetter::GetKeys(const KeyIdArgsList& ids) -> GpgKeyList {
  return p_->GetKeys(ids);
}

auto GpgKeyGetter::Fetch() -> GpgKeyPtrList { return p_->Fetch(); }

auto GpgKeyGetter::GetKeyORSubkeyPtr(const QString& key_id)
    -> GpgAbstractKeyPtr {
  return p_->GetKeyORSubkeyPtr(key_id);
}
}  // namespace GpgFrontend
