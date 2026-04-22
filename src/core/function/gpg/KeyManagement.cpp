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

#include "core/function/gpg/GpgAutomatonHandler.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyGroupGetter.h"
#include "core/function/gpg/MessageCryptoOperation.h"
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

auto DeleteSubKeyGnuPGImpl(GpgContext& ctx, const GpgKeyPtr& key,
                           int subkey_index) -> bool {
  auto& ah = GpgAutomatonHandler::GetInstance(ctx.GetChannel());

  if (subkey_index < 0 ||
      subkey_index >= static_cast<int>(key->SubKeys().size())) {
    LOG_W() << "illegal subkey index: " << subkey_index;
    return false;
  }

  AutomatonNextStateHandler next_state_handler =
      [](AutomatonState state, const QString& status, const QString& args)
      -> GpgFrontend::GpgAutomatonHandler::AutomatonState {
    auto tokens = args.split(' ');

    switch (state) {
      case GpgAutomatonHandler::kAS_START:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::kAS_SELECT;
        }
        return GpgAutomatonHandler::kAS_ERROR;
      case GpgAutomatonHandler::kAS_SELECT:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::kAS_COMMAND;
        }
        return GpgAutomatonHandler::kAS_ERROR;
      case GpgAutomatonHandler::kAS_COMMAND:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::kAS_QUIT;
        } else if (status == "GET_BOOL" &&
                   args == "keyedit.remove.subkey.okay") {
          return GpgAutomatonHandler::kAS_REALLY_ULTIMATE;
        }
        return GpgAutomatonHandler::kAS_ERROR;
      case GpgAutomatonHandler::kAS_REALLY_ULTIMATE:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::kAS_QUIT;
        }
        return GpgAutomatonHandler::kAS_ERROR;
      case GpgAutomatonHandler::kAS_QUIT:
        if (status == "GET_BOOL" && args == "keyedit.save.okay") {
          return GpgAutomatonHandler::kAS_SAVE;
        }
        return GpgAutomatonHandler::kAS_ERROR;
      case GpgAutomatonHandler::kAS_ERROR:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::kAS_QUIT;
        }
        return GpgAutomatonHandler::kAS_ERROR;
      default:
        return GpgAutomatonHandler::kAS_ERROR;
    };
  };

  AutomatonActionHandler action_handler =
      [subkey_index](AutomatonHandelStruct& handler, AutomatonState state) {
        switch (state) {
          case GpgAutomatonHandler::kAS_SELECT:
            return QString("key %1").arg(subkey_index);
          case GpgAutomatonHandler::kAS_COMMAND:
            return QString("delkey");
          case GpgAutomatonHandler::kAS_REALLY_ULTIMATE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::kAS_QUIT:
            return QString("quit");
          case GpgAutomatonHandler::kAS_SAVE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::kAS_START:
          case GpgAutomatonHandler::kAS_ERROR:
          default:
            return QString("");
        }
        return QString("");
      };

  auto [err, succ] = ah.DoInteract(key, next_state_handler, action_handler);
  return err == GPG_ERR_NO_ERROR && succ;
}

auto RevokeSubKeyGnuPGImpl(GpgContext& ctx, const GpgKeyPtr& key, int subkey_index,
                      int reason_code, const QString& reason_text) -> bool {
  auto& ah = GpgAutomatonHandler::GetInstance(ctx.GetChannel());

  if (subkey_index < 0 ||
      subkey_index >= static_cast<int>(key->SubKeys().size())) {
    LOG_W() << "illegal subkey index: " << subkey_index;
    return false;
  }

  if (reason_code < 0 || reason_code > 3) {
    LOG_W() << "illegal reason code: " << reason_code;
    return false;
  }

  // dealing with reason text
  auto reason_text_lines = SecureCreateSharedObject<QStringList>(
      reason_text.split('\n', Qt::SkipEmptyParts));

  AutomatonNextStateHandler next_state_handler =
      [](AutomatonState state, const QString& status, const QString& args) {
        auto tokens = args.split(' ');

        switch (state) {
          case GpgAutomatonHandler::kAS_START:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_SELECT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_SELECT:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_COMMAND:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            } else if (status == "GET_BOOL" &&
                       args == "keyedit.revoke.subkey.okay") {
              return GpgAutomatonHandler::kAS_REALLY_ULTIMATE;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_REASON_CODE:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            } else if (status == "GET_LINE" &&
                       args == "ask_revocation_reason.text") {
              return GpgAutomatonHandler::kAS_REASON_TEXT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_REASON_TEXT:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            } else if (status == "GET_LINE" &&
                       args == "ask_revocation_reason.text") {
              return GpgAutomatonHandler::kAS_REASON_TEXT;
            } else if (status == "GET_BOOL" &&
                       args == "ask_revocation_reason.okay") {
              return GpgAutomatonHandler::kAS_REALLY_ULTIMATE;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_REALLY_ULTIMATE:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            } else if (status == "GET_LINE" &&
                       args == "ask_revocation_reason.code") {
              return GpgAutomatonHandler::kAS_REASON_CODE;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_QUIT:
            if (status == "GET_BOOL" && args == "keyedit.save.okay") {
              return GpgAutomatonHandler::kAS_SAVE;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_ERROR:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          default:
            return GpgAutomatonHandler::kAS_ERROR;
        };
      };

  AutomatonActionHandler action_handler =
      [subkey_index, reason_code, reason_text_lines](
          AutomatonHandelStruct& handler, AutomatonState state) {
        switch (state) {
          case GpgAutomatonHandler::kAS_SELECT:
            return QString("key %1").arg(subkey_index);
          case GpgAutomatonHandler::kAS_COMMAND:
            return QString("revkey");
          case GpgAutomatonHandler::kAS_REASON_CODE:
            return QString::number(reason_code);
          case GpgAutomatonHandler::kAS_REASON_TEXT:
            return reason_text_lines->isEmpty()
                       ? QString("")
                       : QString(reason_text_lines->takeFirst().toUtf8());
          case GpgAutomatonHandler::kAS_REALLY_ULTIMATE:
            return QString("Y");
          case GpgAutomatonHandler::kAS_QUIT:
            return QString("quit");
          case GpgAutomatonHandler::kAS_SAVE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::kAS_START:
          case GpgAutomatonHandler::kAS_ERROR:
          default:
            return QString("");
        }
        return QString("");
      };

  auto [err, succ] = ah.DoInteract(key, next_state_handler, action_handler);
  return err == GPG_ERR_NO_ERROR && succ;
}

auto SetOwnerTrustLevelGnuPGImpl(GpgContext& ctx, const GpgAbstractKeyPtr& key,
                                 int trust_level) -> bool {
  auto& ah = GpgAutomatonHandler::GetInstance(ctx.GetChannel());

  if (trust_level < 1 || trust_level > 5) {
    FLOG_W("illegal owner trust level: %d", trust_level);
  }

  GpgAutomatonHandler::AutomatonNextStateHandler next_state_handler =
      [](AutomatonState state, const QString& status, const QString& args) {
        auto tokens = args.split(' ');

        switch (state) {
          case GpgAutomatonHandler::kAS_START:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_COMMAND:
            if (status == "GET_LINE" && args == "edit_ownertrust.value") {
              return GpgAutomatonHandler::kAS_VALUE;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_VALUE:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            } else if (status == "GET_BOOL" &&
                       args == "edit_ownertrust.set_ultimate.okay") {
              return GpgAutomatonHandler::kAS_REALLY_ULTIMATE;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_REALLY_ULTIMATE:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_QUIT:
            if (status == "GET_BOOL" && args == "keyedit.save.okay") {
              return GpgAutomatonHandler::kAS_SAVE;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_ERROR:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          default:
            return GpgAutomatonHandler::kAS_ERROR;
        };
      };

  AutomatonActionHandler action_handler =
      [trust_level](AutomatonHandelStruct& handler, AutomatonState state) {
        switch (state) {
          case GpgAutomatonHandler::kAS_COMMAND:
            return QString("trust");
          case GpgAutomatonHandler::kAS_VALUE:
            handler.SetSuccess(true);
            return QString::number(trust_level);
          case GpgAutomatonHandler::kAS_REALLY_ULTIMATE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::kAS_QUIT:
            return QString("quit");
          case GpgAutomatonHandler::kAS_SAVE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::kAS_START:
          case GpgAutomatonHandler::kAS_ERROR:
          default:
            return QString("");
        }
        return QString("");
      };

  auto gpg_keys = ConvertKey2GpgKeyList(ctx.GetChannel(), {key});

  bool all_succ = true;
  for (const auto& gpg_key : gpg_keys) {
    LOG_D() << "AAAA: " << gpg_key->Fingerprint();
    auto [err, succ] =
        ah.DoInteract(gpg_key, next_state_handler, action_handler);

    all_succ = all_succ && err == GPG_ERR_NO_ERROR && succ;
  }

  return all_succ;
}

auto RevKeySignatureGnuPGImpl(GpgContext& ctx, const GpgKeyPtr& key,
                              const SignIdArgsList& signature_id) -> bool {
  auto& key_getter = GpgKeyGetter::GetInstance(ctx.GetChannel());

  for (const auto& sign_id : signature_id) {
    auto signing_key = key_getter.GetKey(sign_id.first);
    assert(signing_key.IsGood());

    auto err = CheckGpgError(gpgme_op_revsig(
        ctx.DefaultContext(), static_cast<gpgme_key_t>(*key),
        static_cast<gpgme_key_t>(signing_key), sign_id.second.toUtf8(), 0));
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return false;
  }
  return true;
}

auto SignKeyGnuPGImpl(GpgContext& ctx, const GpgKeyPtr& key,
                      const GpgAbstractKeyPtrList& keys, const QString& uid,
                      const std::optional<QDateTime>& expires) -> bool {
  SetSignersGnuPGImpl(ctx, keys, true);

  unsigned int flags = 0;
  unsigned int expires_time_t = 0;

  if (expires == std::nullopt) {
    flags |= GPGME_KEYSIGN_NOEXPIRE;
  } else {
    expires_time_t = expires->toSecsSinceEpoch();
  }

  auto err = CheckGpgError(
      gpgme_op_keysign(ctx.DefaultContext(), static_cast<gpgme_key_t>(*key),
                       uid.toUtf8(), expires_time_t, flags));

  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

}  // namespace GpgFrontend
