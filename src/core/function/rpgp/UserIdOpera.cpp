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

#include "UserIdOpera.h"

#include "core/GFCoreRust.h"
#include "core/function/rpgp/ResultHandler.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

auto AddUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                    const QString& uid) -> bool {
  const auto uid_utf8 = uid.toUtf8();
  return EditSecretKeyRpgp(ctx, key->Fingerprint(), "Adding a user ID",
                           [&](const GFKeyMetadata&,
                               const Rust::GfrBuffer& block, char** out) {
                             return Rust::gfr_crypto_add_user_id(
                                 ctx.GetChannel(), block, uid_utf8.constData(),
                                 FetchPasswordCallback, out);
                           }) == GPG_ERR_NO_ERROR;
}

auto DeleteUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                       const QString& uid) -> bool {
  const auto uid_utf8 = uid.toUtf8();
  return EditSecretKeyRpgp(ctx, key->Fingerprint(), "Deleting a user ID",
                           [&](const GFKeyMetadata& meta,
                               const Rust::GfrBuffer& block, char** out) {
                             const auto& uids = meta.user_ids;
                             if (std::none_of(uids.begin(), uids.end(),
                                              [&](const GFUserId& u) {
                                                return u.ToString() == uid;
                                              })) {
                               LOG_E() << "UID:" << uid
                                       << "not found in key:" << meta.fpr;
                               return Rust::GfrStatus::ErrorInvalidInput;
                             }
                             return Rust::gfr_crypto_delete_user_id(
                                 block, uid_utf8.constData(), out);
                           }) == GPG_ERR_NO_ERROR;
}

auto SetPrimaryUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                           const QString& uid) -> bool {
  const auto uid_utf8 = uid.toUtf8();
  return EditSecretKeyRpgp(ctx, key->Fingerprint(),
                           "Setting the primary user ID",
                           [&](const GFKeyMetadata&,
                               const Rust::GfrBuffer& block, char** out) {
                             return Rust::gfr_crypto_set_primary_user_id(
                                 ctx.GetChannel(), block, uid_utf8.constData(),
                                 FetchPasswordCallback, out);
                           }) == GPG_ERR_NO_ERROR;
}

auto RevokeUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                       const QString& uid, int reason_code,
                       const QString& reason_text) -> bool {
  const auto uid_utf8 = uid.toUtf8();
  const auto reason_text_utf8 = reason_text.toUtf8();
  // 4 is GnuPG's "user ID is no longer valid"; anything else gives no reason.
  const auto mapped_reason_code = reason_code == 4
                                      ? Rust::GfrRevocationCode::UserIdInvalid
                                      : Rust::GfrRevocationCode::NoReason;

  return EditSecretKeyRpgp(ctx, key->Fingerprint(), "Revoking a user ID",
                           [&](const GFKeyMetadata&,
                               const Rust::GfrBuffer& block, char** out) {
                             return Rust::gfr_crypto_revoke_user_id(
                                 ctx.GetChannel(), block, uid_utf8.constData(),
                                 mapped_reason_code,
                                 reason_text_utf8.constData(),
                                 FetchPasswordCallback, out);
                           }) == GPG_ERR_NO_ERROR;
}

}  // namespace GpgFrontend
