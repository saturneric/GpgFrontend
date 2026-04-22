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
#include "core/function/openpgp/Async.h"
#include "core/function/openpgp/traits/UserIdOperaTraits.h"

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
  return AddUID(key, QString("%1(%2)<%3>").arg(name).arg(comment).arg(email));
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

}  // namespace GpgFrontend
