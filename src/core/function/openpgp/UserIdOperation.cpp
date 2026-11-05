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

#include <algorithm>

#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/UserIdOperaTraits.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

UserIdOperation::UserIdOperation(int channel)
    : SingletonFunctionObject<UserIdOperation>(channel) {}

auto UserIdOperation::AddUIDSync(const GpgKeyPtr& key, const QString& uid)
    -> GFError {
  return RunRegisteredForward<AddUserIdOpTag>(ctx_, key, uid);
}

auto UserIdOperation::AddUIDSync(const GpgKeyPtr& key, const QString& name,
                                 const QString& comment, const QString& email)
    -> GFError {
  if (name.trimmed().isEmpty() || !IsValidUserIdComponent(name) ||
      !IsValidUserIdComponent(comment)) {
    LOG_E() << "refusing to add UID with empty or malformed name, or malformed "
               "comment component";
    return GFError::Application(GFErrorCode::kINVALID_VALUE);
  }
  return AddUIDSync(key, AssembleUserId(name, comment, email));
}

auto UserIdOperation::SetPrimaryUIDSync(const GpgKeyPtr& key,
                                        const QString& uid) -> GFError {
  return RunRegisteredForward<SetPrimaryUserIdOpTag>(ctx_, key, uid);
}

namespace {

/**
 * @brief Whether @p uid may be deleted from or revoked on @p key.
 *
 * One rule for both engines, decided here rather than in each: GnuPG refused
 * the first User ID while rPGP deleted it, and matched User IDs differently.
 * The first User ID is the one the key is known by, and taking it away leaves
 * a key whose identity is whatever happens to be listed next.
 */
auto CheckRemovableUID(const GpgKeyPtr& key, const QString& uid) -> GFError {
  if (key == nullptr) return GFError::Application(GFErrorCode::kINVALID_VALUE);
  const auto uids = key->UIDs();
  const auto it = std::find_if(uids.begin(), uids.end(), [&](const auto& u) {
    return u.GetUID() == uid;
  });
  if (it == uids.end()) {
    LOG_W() << "no such user ID on the key:" << key->Fingerprint();
    return GFError::Application(GFErrorCode::kINVALID_VALUE);
  }
  if (it == uids.begin()) {
    LOG_W() << "refusing to remove the first user ID of" << key->Fingerprint();
    return GFError::Application(GFErrorCode::kINVALID_VALUE);
  }
  return {};
}

}  // namespace

auto UserIdOperation::DeleteUIDSync(const GpgKeyPtr& key, const QString& uid)
    -> GFError {
  if (auto err = CheckRemovableUID(key, uid); !err.Ok()) return err;
  return RunRegisteredForward<DeleteUserIdOpTag>(ctx_, key, uid);
}

auto UserIdOperation::RevokeUIDSync(const GpgKeyPtr& key, const QString& uid,
                                    int reason_code, const QString& reason_text)
    -> GFError {
  if (auto err = CheckRemovableUID(key, uid); !err.Ok()) return err;
  // The reasons a User ID revocation can give (RFC 9580 §5.2.3.31): none, or
  // "no longer valid". rPGP turned any other into "no reason" without a word.
  if (reason_code != 0 && reason_code != 4) {
    LOG_W() << "illegal reason code for a user ID revocation:" << reason_code;
    return GFError::Application(GFErrorCode::kINVALID_VALUE);
  }
  return RunRegisteredForward<RevokeUserIdOpTag>(ctx_, key, uid, reason_code,
                                                 reason_text);
}

auto UserIdOperation::AddUID(const GpgKeyPtr& key, const QString& uid) -> bool {
  return AddUIDSync(key, uid).Ok();
}

auto UserIdOperation::SetPrimaryUID(const GpgKeyPtr& key, const QString& uid)
    -> bool {
  return SetPrimaryUIDSync(key, uid).Ok();
}

auto UserIdOperation::AddUID(const GpgKeyPtr& key, const QString& name,
                             const QString& comment, const QString& email)
    -> bool {
  return AddUIDSync(key, name, comment, email).Ok();
}

auto UserIdOperation::DeleteUID(const GpgKeyPtr& key, const QString& uid)
    -> bool {
  return DeleteUIDSync(key, uid).Ok();
}

auto UserIdOperation::RevokeUID(const GpgKeyPtr& key, const QString& uid,
                                int reason_code, const QString& reason_text)
    -> bool {
  return RevokeUIDSync(key, uid, reason_code, reason_text).Ok();
}

void UserIdOperation::AddUID(const GpgKeyPtr& key, const QString& name,
                             const QString& comment, const QString& email,
                             const GFOperationCallback& cb) {
  if (name.trimmed().isEmpty() || !IsValidUserIdComponent(name) ||
      !IsValidUserIdComponent(comment)) {
    LOG_E() << "refusing to add UID with empty or malformed name, or malformed "
               "comment component";
    cb(GFError::Application(GFErrorCode::kINVALID_VALUE), nullptr);
    return;
  }

  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid = AssembleUserId(name, comment, email)](
          const DataObjectPtr&) -> GFError { return AddUIDSync(key, uid); },
      cb, OpTraits<AddUserIdOpTag>::kOpName,
      OpTraits<AddUserIdOpTag>::Versions());
}

void UserIdOperation::DeleteUID(const GpgKeyPtr& key, const QString& uid,
                                const GFOperationCallback& cb) {
  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid](const DataObjectPtr&) -> GFError {
        return DeleteUIDSync(key, uid);
      },
      cb, OpTraits<DeleteUserIdOpTag>::kOpName,
      OpTraits<DeleteUserIdOpTag>::Versions());
}

void UserIdOperation::RevokeUID(const GpgKeyPtr& key, const QString& uid,
                                int reason_code, const QString& reason_text,
                                const GFOperationCallback& cb) {
  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid, reason_code,
       reason_text](const DataObjectPtr&) -> GFError {
        return RevokeUIDSync(key, uid, reason_code, reason_text);
      },
      cb, OpTraits<RevokeUserIdOpTag>::kOpName,
      OpTraits<RevokeUserIdOpTag>::Versions());
}

void UserIdOperation::SetPrimaryUID(const GpgKeyPtr& key, const QString& uid,
                                    const GFOperationCallback& cb) {
  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [this, key, uid](const DataObjectPtr&) -> GFError {
        return SetPrimaryUIDSync(key, uid);
      },
      cb, OpTraits<SetPrimaryUserIdOpTag>::kOpName,
      OpTraits<SetPrimaryUserIdOpTag>::Versions());
}

}  // namespace GpgFrontend
