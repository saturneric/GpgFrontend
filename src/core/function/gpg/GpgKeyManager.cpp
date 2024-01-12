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

#include "GpgKeyManager.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/GpgUtils.h"

GpgFrontend::GpgKeyManager::GpgKeyManager(int channel)
    : SingletonFunctionObject<GpgKeyManager>(channel) {}

auto GpgFrontend::GpgKeyManager::SignKey(
    const GpgFrontend::GpgKey& target, GpgFrontend::KeyArgsList& keys,
    const QString& uid, const std::unique_ptr<QDateTime>& expires) -> bool {
  GpgBasicOperator::GetInstance().SetSigners(keys, true);

  unsigned int flags = 0;
  unsigned int expires_time_t = 0;

  if (expires == nullptr) {
    flags |= GPGME_KEYSIGN_NOEXPIRE;
  } else {
    expires_time_t = expires->toSecsSinceEpoch();
  }

  auto err = CheckGpgError(
      gpgme_op_keysign(ctx_.DefaultContext(), static_cast<gpgme_key_t>(target),
                       uid.toUtf8(), expires_time_t, flags));

  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgFrontend::GpgKeyManager::RevSign(
    const GpgFrontend::GpgKey& key,
    const GpgFrontend::SignIdArgsListPtr& signature_id) -> bool {
  auto& key_getter = GpgKeyGetter::GetInstance();

  for (const auto& sign_id : *signature_id) {
    auto signing_key = key_getter.GetKey(sign_id.first);
    assert(signing_key.IsGood());
    auto err = CheckGpgError(
        gpgme_op_revsig(ctx_.DefaultContext(), gpgme_key_t(key),
                        gpgme_key_t(signing_key), sign_id.second.toUtf8(), 0));
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return false;
  }
  return true;
}

auto GpgFrontend::GpgKeyManager::SetExpire(const GpgFrontend::GpgKey& key,
                                           std::unique_ptr<GpgSubKey>& subkey,
                                           std::unique_ptr<QDateTime>& expires)
    -> bool {
  unsigned long expires_time = 0;

  if (expires != nullptr) expires_time = expires->toSecsSinceEpoch();

  const char* sub_fprs = nullptr;

  if (subkey != nullptr) sub_fprs = subkey->GetFingerprint().toUtf8();

  auto err = CheckGpgError(gpgme_op_setexpire(ctx_.DefaultContext(),
                                              static_cast<gpgme_key_t>(key),
                                              expires_time, sub_fprs, 0));

  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgFrontend::GpgKeyManager::SetOwnerTrustLevel(const GpgKey& key,
                                                    int trust_level) -> bool {
  if (trust_level < 0 || trust_level > 5) {
    GF_CORE_LOG_ERROR("illegal owner trust level: {}", trust_level);
  }

  AutomatonNextStateHandler next_state_handler = [](AutomatonState state,
                                                    QString status,
                                                    QString args) {
    GF_CORE_LOG_DEBUG("next_state_handler state: {}, gpg_status: {}, args: {}",
                      state, status, args);
    auto tokens = args.split(' ');

    switch (state) {
      case AS_START:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return AS_COMMAND;
        }
        return AS_ERROR;
      case AS_COMMAND:
        if (status == "GET_LINE" && args == "edit_ownertrust.value") {
          return AS_VALUE;
        }
        return AS_ERROR;
      case AS_VALUE:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return AS_QUIT;
        } else if (status == "GET_BOOL" &&
                   args == "edit_ownertrust.set_ultimate.okay") {
          return AS_REALLY_ULTIMATE;
        }
        return AS_ERROR;
      case AS_REALLY_ULTIMATE:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return AS_QUIT;
        }
        return AS_ERROR;
      case AS_QUIT:
        if (status == "GET_LINE" && args == "keyedit.save.okay") {
          return AS_SAVE;
        }
        return AS_ERROR;
      case AS_ERROR:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return AS_QUIT;
        }
        return AS_ERROR;
      default:
        return AS_ERROR;
    };
  };

  AutomatonActionHandler action_handler =
      [trust_level](AutomatonHandelStruct& handler, AutomatonState state) {
        GF_CORE_LOG_DEBUG("action_handler state: {}", state);
        switch (state) {
          case AS_COMMAND:
            return QString("trust");
          case AS_VALUE:
            handler.SetSuccess(true);
            return QString::number(trust_level);
          case AS_REALLY_ULTIMATE:
            handler.SetSuccess(true);
            return QString("Y");
          case AS_QUIT:
            return QString("quit");
          case AS_SAVE:
            handler.SetSuccess(true);
            return QString("Y");
          case AS_START:
          case AS_ERROR:
            return QString("");
          default:
            return QString("");
        }
        return QString("");
      };

  auto key_fpr = key.GetFingerprint();
  AutomatonHandelStruct handel_struct(key_fpr);
  handel_struct.SetHandler(next_state_handler, action_handler);

  GpgData data_out;

  auto err = gpgme_op_interact(
      ctx_.DefaultContext(), static_cast<gpgme_key_t>(key), 0,
      GpgKeyManager::interactor_cb_fnc, (void*)&handel_struct, data_out);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR && handel_struct.Success();
}

auto GpgFrontend::GpgKeyManager::interactor_cb_fnc(void* handle,
                                                   const char* status,
                                                   const char* args, int fd)
    -> gpgme_error_t {
  auto* handle_struct = static_cast<AutomatonHandelStruct*>(handle);
  QString status_s = status;
  QString args_s = args;
  GF_CORE_LOG_DEBUG(
      "cb start status: {}, args: {}, fd: {}, handle struct state: {}",
      status_s, args_s, fd, handle_struct->CuurentStatus());

  if (status_s == "KEY_CONSIDERED") {
    auto tokens = QString(args).split(' ');

    if (tokens.empty() || tokens[0] != handle_struct->KeyFpr()) {
      GF_CORE_LOG_ERROR("handle struct key fpr {} mismatch token: {}, exit...",
                        handle_struct->KeyFpr(), tokens[0]);
      return -1;
    }

    return 0;
  }

  if (status_s == "GOT_IT" || status_s.isEmpty()) {
    GF_CORE_LOG_DEBUG("status GOT_IT, continue...");
    return 0;
  }

  AutomatonState next_state = handle_struct->NextState(status_s, args_s);
  if (next_state == AS_ERROR) {
    GF_CORE_LOG_DEBUG("handle struct next state caught error, skipping...");
    return GPG_ERR_FALSE;
  }

  if (next_state == AS_SAVE) {
    handle_struct->SetSuccess(true);
  }

  // set state and preform action
  handle_struct->SetStatus(next_state);
  Command cmd = handle_struct->Action();
  GF_CORE_LOG_DEBUG("handle struct action done, next state: {}, action cmd: {}",
                    next_state, cmd);
  if (!cmd.isEmpty()) {
    auto btye_array = cmd.toUtf8();
    gpgme_io_write(fd, btye_array, btye_array.size());
    gpgme_io_write(fd, "\n", 1);
  } else if (status_s == "GET_LINE") {
    // avoid trapping in this state
    return GPG_ERR_FALSE;
  }

  return 0;
}