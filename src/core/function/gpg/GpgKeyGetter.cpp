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

#include "GpgKeyGetter.h"

#include <gpg-error.h>

#include <mutex>
#include <shared_mutex>

#include "core/GpgModel.h"
#include "core/function/gpg/GpgContext.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

class GpgKeyGetter::Impl : public SingletonFunctionObject<GpgKeyGetter::Impl> {
 public:
  explicit Impl(int channel)
      : SingletonFunctionObject<GpgKeyGetter::Impl>(channel) {
    SPDLOG_DEBUG("called channel: {}", channel);
  }

  auto GetKey(const std::string& fpr, bool use_cache) -> GpgKey {
    // find in cache first
    if (use_cache) {
      auto key = get_key_in_cache(fpr);
      if (key.IsGood()) return key;
    }

    gpgme_key_t p_key = nullptr;
    gpgme_get_key(ctx_, fpr.c_str(), &p_key, 1);
    if (p_key == nullptr) {
      SPDLOG_WARN("GpgKeyGetter GetKey Private _p_key Null fpr", fpr);
      return GetPubkey(fpr, true);
    }
    return GpgKey(std::move(p_key));
  }

  auto GetPubkey(const std::string& fpr, bool use_cache) -> GpgKey {
    // find in cache first
    if (use_cache) {
      auto key = get_key_in_cache(fpr);
      if (key.IsGood()) return key;
    }

    gpgme_key_t p_key = nullptr;
    gpgme_get_key(ctx_, fpr.c_str(), &p_key, 0);
    if (p_key == nullptr) SPDLOG_WARN("GpgKeyGetter GetKey _p_key Null", fpr);
    return GpgKey(std::move(p_key));
  }

  auto FetchKey() -> KeyLinkListPtr {
    // get the lock
    std::lock_guard<std::mutex> lock(keys_cache_mutex_);

    auto keys_list = std::make_unique<GpgKeyLinkList>();

    for (const auto& [key, value] : keys_cache_) {
      keys_list->push_back(value.Copy());
    }
    return keys_list;
  }

  void FlushKeyCache() {
    SPDLOG_DEBUG("called channel id: {}", GetChannel());

    // clear the keys cache
    keys_cache_.clear();

    // init
    GpgError err = gpgme_op_keylist_start(ctx_, nullptr, 0);

    // for debug
    assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);

    // return when error
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return;

    {
      // get the lock
      std::lock_guard<std::mutex> lock(keys_cache_mutex_);
      gpgme_key_t key;
      while ((err = gpgme_op_keylist_next(ctx_, &key)) == GPG_ERR_NO_ERROR) {
        auto gpg_key = GpgKey(std::move(key));

        // detect if the key is in a smartcard
        // if so, try to get full information using gpgme_get_key()
        // this maybe a bug in gpgme
        if (gpg_key.IsHasCardKey()) {
          gpg_key = GetKey(gpg_key.GetId(), false);
        }

        keys_cache_.insert({gpg_key.GetId(), std::move(gpg_key)});
      }
    }

    SPDLOG_DEBUG("cache address: {} object address: {}",
                 static_cast<void*>(&keys_cache_), static_cast<void*>(this));

    // for debug
    assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_EOF);

    err = gpgme_op_keylist_end(ctx_);
    assert(CheckGpgError2ErrCode(err, GPG_ERR_EOF) == GPG_ERR_NO_ERROR);
  }

  auto GetKeys(const KeyIdArgsListPtr& ids) -> KeyListPtr {
    auto keys = std::make_unique<KeyArgsList>();
    for (const auto& key_id : *ids) keys->emplace_back(GetKey(key_id, true));
    return keys;
  }

  auto GetKeysCopy(const KeyLinkListPtr& keys) -> KeyLinkListPtr {
    // get the lock
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto keys_copy = std::make_unique<GpgKeyLinkList>();
    for (const auto& key : *keys) keys_copy->emplace_back(key.Copy());
    return keys_copy;
  }

  auto GetKeysCopy(const KeyListPtr& keys) -> KeyListPtr {
    // get the lock
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    auto keys_copy = std::make_unique<KeyArgsList>();
    for (const auto& key : *keys) keys_copy->emplace_back(key.Copy());
    return keys_copy;
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
  std::map<std::string, GpgKey> keys_cache_;

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
  auto get_key_in_cache(const std::string& key_id) -> GpgKey {
    std::lock_guard<std::mutex> lock(keys_cache_mutex_);
    if (keys_cache_.find(key_id) != keys_cache_.end()) {
      std::lock_guard<std::mutex> lock(ctx_mutex_);
      // return a copy of the key in cache
      return keys_cache_[key_id].Copy();
    }

    // return a bad key
    return {};
  }
};

GpgKeyGetter::GpgKeyGetter(int channel)
    : SingletonFunctionObject<GpgKeyGetter>(channel),
      p_(std::make_unique<Impl>(channel)) {
  SPDLOG_DEBUG("called channel: {}", channel);
}

auto GpgKeyGetter::GetKey(const std::string& key_id, bool use_cache) -> GpgKey {
  return p_->GetKey(key_id, use_cache);
}

auto GpgKeyGetter::GetPubkey(const std::string& key_id, bool use_cache)
    -> GpgKey {
  return p_->GetPubkey(key_id, use_cache);
}

void GpgKeyGetter::FlushKeyCache() { p_->FlushKeyCache(); }

auto GpgKeyGetter::GetKeys(const KeyIdArgsListPtr& ids) -> KeyListPtr {
  return p_->GetKeys(ids);
}

auto GpgKeyGetter::GetKeysCopy(const KeyLinkListPtr& keys) -> KeyLinkListPtr {
  return p_->GetKeysCopy(keys);
}

auto GpgKeyGetter::GetKeysCopy(const KeyListPtr& keys) -> KeyListPtr {
  return p_->GetKeysCopy(keys);
}

auto GpgKeyGetter::FetchKey() -> KeyLinkListPtr { return p_->FetchKey(); }

}  // namespace GpgFrontend
