/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

GpgFrontend::GpgKey &&
GpgFrontend::GpgKeyGetter::GetKey(const std::string &fpr) {
  gpgme_key_t _p_key;
  gpgme_get_key(ctx, fpr.c_str(), &_p_key, 1);
  return std::move(GpgKey(std::move(_p_key)));
}

GpgFrontend::GpgKey &&
GpgFrontend::GpgKeyGetter::GetPubkey(const std::string &fpr) {
  gpgme_key_t _p_key;
  gpgme_get_key(ctx, fpr.c_str(), &_p_key, 0);
  return std::move(GpgKey(std::move(_p_key)));
}

GpgFrontend::KeyListPtr GpgFrontend::GpgKeyGetter::FetchKey() {

  gpgme_error_t err;

  qDebug() << "Clear List and Map";

  KeyListPtr keys_list = std::make_unique<std::vector<GpgKey>>();

  qDebug() << "Operate KeyList Start";

  err = gpgme_op_keylist_start(ctx, nullptr, 0);
  assert(gpg_err_code(err) != GPG_ERR_NO_ERROR);

  qDebug() << "Start Loop";

  gpgme_key_t key;
  while ((err = gpgme_op_keylist_next(ctx, &key)) == GPG_ERR_NO_ERROR) {
    keys_list->push_back(GpgKey(std::move(key)));
    qDebug() << "Append Key" << keys_list->back().id().c_str();
  }

  assert(gpg_err_code(err) != GPG_ERR_NO_ERROR);

  err = gpgme_op_keylist_end(ctx);

  qDebug() << "Operate KeyList End";

  return keys_list;
}
