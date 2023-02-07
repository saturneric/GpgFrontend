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

#include "GpgKeyOpera.h"

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/format.hpp>
#include <boost/process/async_pipe.hpp>
#include <memory>
#include <string>
#include <vector>

#include "GpgCommandExecutor.h"
#include "GpgKeyGetter.h"
#include "core/GpgConstants.h"
#include "core/GpgGenKeyInfo.h"

GpgFrontend::GpgKeyOpera::GpgKeyOpera(int channel)
    : SingletonFunctionObject<GpgKeyOpera>(channel) {}

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgFrontend::GpgKeyOpera::DeleteKeys(
    GpgFrontend::KeyIdArgsListPtr key_ids) {
  GpgError err;
  for (const auto& tmp : *key_ids) {
    auto key = GpgKeyGetter::GetInstance().GetKey(tmp);
    if (key.IsGood()) {
      err = check_gpg_error(
          gpgme_op_delete_ext(ctx_, gpgme_key_t(key),
                              GPGME_DELETE_ALLOW_SECRET | GPGME_DELETE_FORCE));
      assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
    } else {
      SPDLOG_WARN("GpgKeyOpera DeleteKeys get key failed", tmp);
    }
  }
}

/**
 * Set the expire date and time of a key pair(actually the primary key) or
 * subkey
 * @param key target key pair
 * @param subkey null if primary key
 * @param expires date and time
 * @return if successful
 */
GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::SetExpire(
    const GpgKey& key, const SubkeyId& subkey_fpr,
    std::unique_ptr<boost::posix_time::ptime>& expires) {
  unsigned long expires_time = 0;

  if (expires != nullptr) {
    using namespace boost::posix_time;
    using namespace std::chrono;
    expires_time =
        to_time_t(*expires) - system_clock::to_time_t(system_clock::now());
  }

  SPDLOG_INFO(key.GetId(), subkey_fpr, expires_time);

  GpgError err;
  if (key.GetFingerprint() == subkey_fpr || subkey_fpr.empty())
    err = gpgme_op_setexpire(ctx_, gpgme_key_t(key), expires_time, nullptr, 0);
  else
    err = gpgme_op_setexpire(ctx_, gpgme_key_t(key), expires_time,
                             subkey_fpr.c_str(), 0);

  return err;
}

/**
 * Generate revoke cert of a key pair
 * @param key target key pair
 * @param outputFileName out file name(path)
 * @return the process doing this job
 */
void GpgFrontend::GpgKeyOpera::GenerateRevokeCert(
    const GpgKey& key, const std::string& output_file_name) {}

/**
 * Generate a new key pair
 * @param params key generation args
 * @return error information
 */
GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::GenerateKey(
    const std::unique_ptr<GenKeyInfo>& params, GpgGenKeyResult& result) {
  auto userid_utf8 = params->GetUserid();
  const char* userid = userid_utf8.c_str();
  auto algo_utf8 = params->GetAlgo() + params->GetKeySizeStr();

  SPDLOG_INFO("params: {} {}", params->GetAlgo(), params->GetKeySizeStr());

  const char* algo = algo_utf8.c_str();
  unsigned long expires = 0;
  {
    using namespace boost::posix_time;
    using namespace std::chrono;
    expires = to_time_t(ptime(params->GetExpireTime())) -
              system_clock::to_time_t(system_clock::now());
  }

  GpgError err;

  SPDLOG_INFO("ctx version, {}", ctx_.GetInfo(false).GnupgVersion);

  if (ctx_.GetInfo(false).GnupgVersion >= "2.1.0") {
    unsigned int flags = 0;

    if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
    if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
    if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
    if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
    if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
    if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

    SPDLOG_INFO("args: {}", userid, algo, expires, flags);

    err = gpgme_op_createkey(ctx_, userid, algo, 0, expires, nullptr, flags);

  } else {
    std::stringstream ss;
    auto param_format =
        boost::format{
            "<GnupgKeyParms format=\"internal\">\n"
            "Key-Type: %1%\n"
            "Key-Usage: sign\n"
            "Key-Length: %2%\n"
            "Name-Real: %3%\n"
            "Name-Comment: %4%\n"
            "Name-Email: %5%\n"} %
        params->GetAlgo() % params->GetKeyLength() % params->GetName() %
        params->GetComment() % params->GetEmail();
    ss << param_format;

    if (!params->IsNonExpired()) {
      auto date = params->GetExpireTime().date();
      ss << boost::format{"Expire-Date: %1%\n"} % to_iso_string(date);
    } else
      ss << boost::format{"Expire-Date: 0\n"};
    if (!params->IsNoPassPhrase())
      ss << boost::format{"Passphrase: %1%\n"} % params->GetPassPhrase();

    ss << "</GnupgKeyParms>";

    SPDLOG_INFO("params: {}", ss.str());

    err = gpgme_op_genkey(ctx_, ss.str().c_str(), nullptr, nullptr);
  }

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR) {
    auto temp_result = _new_result(gpgme_op_genkey_result(ctx_));
    std::swap(temp_result, result);
  }

  return check_gpg_error(err);
}

/**
 * Generate a new subkey of a certain key pair
 * @param key target key pair
 * @param params opera args
 * @return error info
 */
GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::GenerateSubkey(
    const GpgKey& key, const std::unique_ptr<GenKeyInfo>& params) {
  if (!params->IsSubKey()) return GPG_ERR_CANCELED;

  SPDLOG_INFO("generate subkey algo {} key size {}", params->GetAlgo(),
              params->GetKeySizeStr());

  auto algo_utf8 = (params->GetAlgo() + params->GetKeySizeStr());
  const char* algo = algo_utf8.c_str();
  unsigned long expires = 0;
  {
    using namespace boost::posix_time;
    using namespace std::chrono;
    expires = to_time_t(ptime(params->GetExpireTime())) -
              system_clock::to_time_t(system_clock::now());
  }
  unsigned int flags = 0;

  if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
  if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
  if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
  if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
  if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
  if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

  SPDLOG_INFO("GpgFrontend::GpgKeyOpera::GenerateSubkey args: {} {} {} {}",
              key.GetId(), algo, expires, flags);

  auto err =
      gpgme_op_createsubkey(ctx_, gpgme_key_t(key), algo, 0, expires, flags);
  return check_gpg_error(err);
}

GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::ModifyPassword(
    const GpgFrontend::GpgKey& key) {
  if (ctx_.GetInfo(false).GnupgVersion < "2.0.15") {
    SPDLOG_ERROR("operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }
  auto err = gpgme_op_passwd(ctx_, gpgme_key_t(key), 0);
  return check_gpg_error(err);
}
GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::ModifyTOFUPolicy(
    const GpgFrontend::GpgKey& key, gpgme_tofu_policy_t tofu_policy) {
  if (ctx_.GetInfo(false).GnupgVersion < "2.1.10") {
    SPDLOG_ERROR("operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }
  auto err = gpgme_op_tofu_policy(ctx_, gpgme_key_t(key), tofu_policy);
  return check_gpg_error(err);
}

void GpgFrontend::GpgKeyOpera::DeleteKey(const GpgFrontend::KeyId& key_id) {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(key_id);
  DeleteKeys(std::move(keys));
}
