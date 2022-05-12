/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GpgKeyGetter.h"

#include <gpg-error.h>

#include <mutex>
#include <shared_mutex>
#include <utility>

#include "GpgConstants.h"
#include "easylogging++.h"
#include "model/GpgKey.h"

GpgFrontend::GpgKeyGetter::GpgKeyGetter(int channel)
    : SingletonFunctionObject<GpgKeyGetter>(channel) {
  LOG(INFO) << "called"
            << "channel:" << channel;
}

GpgFrontend::GpgKey GpgFrontend::GpgKeyGetter::GetKey(const std::string& fpr,
                                                      bool use_cache) {
  LOG(INFO) << "called";

  // find in cache first
  if (use_cache) {
    auto key = get_key_in_cache(fpr);
    if (key.IsGood()) return key;
  }

  gpgme_key_t _p_key = nullptr;
  gpgme_get_key(ctx_, fpr.c_str(), &_p_key, 1);
  if (_p_key == nullptr) {
    DLOG(WARNING) << "GpgKeyGetter GetKey Private _p_key Null fpr" << fpr;
    return GetPubkey(fpr);
  } else {
    return GpgKey(std::move(_p_key));
  }
}

GpgFrontend::GpgKey GpgFrontend::GpgKeyGetter::GetPubkey(const std::string& fpr,
                                                         bool use_cache) {
  // find in cache first
  if (use_cache) {
    auto key = get_key_in_cache(fpr);
    if (key.IsGood()) return key;
  }

  gpgme_key_t _p_key = nullptr;
  gpgme_get_key(ctx_, fpr.c_str(), &_p_key, 0);
  if (_p_key == nullptr)
    DLOG(WARNING) << "GpgKeyGetter GetKey _p_key Null" << fpr;
  return GpgKey(std::move(_p_key));
}

GpgFrontend::KeyLinkListPtr GpgFrontend::GpgKeyGetter::FetchKey() {
  // get the lock
  std::lock_guard<std::mutex> lock(keys_cache_mutex_);

  LOG(INFO) << "GpgKeyGetter FetchKey"
            << "channel id:" << GetChannel();

  auto keys_list = std::make_unique<GpgKeyLinkList>();

  LOG(INFO) << "cache address:" << &keys_cache_ << "object address" << this;

  for (const auto& [key, value] : keys_cache_) {
    LOG(INFO) << "FetchKey Id:" << value.GetId();
    keys_list->push_back(value.Copy());
  }
  LOG(INFO) << "ended";
  return keys_list;
}

void GpgFrontend::GpgKeyGetter::FlushKeyCache() {
  LOG(INFO) << "called"
            << "channel id: " << GetChannel();

  // clear the keys cache
  keys_cache_.clear();

  // init
  GpgError err = gpgme_op_keylist_start(ctx_, nullptr, 0);

  // for debug
  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  // return when error
  if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) return;

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

      LOG(INFO) << "LoadKey Fpr:" << key->fpr << "Id:" << key->subkeys->keyid;
      keys_cache_.insert({key->subkeys->keyid, std::move(gpg_key)});
    }
  }

  LOG(INFO) << "cache address:" << &keys_cache_ << "object address" << this;

  // for debug
  assert(check_gpg_error_2_err_code(err, GPG_ERR_EOF) == GPG_ERR_EOF);

  err = gpgme_op_keylist_end(ctx_);
  assert(check_gpg_error_2_err_code(err, GPG_ERR_EOF) == GPG_ERR_NO_ERROR);

  LOG(INFO) << "ended";
}

GpgFrontend::KeyListPtr GpgFrontend::GpgKeyGetter::GetKeys(
    const KeyIdArgsListPtr& ids) {
  auto keys = std::make_unique<KeyArgsList>();
  for (const auto& id : *ids) keys->push_back(GetKey(id));
  return keys;
}

GpgFrontend::KeyLinkListPtr GpgFrontend::GpgKeyGetter::GetKeysCopy(
    const GpgFrontend::KeyLinkListPtr& keys) {
  // get the lock
  std::lock_guard<std::mutex> lock(ctx_mutex_);
  auto keys_copy = std::make_unique<GpgKeyLinkList>();
  for (const auto& key : *keys) keys_copy->push_back(key.Copy());
  return keys_copy;
}

GpgFrontend::KeyListPtr GpgFrontend::GpgKeyGetter::GetKeysCopy(
    const GpgFrontend::KeyListPtr& keys) {
  // get the lock
  std::lock_guard<std::mutex> lock(ctx_mutex_);
  auto keys_copy = std::make_unique<KeyArgsList>();
  for (const auto& key : *keys) keys_copy->push_back(key.Copy());
  return keys_copy;
}

GpgFrontend::GpgKey GpgFrontend::GpgKeyGetter::get_key_in_cache(
    const std::string& id) {
  std::lock_guard<std::mutex> lock(keys_cache_mutex_);
  if (keys_cache_.find(id) != keys_cache_.end()) {
    std::lock_guard<std::mutex> lock(ctx_mutex_);
    // return a copy of the key in cache
    return keys_cache_[id].Copy();
  }
  // return a bad key
  return GpgKey();
}
