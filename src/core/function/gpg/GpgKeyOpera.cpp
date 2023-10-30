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

#include "GpgKeyOpera.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/format.hpp>
#include <boost/process/async_pipe.hpp>

#include "GpgCommandExecutor.h"
#include "GpgKeyGetter.h"
#include "core/GpgConstants.h"
#include "core/GpgGenKeyInfo.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend {

GpgKeyOpera::GpgKeyOpera(int channel)
    : SingletonFunctionObject<GpgKeyOpera>(channel) {}

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgKeyOpera::DeleteKeys(GpgFrontend::KeyIdArgsListPtr key_ids) {
  GpgError err;
  for (const auto& tmp : *key_ids) {
    auto key = GpgKeyGetter::GetInstance().GetKey(tmp);
    if (key.IsGood()) {
      err = CheckGpgError(
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
GpgError GpgKeyOpera::SetExpire(
    const GpgKey& key, const SubkeyId& subkey_fpr,
    std::unique_ptr<boost::posix_time::ptime>& expires) {
  unsigned long expires_time = 0;

  if (expires != nullptr) {
    using namespace boost::posix_time;
    using namespace std::chrono;
    expires_time =
        to_time_t(*expires) - system_clock::to_time_t(system_clock::now());
  }

  SPDLOG_DEBUG(key.GetId(), subkey_fpr, expires_time);

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
void GpgKeyOpera::GenerateRevokeCert(const GpgKey& key,
                                     const std::string& output_file_path) {
  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", std::string{});
  // get all components
  GpgCommandExecutor::ExecuteSync(
      {app_path,
       {"--command-fd", "0", "--status-fd", "1", "--no-tty", "-o",
        output_file_path, "--gen-revoke", key.GetFingerprint().c_str()},
       [=](int exit_code, const std::string& p_out, const std::string& p_err) {
         if (exit_code != 0) {
           SPDLOG_ERROR(
               "gnupg gen revoke execute error, process stderr: {}, process "
               "stdout: {}",
               p_err, p_out);
         } else {
           SPDLOG_DEBUG(
               "gnupg gen revoke exit_code: {}, process stdout size: {}",
               exit_code, p_out.size());
         }
       },
       nullptr,
       [](QProcess* proc) -> void {
         // Code From Gpg4Win
         while (proc->canReadLine()) {
           const QString line = QString::fromUtf8(proc->readLine()).trimmed();
           SPDLOG_DEBUG("line: {}", line.toStdString());
           if (line == QLatin1String("[GNUPG:] GET_BOOL gen_revoke.okay")) {
             proc->write("y\n");
           } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                            "ask_revocation_reason.code")) {
             proc->write("0\n");
           } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                            "ask_revocation_reason.text")) {
             proc->write("\n");
           } else if (line ==
                      QLatin1String(
                          "[GNUPG:] GET_BOOL openfile.overwrite.okay")) {
             // We asked before
             proc->write("y\n");
           } else if (line == QLatin1String("[GNUPG:] GET_BOOL "
                                            "ask_revocation_reason.okay")) {
             proc->write("y\n");
           }
         }
       }});
}

/**
 * Generate a new key pair
 * @param params key generation args
 * @return error information
 */
GpgError GpgKeyOpera::GenerateKey(const std::unique_ptr<GenKeyInfo>& params,
                                  GpgGenKeyResult& result) {
  auto userid_utf8 = params->GetUserid();
  const char* userid = userid_utf8.c_str();
  auto algo_utf8 = params->GetAlgo() + params->GetKeySizeStr();

  SPDLOG_DEBUG("params: {} {}", params->GetAlgo(), params->GetKeySizeStr());

  const char* algo = algo_utf8.c_str();
  unsigned long expires = 0;
  {
    using namespace boost::posix_time;
    using namespace std::chrono;
    expires = to_time_t(ptime(params->GetExpireTime())) -
              system_clock::to_time_t(system_clock::now());
  }

  GpgError err;

  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});
  SPDLOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

  if (CompareSoftwareVersion(gnupg_version, "2.1.0") >= 0) {
    unsigned int flags = 0;

    if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
    if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
    if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
    if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
    if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
    if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

    SPDLOG_DEBUG("args: {}", userid, algo, expires, flags);

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

    SPDLOG_DEBUG("params: {}", ss.str());

    err = gpgme_op_genkey(ctx_, ss.str().c_str(), nullptr, nullptr);
  }

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    auto temp_result = NewResult(gpgme_op_genkey_result(ctx_));
    std::swap(temp_result, result);
  }

  return CheckGpgError(err);
}

/**
 * Generate a new subkey of a certain key pair
 * @param key target key pair
 * @param params opera args
 * @return error info
 */
GpgError GpgKeyOpera::GenerateSubkey(
    const GpgKey& key, const std::unique_ptr<GenKeyInfo>& params) {
  if (!params->IsSubKey()) return GPG_ERR_CANCELED;

  SPDLOG_DEBUG("generate subkey algo {} key size {}", params->GetAlgo(),
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

  SPDLOG_DEBUG("args: {} {} {} {}", key.GetId(), algo, expires, flags);

  auto err =
      gpgme_op_createsubkey(ctx_, gpgme_key_t(key), algo, 0, expires, flags);
  return CheckGpgError(err);
}

GpgError GpgKeyOpera::ModifyPassword(const GpgKey& key) {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});
  SPDLOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

  if (CompareSoftwareVersion(gnupg_version, "2.0.15") < 0) {
    SPDLOG_ERROR("operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }
  auto err = gpgme_op_passwd(ctx_, gpgme_key_t(key), 0);
  return CheckGpgError(err);
}

GpgError GpgKeyOpera::ModifyTOFUPolicy(const GpgKey& key,
                                       gpgme_tofu_policy_t tofu_policy) {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});
  SPDLOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

  if (CompareSoftwareVersion(gnupg_version, "2.1.10") < 0) {
    SPDLOG_ERROR("operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }

  auto err = gpgme_op_tofu_policy(ctx_, gpgme_key_t(key), tofu_policy);
  return CheckGpgError(err);
}

void GpgKeyOpera::DeleteKey(const GpgFrontend::KeyId& key_id) {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(key_id);
  DeleteKeys(std::move(keys));
}
}  // namespace GpgFrontend
