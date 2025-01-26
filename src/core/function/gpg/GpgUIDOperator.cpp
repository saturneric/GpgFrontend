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

#include "GpgUIDOperator.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgAutomatonHandler.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgUIDOperator::GpgUIDOperator(int channel)
    : SingletonFunctionObject<GpgUIDOperator>(channel) {}

auto GpgUIDOperator::AddUID(const GpgKey& key, const QString& uid) -> bool {
  auto err = gpgme_op_adduid(ctx_.DefaultContext(),
                             static_cast<gpgme_key_t>(key), uid.toUtf8(), 0);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgUIDOperator::SetPrimaryUID(const GpgKey& key,
                                   const QString& uid) -> bool {
  auto err = CheckGpgError(gpgme_op_set_uid_flag(
      ctx_.DefaultContext(), static_cast<gpgme_key_t>(key), uid.toUtf8(),
      "primary", nullptr));
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgUIDOperator::AddUID(const GpgKey& key, const QString& name,
                            const QString& comment,
                            const QString& email) -> bool {
  LOG_D() << "new uuid:" << name << comment << email;
  return AddUID(key, QString("%1(%2)<%3>").arg(name).arg(comment).arg(email));
}

auto GpgUIDOperator::DeleteUID(const GpgKey& key, int uid_index) -> bool {
  if (uid_index < 2 || uid_index > static_cast<int>(key.GetUIDs()->size())) {
    LOG_W() << "illegal uid_index index: " << uid_index;
    return false;
  }

  AutomatonNextStateHandler next_state_handler = [](AutomatonState state,
                                                    QString status,
                                                    QString args) {
    auto tokens = args.split(' ');

    switch (state) {
      case GpgAutomatonHandler::AS_START:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_SELECT;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_SELECT:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_COMMAND;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_COMMAND:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        } else if (status == "GET_BOOL" && args == "keyedit.remove.uid.okay") {
          return GpgAutomatonHandler::AS_REALLY_ULTIMATE;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_REALLY_ULTIMATE:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_QUIT:
        if (status == "GET_BOOL" && args == "keyedit.save.okay") {
          return GpgAutomatonHandler::AS_SAVE;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_ERROR:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        }
        return GpgAutomatonHandler::AS_ERROR;
      default:
        return GpgAutomatonHandler::AS_ERROR;
    };
  };

  AutomatonActionHandler action_handler =
      [uid_index](AutomatonHandelStruct& handler, AutomatonState state) {
        switch (state) {
          case GpgAutomatonHandler::AS_SELECT:
            return QString("uid %1").arg(uid_index);
          case GpgAutomatonHandler::AS_COMMAND:
            return QString("deluid");
          case GpgAutomatonHandler::AS_REALLY_ULTIMATE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::AS_QUIT:
            return QString("quit");
          case GpgAutomatonHandler::AS_SAVE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::AS_START:
          case GpgAutomatonHandler::AS_ERROR:
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

  return GpgAutomatonHandler::GetInstance(GetChannel())
      .DoInteract(key, next_state_handler, action_handler);
}

auto GpgUIDOperator::RevokeUID(const GpgKey& key, int uid_index,
                               int reason_code,
                               const QString& reason_text) -> bool {
  if (uid_index < 2 || uid_index > static_cast<int>(key.GetUIDs()->size())) {
    LOG_W() << "illegal uid index: " << uid_index;
    return false;
  }

  if (reason_code != 0 && reason_code != 4) {
    LOG_W() << "illegal reason code: " << reason_code;
    return false;
  }

  // dealing with reason text
  auto reason_text_lines = GpgFrontend::SecureCreateSharedObject<QStringList>(
      reason_text.split('\n', Qt::SkipEmptyParts).toVector());

  AutomatonNextStateHandler next_state_handler = [](AutomatonState state,
                                                    QString status,
                                                    QString args) {
    auto tokens = args.split(' ');

    switch (state) {
      case GpgAutomatonHandler::AS_START:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_SELECT;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_SELECT:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_COMMAND;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_COMMAND:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        } else if (status == "GET_BOOL" && args == "keyedit.revoke.uid.okay") {
          return GpgAutomatonHandler::AS_REALLY_ULTIMATE;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_REASON_CODE:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        } else if (status == "GET_LINE" &&
                   args == "ask_revocation_reason.text") {
          return GpgAutomatonHandler::AS_REASON_TEXT;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_REASON_TEXT:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        } else if (status == "GET_LINE" &&
                   args == "ask_revocation_reason.text") {
          return GpgAutomatonHandler::AS_REASON_TEXT;
        } else if (status == "GET_BOOL" &&
                   args == "ask_revocation_reason.okay") {
          return GpgAutomatonHandler::AS_REALLY_ULTIMATE;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_REALLY_ULTIMATE:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        } else if (status == "GET_LINE" &&
                   args == "ask_revocation_reason.code") {
          return GpgAutomatonHandler::AS_REASON_CODE;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_QUIT:
        if (status == "GET_BOOL" && args == "keyedit.save.okay") {
          return GpgAutomatonHandler::AS_SAVE;
        }
        return GpgAutomatonHandler::AS_ERROR;
      case GpgAutomatonHandler::AS_ERROR:
        if (status == "GET_LINE" && args == "keyedit.prompt") {
          return GpgAutomatonHandler::AS_QUIT;
        }
        return GpgAutomatonHandler::AS_ERROR;
      default:
        return GpgAutomatonHandler::AS_ERROR;
    };
  };

  AutomatonActionHandler action_handler =
      [uid_index, reason_code, reason_text_lines](
          AutomatonHandelStruct& handler, AutomatonState state) {
        switch (state) {
          case GpgAutomatonHandler::AS_SELECT:
            return QString("uid %1").arg(uid_index);
          case GpgAutomatonHandler::AS_COMMAND:
            return QString("revuid");
          case GpgAutomatonHandler::AS_REASON_CODE:
            return QString::number(reason_code);
          case GpgAutomatonHandler::AS_REASON_TEXT:
            return reason_text_lines->isEmpty()
                       ? QString("")
                       : QString(reason_text_lines->takeFirst().toUtf8());
          case GpgAutomatonHandler::AS_REALLY_ULTIMATE:
            return QString("Y");
          case GpgAutomatonHandler::AS_QUIT:
            return QString("quit");
          case GpgAutomatonHandler::AS_SAVE:
            handler.SetSuccess(true);
            return QString("Y");
          case GpgAutomatonHandler::AS_START:
          case GpgAutomatonHandler::AS_ERROR:
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

  return GpgAutomatonHandler::GetInstance(GetChannel())
      .DoInteract(key, next_state_handler, action_handler);
}

}  // namespace GpgFrontend
