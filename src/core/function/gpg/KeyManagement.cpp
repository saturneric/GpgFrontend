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

#include "KeyManagement.h"

#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgKeyGroupGetter.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto DeleteKeysGnuPGImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys)
    -> bool {
  for (const auto& key : keys) {
    if (key->KeyType() == GpgAbstractKeyType::kGPG_KEY && key->IsGood()) {
      if (key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP) {
        GpgKeyGroupGetter::GetInstance(ctx.GetChannel()).Remove(key->ID());
        continue;
      }

      auto k = qSharedPointerDynamicCast<GpgKey>(key);
      auto err = CheckGpgError(gpgme_op_delete_ext(
          ctx.DefaultContext(), static_cast<gpgme_key_t>(*k),
          GPGME_DELETE_ALLOW_SECRET | GPGME_DELETE_FORCE));
      if (err != GPG_ERR_NO_ERROR) {
        LOG_W() << "Failed to delete key: " << key->ID()
                << ", error code: " << CheckGpgError(err);
        continue;
      }
    }
  }
  return true;
}

auto SetExpireGnuPGImpl(GpgContext& ctx, const GpgKeyPtr& key,
                        const SubkeyId& skey_fpr,
                        const std::optional<QDateTime>& expires) -> GpgError {
  unsigned long expires_time = 0;

  if (expires.has_value()) {
    expires_time = QDateTime::currentDateTime().secsTo(expires.value());
  }

  GpgError err;
  if (key->Fingerprint() == skey_fpr || skey_fpr.isEmpty()) {
    err =
        gpgme_op_setexpire(ctx.DefaultContext(), static_cast<gpgme_key_t>(*key),
                           expires_time, nullptr, 0);
    assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    err =
        gpgme_op_setexpire(ctx.DefaultContext(), static_cast<gpgme_key_t>(*key),
                           expires_time, skey_fpr.toUtf8(), 0);
    assert(gpg_err_code(err) == GPG_ERR_NO_ERROR);
  }

  return err;
}

auto GenerateRevCertGnuPGImpl(GpgContext& ctx_, const GpgKeyPtr& key,
                              const QString& output_path, int reason_code,
                              const QString& reason_text) -> bool {
  // dealing with reason text
  auto reason_text_lines = GpgFrontend::SecureCreateSharedObject<QStringList>(
      reason_text.split('\n', Qt::SkipEmptyParts));

  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});
  // get all components
  GpgCommandExecutor::ExecuteSync(
      {app_path,
       QStringList{"--homedir", ctx_.HomeDirectory(), "--command-fd", "0",
                   "--status-fd", "1", "--no-tty", "-o", output_path,
                   "--gen-revoke", key->Fingerprint()},
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
       [reason_code, reason_text_lines](QProcess* proc) -> void {
         // Code From Gpg4Win
         while (proc->canReadLine()) {
           const QString line = QString::fromUtf8(proc->readLine()).trimmed();
           LOG_D() << "gpg revoke proc line:" << line;

           if (line == QLatin1String("[GNUPG:] GET_BOOL gen_revoke.okay")) {
             proc->write("y\n");
           } else if (line == QLatin1String("[GNUPG:] GET_LINE "
                                            "ask_revocation_reason.code")) {
             proc->write(QString("%1%\n").arg(reason_code).toLatin1());
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

  return true;
}

auto ModifyKeyPassphraseGnuPGImpl(GpgContext& ctx, const GpgKeyPtr& key,
                                  const DataObjectPtr& data_object)
    -> GpgError {
  return gpgme_op_passwd(ctx.DefaultContext(), static_cast<gpgme_key_t>(*key),
                         0);
}

auto AddADSKGnuPGIImpl(GpgContext& ctx, const GpgKeyPtr& key,
                       const GpgSubKey& adsk, const DataObjectPtr& data_object)
    -> GpgError {
  auto algo = adsk.Fingerprint();
  unsigned int flags = GPGME_CREATE_ADSK;

  LOG_D() << "add adsk args: " << key->ID() << algo;

  auto err = gpgme_op_createsubkey(ctx.DefaultContext(),
                                   static_cast<gpgme_key_t>(*key),
                                   algo.toLatin1(), 0, 0, flags);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgGenerateKeyResult{}});
    return err;
  }

  data_object->Swap(
      {GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())}});
  return CheckGpgError(err);
}

}  // namespace GpgFrontend
