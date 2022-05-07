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

#include "GpgConstants.h"

GpgFrontend::GpgKeyGetter::GpgKeyGetter(int channel)
    : SingletonFunctionObject<GpgKeyGetter>(channel) {}

GpgFrontend::GpgKey GpgFrontend::GpgKeyGetter::GetKey(const std::string& fpr) {
  gpgme_key_t _p_key = nullptr;
  gpgme_get_key(ctx_, fpr.c_str(), &_p_key, 1);
  if (_p_key == nullptr) {
    DLOG(WARNING) << "GpgKeyGetter GetKey Private _p_key Null fpr" << fpr;
    return GetPubkey(fpr);
  } else {
    return GpgKey(std::move(_p_key));
  }
}

GpgFrontend::GpgKey GpgFrontend::GpgKeyGetter::GetPubkey(
    const std::string& fpr) {
  gpgme_key_t _p_key = nullptr;
  gpgme_get_key(ctx_, fpr.c_str(), &_p_key, 0);
  if (_p_key == nullptr)
    DLOG(WARNING) << "GpgKeyGetter GetKey _p_key Null" << fpr;
  return GpgKey(std::move(_p_key));
}

GpgFrontend::KeyLinkListPtr GpgFrontend::GpgKeyGetter::FetchKey() {
  gpgme_error_t err;

  auto keys_list = std::make_unique<GpgKeyLinkList>();

  LOG(INFO) << "GpgKeyGetter FetchKey"
            << "ctx address" << ctx_;

  err = gpgme_op_keylist_start(ctx_, nullptr, 0);
  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  gpgme_key_t key;
  while ((err = gpgme_op_keylist_next(ctx_, &key)) == GPG_ERR_NO_ERROR) {
    keys_list->push_back(GetKey(key->fpr));
  }

  assert(check_gpg_error_2_err_code(err, GPG_ERR_EOF) == GPG_ERR_EOF);

  err = gpgme_op_keylist_end(ctx_);

  assert(check_gpg_error_2_err_code(err, GPG_ERR_EOF) == GPG_ERR_NO_ERROR);

  return keys_list;
}

GpgFrontend::KeyListPtr GpgFrontend::GpgKeyGetter::GetKeys(
    const KeyIdArgsListPtr& ids) {
  auto keys = std::make_unique<KeyArgsList>();
  for (const auto& id : *ids) keys->push_back(GetKey(id));
  return keys;
}

GpgFrontend::KeyLinkListPtr GpgFrontend::GpgKeyGetter::GetKeysCopy(
    const GpgFrontend::KeyLinkListPtr& keys) {
  auto keys_copy = std::make_unique<GpgKeyLinkList>();
  for (const auto& key : *keys) keys_copy->push_back(key.Copy());
  return keys_copy;
}

GpgFrontend::KeyListPtr GpgFrontend::GpgKeyGetter::GetKeysCopy(
    const GpgFrontend::KeyListPtr& keys) {
  auto keys_copy = std::make_unique<KeyArgsList>();
  for (const auto& key : *keys) keys_copy->push_back(key.Copy());
  return keys_copy;
}
