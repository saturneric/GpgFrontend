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

#include "gpg/function/GpgKeyOpera.h"

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/format.hpp>
#include <boost/process/async_pipe.hpp>
#include <memory>
#include <string>
#include <vector>

#include "gpg/GpgConstants.h"
#include "gpg/GpgGenKeyInfo.h"
#include "gpg/function/GpgCommandExecutor.h"
#include "gpg/function/GpgKeyGetter.h"

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgFrontend::GpgKeyOpera::DeleteKeys(
    GpgFrontend::KeyIdArgsListPtr key_ids) {
  GpgError err;
  for (const auto& tmp : *key_ids) {
    auto key = GpgKeyGetter::GetInstance().GetKey(tmp);
    if (key.good()) {
      LOG(INFO) << "GpgKeyOpera DeleteKeys Get Key Good";
      err = check_gpg_error(gpgme_op_delete(ctx, gpgme_key_t(key), 1));
      assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
    } else
      LOG(WARNING) << "GpgKeyOpera DeleteKeys get key failed" << key.fpr();
  }
}

/**
 * Set the expire date and time of a key pair(actually the master key) or subkey
 * @param key target key pair
 * @param subkey null if master key
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

  LOG(INFO) << key.id() << subkey_fpr << expires_time;

  GpgError err;
  if (key.fpr() == subkey_fpr || subkey_fpr.empty())
    err = gpgme_op_setexpire(ctx, gpgme_key_t(key), expires_time, nullptr, 0);
  else
    err = gpgme_op_setexpire(ctx, gpgme_key_t(key), expires_time,
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
    const GpgKey& key, const std::string& output_file_name) {
  auto args = std::vector<std::string>{
      "--no-tty",       "--command-fd", "0",      "--status-fd", "1", "-o",
      output_file_name, "--gen-revoke", key.fpr()};

  using boost::asio::async_write;
  using boost::process::async_pipe;
#ifndef WINDOWS
  GpgCommandExecutor::GetInstance().Execute(
      args, [](async_pipe& in, async_pipe& out) -> void {
        //        boost::asio::streambuf buff;
        //        boost::asio::read_until(in, buff, '\n');
        //
        //        std::istream is(&buff);
        //
        //        while (!is.eof()) {
        //          std::string line;
        //          is >> line;
        //          LOG(INFO) << "line" << line;
        //          boost::algorithm::trim(line);
        //          if (line == std::string("[GNUPG:] GET_BOOL
        //          gen_revoke.okay")) {
        //
        //          } else if (line ==
        //                     std::string(
        //                         "[GNUPG:] GET_LINE
        //                         ask_revocation_reason.code")) {
        //
        //          } else if (line ==
        //                     std::string(
        //                         "[GNUPG:] GET_LINE
        //                         ask_revocation_reason.text")) {
        //
        //          } else if (line ==
        //                     std::string("[GNUPG:] GET_BOOL
        //                     openfile.overwrite.okay")) {
        //
        //          } else if (line ==
        //                     std::string(
        //                         "[GNUPG:] GET_BOOL
        //                         ask_revocation_reason.okay")) {
        //
        //          }
        //        }
      });
#endif
}

/**
 * Generate a new key pair
 * @param params key generation args
 * @return error information
 */
GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::GenerateKey(
    const std::unique_ptr<GenKeyInfo>& params, GpgGenKeyResult& result) {
  auto userid_utf8 = params->getUserid();
  const char* userid = userid_utf8.c_str();
  auto algo_utf8 = params->getAlgo() + params->getKeySizeStr();

  LOG(INFO) << "GpgFrontend::GpgKeyOpera::GenerateKey Params"
            << params->getAlgo() << params->getKeySizeStr();

  const char* algo = algo_utf8.c_str();
  unsigned long expires = 0;
  {
    using namespace boost::posix_time;
    using namespace std::chrono;
    expires = to_time_t(ptime(params->getExpired())) -
              system_clock::to_time_t(system_clock::now());
  }

  GpgError err;

  if (ctx.GetInfo().GnupgVersion >= "2.1.0") {
    unsigned int flags = 0;

    if (!params->isSubKey()) flags |= GPGME_CREATE_CERT;
    if (params->isAllowEncryption()) flags |= GPGME_CREATE_ENCR;
    if (params->isAllowSigning()) flags |= GPGME_CREATE_SIGN;
    if (params->isAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
    if (params->isNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
    if (params->isNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

    LOG(INFO) << "GpgFrontend::GpgKeyOpera::GenerateKey Args: " << userid
              << algo << expires << flags;

    err = gpgme_op_createkey(ctx, userid, algo, 0, expires, nullptr, flags);

  } else {
    std::stringstream ss;

    auto param_format =
        boost::format{
            "<GnupgKeyParms format=\"internal\">\n"
            "Key-Type: %1%\n"
            "Subkey-Type: %2%\n"
            "Name-Real: %3%\n"
            "Name-Comment: %4%\n"
            "Name-Email: %5%\n"} %
        params->getAlgo() % params->getKeySize() % params->getName() %
        params->getComment() % params->getEmail();
    ss << param_format;

    if (!params->isNonExpired())
      ss << boost::format{"Expire-Date: %1%\n"} % expires;
    if (!params->isNoPassPhrase())
      ss << boost::format{"Passphrase: %1%\n"} % params->getPassPhrase();

    ss << "</GnupgKeyParms>";

    err = gpgme_op_genkey(ctx, ss.str().c_str(), nullptr, nullptr);
  }

  if (check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR) {
    auto temp_result = _new_result(gpgme_op_genkey_result(ctx));
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
  if (!params->isSubKey()) return GPG_ERR_CANCELED;

  auto algo_utf8 = (params->getAlgo() + params->getKeySizeStr());
  const char* algo = algo_utf8.c_str();
  unsigned long expires = 0;
  {
    using namespace boost::posix_time;
    using namespace std::chrono;
    expires = to_time_t(ptime(params->getExpired())) -
              system_clock::to_time_t(system_clock::now());
  }
  unsigned int flags = 0;

  if (!params->isSubKey()) flags |= GPGME_CREATE_CERT;
  if (params->isAllowEncryption()) flags |= GPGME_CREATE_ENCR;
  if (params->isAllowSigning()) flags |= GPGME_CREATE_SIGN;
  if (params->isAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
  if (params->isNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;

  flags |= GPGME_CREATE_NOPASSWD;

  LOG(INFO) << "GpgFrontend::GpgKeyOpera::GenerateSubkey Args: " << key.id()
            << algo << expires << flags;

  auto err =
      gpgme_op_createsubkey(ctx, gpgme_key_t(key), algo, 0, expires, flags);
  return check_gpg_error(err);
}

GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::ModifyPassword(
    const GpgFrontend::GpgKey& key) {
  if (ctx.GetInfo().GnupgVersion < "2.0.15") {
    LOG(ERROR) << _("operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }
  auto err = gpgme_op_passwd(ctx, gpgme_key_t(key), 0);
  return check_gpg_error(err);
}
GpgFrontend::GpgError GpgFrontend::GpgKeyOpera::ModifyTOFUPolicy(
    const GpgFrontend::GpgKey& key, gpgme_tofu_policy_t tofu_policy) {
  if (ctx.GetInfo().GnupgVersion < "2.1.10") {
    LOG(ERROR) << _("operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }
  auto err = gpgme_op_tofu_policy(ctx, gpgme_key_t(key), tofu_policy);
  return check_gpg_error(err);
}

void GpgFrontend::GpgKeyOpera::DeleteKey(const GpgFrontend::KeyId& key_id) {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(key_id);
  DeleteKeys(std::move(keys));
}
