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

#include "gpg/function/GpgKeyOpera.h"
#include "gpg/GpgConstants.h"
#include "gpg/GpgGenKeyInfo.h"
#include "gpg/function/GpgCommandExecutor.h"
#include "gpg/function/GpgKeyGetter.h"

#include <boost/asio/read_until.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include <boost/process/async_pipe.hpp>
#include <memory>
#include <string>
#include <vector>

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgFrontend::GpgKeyOpera::DeleteKeys(
    GpgFrontend::KeyIdArgsListPtr uid_list) {
  GpgError err;
  for (const auto &tmp : *uid_list) {
    auto key = GpgKeyGetter::GetInstance().GetKey(tmp);
    if (key.good()) {
      LOG(INFO) << "GpgKeyOpera DeleteKeys Get Key Good";
      err = check_gpg_error(gpgme_op_delete(ctx, gpgme_key_t(key), 1));
      assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
    } else
      LOG(WARNING) << "GpgKeyOpera DeleteKeys Get Key Bad";
  }
}

/**
 * Set the expire date and time of a key pair(actually the master key) or subkey
 * @param key target key pair
 * @param subkey null if master key
 * @param expires date and time
 * @return if successful
 */
void GpgFrontend::GpgKeyOpera::SetExpire(
    const GpgKey &key, std::unique_ptr<GpgSubKey> &subkey,
    std::unique_ptr<boost::gregorian::date> &expires) {
  unsigned long expires_time = 0;
  if (expires != nullptr) {
    using namespace boost::posix_time;
    expires_time = to_time_t(ptime(*expires));
  }

  const char *sub_fprs = nullptr;

  if (subkey != nullptr)
    sub_fprs = subkey->fpr().c_str();

  auto err =
      gpgme_op_setexpire(ctx, gpgme_key_t(key), expires_time, sub_fprs, 0);

  assert(gpg_err_code(err) != GPG_ERR_NO_ERROR);
}

/**
 * Generate revoke cert of a key pair
 * @param key target key pair
 * @param outputFileName out file name(path)
 * @return the process doing this job
 */
void GpgFrontend::GpgKeyOpera::GenerateRevokeCert(
    const GpgKey &key, const std::string &output_file_name) {
  auto args = std::vector<std::string>{"--command-fd", "0",
                                       "--status-fd",  "1",
                                       "-o",           output_file_name.c_str(),
                                       "--gen-revoke", key.fpr().c_str()};

  using boost::process::async_pipe;
  GpgCommandExecutor::GetInstance().Execute(
      args, [](async_pipe &in, async_pipe &out) -> void {
        boost::asio::streambuf buff;
        boost::asio::read_until(in, buff, '\n');

        std::string line;
        std::istream is(&buff);
        is >> line;
        // Code From Gpg4Win
      });
}

/**
 * Generate a new key pair
 * @param params key generation args
 * @return error information
 */
GpgFrontend::GpgError
GpgFrontend::GpgKeyOpera::GenerateKey(std::unique_ptr<GenKeyInfo> params) {

  auto userid_utf8 = params->getUserid();
  const char *userid = userid_utf8.c_str();
  auto algo_utf8 = (params->getAlgo() + params->getKeySizeStr());
  const char *algo = algo_utf8.c_str();
  unsigned long expires = 0;
  {
    using namespace boost::posix_time;
    expires = to_time_t(ptime(params->getExpired()));
  }

  unsigned int flags = 0;

  if (!params->isSubKey())
    flags |= GPGME_CREATE_CERT;
  if (params->isAllowEncryption())
    flags |= GPGME_CREATE_ENCR;
  if (params->isAllowSigning())
    flags |= GPGME_CREATE_SIGN;
  if (params->isAllowAuthentication())
    flags |= GPGME_CREATE_AUTH;
  if (params->isNonExpired())
    flags |= GPGME_CREATE_NOEXPIRE;
  if (params->isNoPassPhrase())
    flags |= GPGME_CREATE_NOPASSWD;

  auto err = gpgme_op_createkey(ctx, userid, algo, 0, expires, nullptr, flags);
  return check_gpg_error(err);
}

/**
 * Generate a new subkey of a certain key pair
 * @param key target key pair
 * @param params opera args
 * @return error info
 */
GpgFrontend::GpgError
GpgFrontend::GpgKeyOpera::GenerateSubkey(const GpgKey &key,
                                         std::unique_ptr<GenKeyInfo> params) {

  if (!params->isSubKey())
    return GPG_ERR_CANCELED;

  auto algo_utf8 = (params->getAlgo() + params->getKeySizeStr());
  const char *algo = algo_utf8.c_str();
  unsigned long expires = 0;
  {
    using namespace boost::posix_time;
    expires = to_time_t(ptime(params->getExpired()));
  }
  unsigned int flags = 0;

  if (!params->isSubKey())
    flags |= GPGME_CREATE_CERT;
  if (params->isAllowEncryption())
    flags |= GPGME_CREATE_ENCR;
  if (params->isAllowSigning())
    flags |= GPGME_CREATE_SIGN;
  if (params->isAllowAuthentication())
    flags |= GPGME_CREATE_AUTH;
  if (params->isNonExpired())
    flags |= GPGME_CREATE_NOEXPIRE;

  flags |= GPGME_CREATE_NOPASSWD;

  auto err =
      gpgme_op_createsubkey(ctx, gpgme_key_t(key), algo, 0, expires, flags);
  return check_gpg_error(err);
}