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

#include "core/function/GpgKeyManager.h"

#include <boost/date_time/posix_time/conversion.hpp>
#include <string>

#include "core/function/GpgBasicOperator.h"
#include "core/function/GpgKeyGetter.h"

bool GpgFrontend::GpgKeyManager::SignKey(
    const GpgFrontend::GpgKey& target, GpgFrontend::KeyArgsList& keys,
    const std::string& uid,
    const std::unique_ptr<boost::posix_time::ptime>& expires) {
  using namespace boost::posix_time;

  GpgBasicOperator::GetInstance().SetSigners(keys);

  unsigned int flags = 0;
  unsigned int expires_time_t = 0;

  if (expires == nullptr)
    flags |= GPGME_KEYSIGN_NOEXPIRE;
  else
    expires_time_t = to_time_t(*expires);

  auto err = check_gpg_error(gpgme_op_keysign(
      ctx_, gpgme_key_t(target), uid.c_str(), expires_time_t, flags));

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyManager::RevSign(
    const GpgFrontend::GpgKey& key,
    const GpgFrontend::SignIdArgsListPtr& signature_id) {
  auto& key_getter = GpgKeyGetter::GetInstance();

  for (const auto& sign_id : *signature_id) {
    auto signing_key = key_getter.GetKey(sign_id.first);
    assert(signing_key.IsGood());
    auto err = check_gpg_error(gpgme_op_revsig(ctx_, gpgme_key_t(key),
                                               gpgme_key_t(signing_key),
                                               sign_id.second.c_str(), 0));
    if (check_gpg_error_2_err_code(err) != GPG_ERR_NO_ERROR) return false;
  }
  return true;
}

bool GpgFrontend::GpgKeyManager::SetExpire(
    const GpgFrontend::GpgKey& key, std::unique_ptr<GpgSubKey>& subkey,
    std::unique_ptr<boost::posix_time::ptime>& expires) {
  using namespace boost::posix_time;

  unsigned long expires_time = 0;

  if (expires != nullptr) expires_time = to_time_t(ptime(*expires));

  const char* sub_fprs = nullptr;

  if (subkey != nullptr) sub_fprs = subkey->GetFingerprint().c_str();

  auto err = check_gpg_error(
      gpgme_op_setexpire(ctx_, gpgme_key_t(key), expires_time, sub_fprs, 0));

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}
