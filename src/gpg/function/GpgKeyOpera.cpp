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

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgFrontend::GpgKeyOpera::DeleteKeys(
    GpgFrontend::KeyIdArgsListPtr uid_list) {

  GpgError err;
  gpgme_key_t key;

  for (const auto &tmp : *uid_list) {

    err = gpgme_op_keylist_start(ctx, tmp.c_str(), 0);
    assert(gpg_err_code(err) != GPG_ERR_NO_ERROR);

    err = gpgme_op_keylist_next(ctx, &key);
    assert(gpg_err_code(err) != GPG_ERR_NO_ERROR);

    err = gpgme_op_keylist_end(ctx);
    assert(gpg_err_code(err) != GPG_ERR_NO_ERROR);

    err = gpgme_op_delete(ctx, key, 1);
    assert(gpg_err_code(err) != GPG_ERR_NO_ERROR);
  }
}

/**
 * Set the expire date and time of a key pair(actually the master key) or subkey
 * @param key target key pair
 * @param subkey null if master key
 * @param expires date and time
 * @return if successful
 */
void GpgFrontend::GpgKeyOpera::SetExpire(const GpgKey &key,
                                         std::unique_ptr<GpgSubKey> &subkey,
                                         std::unique_ptr<QDateTime> &expires) {
  unsigned long expires_time = 0;
  if (expires != nullptr) {
    qDebug() << "Expire Datetime" << expires->toString();
    expires_time = QDateTime::currentDateTime().secsTo(*expires);
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
  GpgCommandExecutor::GetInstance().Execute(
      {"--command-fd", "0", "--status-fd", "1", "-o", output_file_name.c_str(),
       "--gen-revoke", key.fpr().c_str()},
      [](QProcess *proc) -> void {
        qDebug() << "Function Called" << proc;
        // Code From Gpg4Win
        while (proc->canReadLine()) {
          const QString line = QString::fromUtf8(proc->readLine()).trimmed();
          if (line == QLatin1String("[GNUPG:] GET_BOOL gen_revoke.okay")) {
            proc->write("y\n");
          } else if (line ==
                     QLatin1String(
                         "[GNUPG:] GET_LINE ask_revocation_reason.code")) {
            proc->write("0\n");
          } else if (line ==
                     QLatin1String(
                         "[GNUPG:] GET_LINE ask_revocation_reason.text")) {
            proc->write("\n");
          } else if (line == QLatin1String(
                                 "[GNUPG:] GET_BOOL openfile.overwrite.okay")) {
            // We asked before
            proc->write("y\n");
          } else if (line ==
                     QLatin1String(
                         "[GNUPG:] GET_BOOL ask_revocation_reason.okay")) {
            proc->write("y\n");
          }
        }
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
  unsigned long expires =
      QDateTime::currentDateTime().secsTo(params->getExpired());
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
  unsigned long expires =
      QDateTime::currentDateTime().secsTo(params->getExpired());
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