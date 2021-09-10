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

#include "gpg/function/GpgKeyManager.h"
#include "gpg/function/BasicOperator.h"
#include "gpg/function/GpgKeyGetter.h"
#include <boost/date_time/posix_time/conversion.hpp>
#include <string>

bool GpgFrontend::GpgKeyManager::signKey(
    const GpgFrontend::GpgKey &target, GpgFrontend::KeyArgsList &keys,
    const std::string &uid, std::unique_ptr<boost::gregorian::date> &expires) {
  using namespace boost::posix_time;

  BasicOperator::GetInstance().SetSigners(keys);

  unsigned int flags = 0;
  unsigned int expires_time_t = 0;

  if (expires == nullptr)
    flags |= GPGME_KEYSIGN_NOEXPIRE;
  else
    expires_time_t = to_time_t(ptime(*expires));

  auto err = check_gpg_error(gpgme_op_keysign(
      ctx, gpgme_key_t(target), uid.c_str(), expires_time_t, flags));

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyManager::revSign(
    const GpgFrontend::GpgKey &key,
    const GpgFrontend::GpgKeySignature &signature) {

  auto &key_getter = GpgKeyGetter::GetInstance();
  auto signing_key = key_getter.GetKey(signature.keyid());

  auto err = check_gpg_error(gpgme_op_revsig(ctx, gpgme_key_t(key),
                                             gpgme_key_t(signing_key),
                                             signature.uid().data(), 0));
  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyManager::setExpire(
    const GpgFrontend::GpgKey &key, std::unique_ptr<GpgSubKey> &subkey,
    std::unique_ptr<boost::gregorian::date> &expires) {
  using namespace boost::posix_time;

  unsigned long expires_time = 0;

  if (expires != nullptr)
    expires_time = to_time_t(ptime(*expires));

  const char *sub_fprs = nullptr;

  if (subkey != nullptr)
    sub_fprs = subkey->fpr().c_str();

  auto err = check_gpg_error(
      gpgme_op_setexpire(ctx, gpgme_key_t(key), expires_time, sub_fprs, 0));

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}
