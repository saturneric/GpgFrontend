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

#include "UserIdOperation.h"

#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/UserIdOperaTraits.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

UserIdOperation::UserIdOperation(int channel)
    : SingletonFunctionObject<UserIdOperation>(channel) {}

auto UserIdOperation::AddUID(const GpgKeyPtr& key, const QString& uid) -> bool {
  return RunRegisteredForward<AddUserIdOpTag>(ctx_, key, uid);
}

auto UserIdOperation::SetPrimaryUID(const GpgKeyPtr& key, const QString& uid)
    -> bool {
  return RunRegisteredForward<SetPrimaryUserIdOpTag>(ctx_, key, uid);
}

auto UserIdOperation::AddUID(const GpgKeyPtr& key, const QString& name,
                             const QString& comment, const QString& email)
    -> bool {
  if (!IsValidUserIdComponent(name) || !IsValidUserIdComponent(comment)) {
    LOG_E() << "refusing to add UID with malformed name or comment component";
    return false;
  }
  return AddUID(key, AssembleUserId(name, comment, email));
}

auto UserIdOperation::DeleteUID(const GpgKeyPtr& key, const QString& uid)
    -> bool {
  return RunRegisteredForward<DeleteUserIdOpTag>(ctx_, key, uid);
}

auto UserIdOperation::RevokeUID(const GpgKeyPtr& key, const QString& uid,
                                int reason_code, const QString& reason_text)
    -> bool {
  return RunRegisteredForward<RevokeUserIdOpTag>(ctx_, key, uid, reason_code,
                                                 reason_text);
}

void UserIdOperation::AddUID(const GpgKeyPtr& key, const QString& name,
                             const QString& comment, const QString& email,
                             const GpgOperationCallback& cb) {
  if (!IsValidUserIdComponent(name) || !IsValidUserIdComponent(comment)) {
    LOG_E() << "refusing to add UID with malformed name or comment component";
    cb(GPG_ERR_INV_VALUE, nullptr);
    return;
  }

  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid = AssembleUserId(name, comment, email)](
          const DataObjectPtr&) -> GpgError {
        return AddUID(key, uid) ? GPG_ERR_NO_ERROR : GPG_ERR_GENERAL;
      },
      cb, OpTraits<AddUserIdOpTag>::kOpName,
      OpTraits<AddUserIdOpTag>::Versions());
}

void UserIdOperation::DeleteUID(const GpgKeyPtr& key, const QString& uid,
                                const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid](const DataObjectPtr&) -> GpgError {
        return DeleteUID(key, uid) ? GPG_ERR_NO_ERROR : GPG_ERR_GENERAL;
      },
      cb, OpTraits<DeleteUserIdOpTag>::kOpName,
      OpTraits<DeleteUserIdOpTag>::Versions());
}

void UserIdOperation::RevokeUID(const GpgKeyPtr& key, const QString& uid,
                                int reason_code, const QString& reason_text,
                                const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid, reason_code, reason_text](
          const DataObjectPtr&) -> GpgError {
        return RevokeUID(key, uid, reason_code, reason_text) ? GPG_ERR_NO_ERROR
                                                             : GPG_ERR_GENERAL;
      },
      cb, OpTraits<RevokeUserIdOpTag>::kOpName,
      OpTraits<RevokeUserIdOpTag>::Versions());
}

void UserIdOperation::SetPrimaryUID(const GpgKeyPtr& key, const QString& uid,
                                    const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid](const DataObjectPtr&) -> GpgError {
        return SetPrimaryUID(key, uid) ? GPG_ERR_NO_ERROR : GPG_ERR_GENERAL;
      },
      cb, OpTraits<SetPrimaryUserIdOpTag>::kOpName,
      OpTraits<SetPrimaryUserIdOpTag>::Versions());
}

}  // namespace GpgFrontend
