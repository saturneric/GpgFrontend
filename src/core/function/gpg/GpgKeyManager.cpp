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

#include "GpgKeyManager.h"

#include "core/function/gpg/GpgAutomatonHandler.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/GpgData.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyManager::GpgKeyManager(int channel)
    : SingletonFunctionObject<GpgKeyManager>(channel) {}

auto GpgKeyManager::SignKey(const GpgKeyPtr& key,
                            const GpgAbstractKeyPtrList& keys,
                            const QString& uid,
                            const std::unique_ptr<QDateTime>& expires) -> bool {
  GpgBasicOperator::GetInstance(GetChannel()).SetSigners(keys, true);

  unsigned int flags = 0;
  unsigned int expires_time_t = 0;

  if (expires == nullptr) {
    flags |= GPGME_KEYSIGN_NOEXPIRE;
  } else {
    expires_time_t = expires->toSecsSinceEpoch();
  }

  auto err = CheckGpgError(
      gpgme_op_keysign(ctx_.DefaultContext(), static_cast<gpgme_key_t>(*key),
                       uid.toUtf8(), expires_time_t, flags));

  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgKeyManager::RevSign(const GpgKeyPtr& key,
                            const SignIdArgsList& signature_id) -> bool {
  auto& key_getter = GpgKeyGetter::GetInstance(GetChannel());

  for (const auto& sign_id : signature_id) {
    auto signing_key = key_getter.GetKey(sign_id.first);
    assert(signing_key.IsGood());

    auto err = CheckGpgError(
        gpgme_op_revsig(ctx_.DefaultContext(), gpgme_key_t(*key),
                        gpgme_key_t(signing_key), sign_id.second.toUtf8(), 0));
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return false;
  }
  return true;
}

auto GpgKeyManager::SetExpire(const GpgKeyPtr& key,
                              std::unique_ptr<GpgSubKey>& subkey,
                              std::unique_ptr<QDateTime>& expires) -> bool {
  unsigned long expires_time = 0;

  if (expires != nullptr) expires_time = expires->toSecsSinceEpoch();

  const char* sub_fprs = nullptr;

  if (subkey != nullptr) sub_fprs = subkey->Fingerprint().toUtf8();

  auto err = CheckGpgError(gpgme_op_setexpire(ctx_.DefaultContext(),
                                              static_cast<gpgme_key_t>(*key),
                                              expires_time, sub_fprs, 0));

  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgKeyManager::SetOwnerTrustLevel(const GpgKeyPtr& key,
                                       int trust_level) -> bool {
  if (trust_level < 1 || trust_level > 5) {
    FLOG_W("illegal owner trust level: %d", trust_level);
  }

  GpgAutomatonHandler::AutomatonNextStateHandler next_state_handler =
      [](AutomatonState state, QString status, QString args) {
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
            return QString("");
          default:
            return QString("");
        }
        return QString("");
      };

  return GpgAutomatonHandler::GetInstance(GetChannel())
      .DoInteract(key, next_state_handler, action_handler);
}

auto GpgKeyManager::DeleteSubkey(const GpgKeyPtr& key,
                                 int subkey_index) -> bool {
  if (subkey_index < 0 ||
      subkey_index >= static_cast<int>(key->SubKeys().size())) {
    LOG_W() << "illegal subkey index: " << subkey_index;
    return false;
  }

  AutomatonNextStateHandler next_state_handler =
      [](AutomatonState state, QString status, QString args) {
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

  return GpgAutomatonHandler::GetInstance(GetChannel())
      .DoInteract(key, next_state_handler, action_handler);
}

auto GpgKeyManager::RevokeSubkey(const GpgKeyPtr& key, int subkey_index,
                                 int reason_code,
                                 const QString& reason_text) -> bool {
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
      [](AutomatonState state, QString status, QString args) {
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

  return GpgAutomatonHandler::GetInstance(GetChannel())
      .DoInteract(key, next_state_handler, action_handler);
}

}  // namespace GpgFrontend