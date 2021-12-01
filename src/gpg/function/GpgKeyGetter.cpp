/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "gpg/function/GpgKeyGetter.h"

#include <gpg-error.h>

#include "GpgConstants.h"

GpgFrontend::GpgKey GpgFrontend::GpgKeyGetter::GetKey(const std::string& fpr) {
  gpgme_key_t _p_key;
  gpgme_get_key(ctx, fpr.c_str(), &_p_key, 1);
  if (_p_key == nullptr) {
    DLOG(WARNING) << "GpgKeyGetter GetKey Private _p_key Null fpr" << fpr;
    return GetPubkey(fpr);
  } else {
    return GpgKey(std::move(_p_key));
  }
}

GpgFrontend::GpgKey GpgFrontend::GpgKeyGetter::GetPubkey(
    const std::string& fpr) {
  gpgme_key_t _p_key;
  gpgme_get_key(ctx, fpr.c_str(), &_p_key, 0);
  if (_p_key == nullptr)
    DLOG(WARNING) << "GpgKeyGetter GetKey _p_key Null" << fpr;
  return GpgKey(std::move(_p_key));
}

GpgFrontend::KeyLinkListPtr GpgFrontend::GpgKeyGetter::FetchKey() {
  gpgme_error_t err;

  auto keys_list = std::make_unique<GpgKeyLinkList>();

  err = gpgme_op_keylist_start(ctx, nullptr, 0);
  assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);

  gpgme_key_t key;
  while ((err = gpgme_op_keylist_next(ctx, &key)) == GPG_ERR_NO_ERROR) {
    keys_list->push_back(GpgKey(std::move(key)));
  }

  assert(check_gpg_error_2_err_code(err, GPG_ERR_EOF) == GPG_ERR_EOF);

  err = gpgme_op_keylist_end(ctx);

  return keys_list;
}
GpgFrontend::KeyListPtr GpgFrontend::GpgKeyGetter::GetKeys(
    const KeyIdArgsListPtr& ids) {
  auto keys = std::make_unique<KeyArgsList>();
  for (const auto& id : *ids) keys->push_back(GetKey(id));
  return keys;
}
