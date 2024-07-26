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

#include <gpg-error.h>

#include "core/GpgModel.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenKeyInfo.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyOpera::GpgKeyOpera(int channel)
    : SingletonFunctionObject<GpgKeyOpera>(channel) {}

/**
 * Delete keys
 * @param uidList key ids
 */
void GpgKeyOpera::DeleteKeys(KeyIdArgsListPtr key_ids) {
  GpgError err;
  for (const auto& tmp : *key_ids) {
    auto key = GpgKeyGetter::GetInstance().GetKey(tmp);
    if (key.IsGood()) {
      err = CheckGpgError(gpgme_op_delete_ext(
          ctx_.DefaultContext(), static_cast<gpgme_key_t>(key),
          GPGME_DELETE_ALLOW_SECRET | GPGME_DELETE_FORCE));
      assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
    } else {
      qCWarning(core) << "GpgKeyOpera DeleteKeys get key failed: " << tmp;
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
auto GpgKeyOpera::SetExpire(const GpgKey& key, const SubkeyId& subkey_fpr,
                            std::unique_ptr<QDateTime>& expires) -> GpgError {
  unsigned long expires_time = 0;

  if (expires != nullptr) {
    expires_time = QDateTime::currentDateTime().secsTo(*expires);
  }

  GpgError err;
  if (key.GetFingerprint() == subkey_fpr || subkey_fpr.isEmpty()) {
    err =
        gpgme_op_setexpire(ctx_.DefaultContext(), static_cast<gpgme_key_t>(key),
                           expires_time, nullptr, 0);
  } else {
    err =
        gpgme_op_setexpire(ctx_.DefaultContext(), static_cast<gpgme_key_t>(key),
                           expires_time, subkey_fpr.toUtf8(), 0);
  }

  return err;
}

/**
 * Generate revoke cert of a key pair
 * @param key target key pair
 * @param outputFileName out file name(path)
 * @return the process doing this job
 */
void GpgKeyOpera::GenerateRevokeCert(const GpgKey& key,
                                     const QString& output_path) {
  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});
  // get all components
  GpgCommandExecutor::ExecuteSync(
      {app_path,
       QStringList{"--command-fd", "0", "--status-fd", "1", "--no-tty", "-o",
                   output_path, "--gen-revoke", key.GetFingerprint()},
       [=](int exit_code, const QString& p_out, const QString& p_err) {
         if (exit_code != 0) {
           qCWarning(core) << "gnupg gen revoke execute error, process stderr: "
                           << p_err << ", process stdout: " << p_out;
         } else {
           qCDebug(core,
                   "gnupg gen revoke exit_code: %d, process stdout size: %lld",
                   exit_code, p_out.size());
         }
       },
       nullptr,
       [](QProcess* proc) -> void {
         // Code From Gpg4Win
         while (proc->canReadLine()) {
           const QString line = QString::fromUtf8(proc->readLine()).trimmed();
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
void GpgKeyOpera::GenerateKey(const std::shared_ptr<GenKeyInfo>& params,
                              const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [&ctx = ctx_, params](const DataObjectPtr& data_object) -> GpgError {
        auto userid = params->GetUserid();
        auto algo = params->GetAlgo() + params->GetKeySizeStr();

        qCDebug(core) << "params: " << params->GetAlgo()
                      << params->GetKeySizeStr();

        unsigned long expires =
            QDateTime::currentDateTime().secsTo(params->GetExpireTime());

        GpgError err;
        unsigned int flags = 0;

        if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
        if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << "key generation args: " << userid << algo << expires
                      << flags;

        err = gpgme_op_createkey(ctx.DefaultContext(), userid.toUtf8(),
                                 algo.toUtf8(), 0, expires, nullptr, flags);

        if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
          data_object->Swap({GpgGenerateKeyResult{
              gpgme_op_genkey_result(ctx.DefaultContext())}});
        } else {
          data_object->Swap({GpgGenerateKeyResult{}});
        }

        return CheckGpgError(err);
      },
      callback, "gpgme_op_createkey", "2.1.0");
}

auto GpgKeyOpera::GenerateKeySync(const std::shared_ptr<GenKeyInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=, &ctx = ctx_](const DataObjectPtr& data_object) -> GpgError {
        auto userid = params->GetUserid();
        auto algo = params->GetAlgo() + params->GetKeySizeStr();

        qCDebug(core) << "params: " << params->GetAlgo()
                      << params->GetKeySizeStr();

        unsigned long expires =
            QDateTime::currentDateTime().secsTo(params->GetExpireTime());

        GpgError err;
        unsigned int flags = 0;

        if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
        if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << "key generation args: " << userid << algo << expires
                      << flags;

        err = gpgme_op_createkey(ctx.DefaultContext(), userid.toUtf8(),
                                 algo.toUtf8(), 0, expires, nullptr, flags);

        if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
          data_object->Swap({GpgGenerateKeyResult{
              gpgme_op_genkey_result(ctx.DefaultContext())}});
        } else {
          data_object->Swap({GpgGenerateKeyResult{}});
        }

        return CheckGpgError(err);
      },
      "gpgme_op_createkey", "2.1.0");
}

void GpgKeyOpera::GenerateSubkey(const GpgKey& key,
                                 const std::shared_ptr<GenKeyInfo>& params,
                                 const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [key, &ctx = ctx_, params](const DataObjectPtr& data_object) -> GpgError {
        if (!params->IsSubKey()) return GPG_ERR_CANCELED;

        qCDebug(core) << "generate subkey algo: " << params->GetAlgo()
                      << "key size: " << params->GetKeySizeStr();

        auto algo = params->GetAlgo() + params->GetKeySizeStr();
        unsigned long expires =
            QDateTime::currentDateTime().secsTo(params->GetExpireTime());
        unsigned int flags = 0;

        if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
        if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << "subkey generation args: " << key.GetId() << algo
                      << expires << flags;

        auto err = gpgme_op_createsubkey(ctx.DefaultContext(),
                                         static_cast<gpgme_key_t>(key),
                                         algo.toUtf8(), 0, expires, flags);
        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          data_object->Swap({GpgGenerateKeyResult{}});
          return err;
        }

        data_object->Swap({GpgGenerateKeyResult{
            gpgme_op_genkey_result(ctx.DefaultContext())}});
        return CheckGpgError(err);
      },
      callback, "gpgme_op_createsubkey", "2.1.13");
}

auto GpgKeyOpera::GenerateSubkeySync(const GpgKey& key,
                                     const std::shared_ptr<GenKeyInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [key, &ctx = ctx_, params](const DataObjectPtr& data_object) -> GpgError {
        if (!params->IsSubKey()) return GPG_ERR_CANCELED;

        qCDebug(core) << "generate subkey algo: " << params->GetAlgo()
                      << " key size: " << params->GetKeySizeStr();

        auto algo = params->GetAlgo() + params->GetKeySizeStr();
        unsigned long expires =
            QDateTime::currentDateTime().secsTo(params->GetExpireTime());
        unsigned int flags = 0;

        if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
        if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << " args: " << key.GetId() << algo << expires << flags;

        auto err = gpgme_op_createsubkey(ctx.DefaultContext(),
                                         static_cast<gpgme_key_t>(key),
                                         algo.toUtf8(), 0, expires, flags);

        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          data_object->Swap({GpgGenerateKeyResult{}});
          return err;
        }

        data_object->Swap({GpgGenerateKeyResult{
            gpgme_op_genkey_result(ctx.DefaultContext())}});
        return CheckGpgError(err);
      },
      "gpgme_op_createsubkey", "2.1.13");
}

void GpgKeyOpera::GenerateKeyWithSubkey(
    const std::shared_ptr<GenKeyInfo>& params,
    const std::shared_ptr<GenKeyInfo>& subkey_params,
    const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [&ctx = ctx_, params,
       subkey_params](const DataObjectPtr& data_object) -> GpgError {
        auto userid = params->GetUserid().toUtf8();
        auto algo = (params->GetAlgo() + params->GetKeySizeStr()).toUtf8();
        unsigned long expires =
            QDateTime::currentDateTime().secsTo(params->GetExpireTime());

        GpgError err;
        unsigned int flags = 0;

        if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
        if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << "key generation args: " << userid << algo << expires
                      << flags;

        err = gpgme_op_createkey(ctx.DefaultContext(), userid, algo, 0, expires,
                                 nullptr, flags);

        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          data_object->Swap({GpgGenerateKeyResult{}});
          return err;
        }

        auto genkey_result =
            GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())};

        if (subkey_params == nullptr || !subkey_params->IsSubKey()) {
          data_object->Swap({genkey_result});
          return err;
        }

        auto key =
            GpgKeyGetter::GetInstance().GetKey(genkey_result.GetFingerprint());
        if (!key.IsGood()) {
          qCWarning(core) << "cannot get key which has been generate, fpr: "
                          << genkey_result.GetFingerprint();
          return err;
        }

        algo = (subkey_params->GetAlgo() + subkey_params->GetKeySizeStr())
                   .toUtf8();
        expires =
            QDateTime::currentDateTime().secsTo(subkey_params->GetExpireTime());

        flags = 0;
        if (subkey_params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (subkey_params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (subkey_params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (subkey_params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (subkey_params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << "subkey generation args: " << key.GetId() << algo
                      << expires << flags;

        err = gpgme_op_createsubkey(ctx.DefaultContext(),
                                    static_cast<gpgme_key_t>(key), algo, 0,
                                    expires, flags);

        if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
          data_object->Swap(
              {genkey_result, GpgGenerateKeyResult{gpgme_op_genkey_result(
                                  ctx.DefaultContext())}});
        } else {
          data_object->Swap({genkey_result, GpgGenerateKeyResult{}});
        }

        return CheckGpgError(err);
      },
      callback, "gpgme_op_createkey&gpgme_op_createsubkey", "2.1.0");
}

auto GpgKeyOpera::GenerateKeyWithSubkeySync(
    const std::shared_ptr<GenKeyInfo>& params,
    const std::shared_ptr<GenKeyInfo>& subkey_params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [&ctx = ctx_, params,
       subkey_params](const DataObjectPtr& data_object) -> GpgError {
        auto userid = params->GetUserid().toUtf8();
        auto algo = (params->GetAlgo() + params->GetKeySizeStr()).toUtf8();
        unsigned long expires =
            QDateTime::currentDateTime().secsTo(params->GetExpireTime());

        GpgError err;
        unsigned int flags = 0;

        if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
        if (params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << "key generation args: " << userid << algo << expires
                      << flags;

        err = gpgme_op_createkey(ctx.DefaultContext(), userid, algo, 0, expires,
                                 nullptr, flags);

        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          data_object->Swap({GpgGenerateKeyResult{}});
          return err;
        }

        auto genkey_result =
            GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())};

        if (subkey_params == nullptr || !subkey_params->IsSubKey()) {
          data_object->Swap({genkey_result});
          return err;
        }

        auto key =
            GpgKeyGetter::GetInstance().GetKey(genkey_result.GetFingerprint());
        if (!key.IsGood()) {
          qCWarning(core) << "cannot get key which has been generate, fpr: "
                          << genkey_result.GetFingerprint();
          return err;
        }

        qCDebug(core) << "try to generate subkey of key: " << key.GetId()
                      << ", algo :" << subkey_params->GetAlgo()
                      << ", key size: " << subkey_params->GetKeySizeStr();

        algo = (subkey_params->GetAlgo() + subkey_params->GetKeySizeStr())
                   .toUtf8();
        expires =
            QDateTime::currentDateTime().secsTo(subkey_params->GetExpireTime());

        flags = 0;
        if (subkey_params->IsAllowEncryption()) flags |= GPGME_CREATE_ENCR;
        if (subkey_params->IsAllowSigning()) flags |= GPGME_CREATE_SIGN;
        if (subkey_params->IsAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
        if (subkey_params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
        if (subkey_params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

        qCDebug(core) << "subkey generation args: " << key.GetId() << algo
                      << expires << flags;

        err = gpgme_op_createsubkey(ctx.DefaultContext(),
                                    static_cast<gpgme_key_t>(key), algo, 0,
                                    expires, flags);

        if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
          data_object->Swap(
              {genkey_result, GpgGenerateKeyResult{gpgme_op_genkey_result(
                                  ctx.DefaultContext())}});
        } else {
          data_object->Swap({genkey_result, GpgGenerateKeyResult{}});
        }

        return CheckGpgError(err);
      },
      "gpgme_op_createkey&gpgme_op_createsubkey", "2.1.0");
}

void GpgKeyOpera::ModifyPassword(const GpgKey& key,
                                 const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [&key, &ctx = ctx_](const DataObjectPtr&) -> GpgError {
        return gpgme_op_passwd(ctx.DefaultContext(),
                               static_cast<gpgme_key_t>(key), 0);
      },
      callback, "gpgme_op_passwd", "2.0.15");
}

auto GpgKeyOpera::ModifyTOFUPolicy(
    const GpgKey& key, gpgme_tofu_policy_t tofu_policy) -> GpgError {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"2.0.0"});
  qCDebug(core) << "got gnupg version from rt: " << gnupg_version;

  if (GFCompareSoftwareVersion(gnupg_version, "2.1.10") < 0) {
    qCWarning(core, "operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }

  auto err = gpgme_op_tofu_policy(ctx_.DefaultContext(),
                                  static_cast<gpgme_key_t>(key), tofu_policy);
  return CheckGpgError(err);
}

void GpgKeyOpera::DeleteKey(const KeyId& key_id) {
  auto keys = std::make_unique<KeyIdArgsList>();
  keys->push_back(key_id);
  DeleteKeys(std::move(keys));
}
}  // namespace GpgFrontend
