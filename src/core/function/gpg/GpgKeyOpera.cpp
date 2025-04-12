/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "core/GpgModel.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
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
void GpgKeyOpera::DeleteKeys(KeyIdArgsList key_ids) {
  GpgError err;
  for (const auto& tmp : key_ids) {
    auto key = GpgKeyGetter::GetInstance(GetChannel()).GetKey(tmp);
    if (key.IsGood()) {
      err = CheckGpgError(gpgme_op_delete_ext(
          ctx_.DefaultContext(), static_cast<gpgme_key_t>(key),
          GPGME_DELETE_ALLOW_SECRET | GPGME_DELETE_FORCE));
      assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
    } else {
      LOG_W() << "GpgKeyOpera DeleteKeys get key failed: " << tmp;
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
    assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    err =
        gpgme_op_setexpire(ctx_.DefaultContext(), static_cast<gpgme_key_t>(key),
                           expires_time, subkey_fpr.toUtf8(), 0);
    assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
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
                                     const QString& output_path,
                                     int revocation_reason_code,
                                     const QString& revocation_reason_text) {
  LOG_D() << "revoke code:" << revocation_reason_code
          << "text:" << revocation_reason_text;

  // dealing with reason text
  auto reason_text_lines = GpgFrontend::SecureCreateSharedObject<QStringList>(
      revocation_reason_text.split('\n', Qt::SkipEmptyParts));

  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});
  // get all components
  GpgCommandExecutor::ExecuteSync(
      {app_path,
       QStringList{"--command-fd", "0", "--status-fd", "1", "--no-tty", "-o",
                   output_path, "--gen-revoke", key.GetFingerprint()},
       [=](int exit_code, const QString& p_out, const QString& p_err) {
         if (exit_code != 0) {
           LOG_W() << "gnupg gen revoke execute error, process stderr: "
                   << p_err << ", process stdout: " << p_out;
           return;
         }
         LOG_D() << "gnupg gen revoke exit_code: " << exit_code
                 << "process stdout size: " << p_out.size();
       },
       nullptr,
       [revocation_reason_code, reason_text_lines](QProcess* proc) -> void {
         // Code From Gpg4Win
         while (proc->canReadLine()) {
           const QString line = QString::fromUtf8(proc->readLine()).trimmed();
           LOG_D() << "gpg revoke proc line:" << line;

           if (line == QLatin1String("[GNUPG:] GET_BOOL gen_revoke.okay")) {
             proc->write("y\n");
           } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                            "ask_revocation_reason.code")) {
             proc->write(
                 QString("%1%\n").arg(revocation_reason_code).toLatin1());
           } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                            "ask_revocation_reason.text")) {
             if (!reason_text_lines->isEmpty()) {
               proc->write(reason_text_lines->takeFirst().toUtf8());
             }
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

auto GenerateKeyImpl(GpgContext& ctx,
                     const QSharedPointer<KeyGenerateInfo>& params,
                     const DataObjectPtr& data_object) -> GpgError {
  if (params == nullptr || params->GetAlgo() == KeyGenerateInfo::kNoneAlgo ||
      params->IsSubKey()) {
    return GPG_ERR_CANCELED;
  }

  const auto userid = params->GetUserid();
  const auto algo = params->GetAlgo().Id();

  unsigned long expires =
      QDateTime::currentDateTime().secsTo(params->GetExpireTime());

  GpgError err;
  unsigned int flags = 0;

  if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
  if (params->IsAllowEncr()) flags |= GPGME_CREATE_ENCR;
  if (params->IsAllowSign()) flags |= GPGME_CREATE_SIGN;
  if (params->IsAllowAuth()) flags |= GPGME_CREATE_AUTH;
  if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
  if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

  LOG_D() << "key generation args: " << userid << algo << expires << flags;
  LOG_D() << "key generation flags" << params->IsAllowEncr()
          << params->IsAllowSign() << params->IsAllowAuth()
          << !params->IsSubKey();

  err = gpgme_op_createkey(ctx.DefaultContext(), userid.toUtf8(), algo.toUtf8(),
                           0, expires, nullptr, flags);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    data_object->Swap(
        {GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())}});
  } else {
    data_object->Swap({GpgGenerateKeyResult{}});
  }

  return CheckGpgError(err);
}

/**
 * Generate a new key pair
 * @param params key generation args
 * @return error information
 */
void GpgKeyOpera::GenerateKey(const QSharedPointer<KeyGenerateInfo>& params,
                              const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return GenerateKeyImpl(ctx_, params, data_object);
      },
      callback, "gpgme_op_createkey", "2.1.0");
}

auto GpgKeyOpera::GenerateKeySync(const QSharedPointer<KeyGenerateInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return GenerateKeyImpl(ctx_, params, data_object);
      },
      "gpgme_op_createkey", "2.1.0");
}

auto GenerateSubKeyImpl(GpgContext& ctx, const GpgKey& key,
                        const QSharedPointer<KeyGenerateInfo>& params,
                        const DataObjectPtr& data_object) -> GpgError {
  if (params == nullptr || params->GetAlgo() == KeyGenerateInfo::kNoneAlgo ||
      !params->IsSubKey()) {
    return GPG_ERR_CANCELED;
  }

  auto algo = params->GetAlgo().Id();
  unsigned long expires =
      QDateTime::currentDateTime().secsTo(params->GetExpireTime());
  unsigned int flags = 0;

  if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
  if (params->IsAllowEncr()) flags |= GPGME_CREATE_ENCR;
  if (params->IsAllowSign()) flags |= GPGME_CREATE_SIGN;
  if (params->IsAllowAuth()) flags |= GPGME_CREATE_AUTH;
  if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
  if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

  LOG_D() << "subkey generation args: " << key.GetId() << algo << expires
          << flags;

  auto err =
      gpgme_op_createsubkey(ctx.DefaultContext(), static_cast<gpgme_key_t>(key),
                            algo.toLatin1(), 0, expires, flags);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgGenerateKeyResult{}});
    return err;
  }

  data_object->Swap(
      {GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())}});
  return CheckGpgError(err);
}

void GpgKeyOpera::GenerateSubkey(const GpgKey& key,
                                 const QSharedPointer<KeyGenerateInfo>& params,
                                 const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return GenerateSubKeyImpl(ctx_, key, params, data_object);
      },
      callback, "gpgme_op_createsubkey", "2.1.13");
}

auto GpgKeyOpera::GenerateSubkeySync(
    const GpgKey& key, const QSharedPointer<KeyGenerateInfo>& params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return GenerateSubKeyImpl(ctx_, key, params, data_object);
      },
      "gpgme_op_createsubkey", "2.1.13");
}

auto GenerateKeyWithSubkeyImpl(GpgContext& ctx, GpgKeyGetter& key_getter,
                               const QSharedPointer<KeyGenerateInfo>& p_params,
                               const QSharedPointer<KeyGenerateInfo>& s_params,
                               const DataObjectPtr& data_object) -> GpgError {
  auto err = GenerateKeyImpl(ctx, p_params, data_object);

  if (err != GPG_ERR_NO_ERROR) return err;
  if (!data_object->Check<GpgGenerateKeyResult>()) return GPG_ERR_CANCELED;

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  auto key = key_getter.GetKey(result.GetFingerprint());
  if (!key.IsGood()) {
    LOG_W() << "cannot get key which has been generated, fpr: "
            << result.GetFingerprint();
    return GPG_ERR_CANCELED;
  }

  if (s_params == nullptr) return err;

  data_object->Swap({});
  err = GenerateSubKeyImpl(ctx, key, s_params, data_object);
  auto s_result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);

  data_object->Swap({result, s_result});
  return err;
}

void GpgKeyOpera::GenerateKeyWithSubkey(
    const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params,
    const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return GenerateKeyWithSubkeyImpl(ctx_, key_getter_, p_params, s_params,
                                         data_object);
      },
      callback, "gpgme_op_createkey&gpgme_op_createsubkey", "2.1.0");
}

auto GpgKeyOpera::GenerateKeyWithSubkeySync(
    const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return GenerateKeyWithSubkeyImpl(ctx_, key_getter_, p_params, s_params,
                                         data_object);
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
  LOG_D() << "got gnupg version from rt: " << gnupg_version;

  if (GFCompareSoftwareVersion(gnupg_version, "2.1.10") < 0) {
    FLOG_W("operator not support");
    return GPG_ERR_NOT_SUPPORTED;
  }

  auto err = gpgme_op_tofu_policy(ctx_.DefaultContext(),
                                  static_cast<gpgme_key_t>(key), tofu_policy);
  return CheckGpgError(err);
}

void GpgKeyOpera::DeleteKey(const KeyId& key_id) {
  auto keys = KeyIdArgsList{};
  keys.push_back(key_id);
  DeleteKeys(keys);
}

auto AddADSKImpl(GpgContext& ctx, const GpgKey& key, const GpgSubKey& adsk,
                 const DataObjectPtr& data_object) -> GpgError {
  auto algo = adsk.GetFingerprint();
  unsigned int flags = GPGME_CREATE_ADSK;

  LOG_D() << "add adsk args: " << key.GetId() << algo;

  auto err =
      gpgme_op_createsubkey(ctx.DefaultContext(), static_cast<gpgme_key_t>(key),
                            algo.toLatin1(), 0, 0, flags);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgGenerateKeyResult{}});
    return err;
  }

  data_object->Swap(
      {GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())}});
  return CheckGpgError(err);
}

void GpgKeyOpera::AddADSK(const GpgKey& key, const GpgSubKey& adsk,
                          const GpgOperationCallback& callback) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return AddADSKImpl(ctx_, key, adsk, data_object);
      },
      callback, "gpgme_op_createsubkey", "2.4.1");
}

auto GpgKeyOpera::AddADSKSync(const GpgKey& key, const GpgSubKey& adsk)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return AddADSKImpl(ctx_, key, adsk, data_object);
      },
      "gpgme_op_createsubkey", "2.4.1");
}
}  // namespace GpgFrontend
