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

#pragma once

#include "GFCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgKey.h"
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

class OpenPGPContext;

/**
 * @brief The GnuPG error for an engine status any operation can report.
 *
 * Handlers map their operation-specific statuses first and fall back to this,
 * so no known status reaches the GUI as GPG_ERR_GENERAL ("unknown error").
 *
 * @param err the rPGP engine status
 * @return GpgError
 */
auto CommonEngineError(Rust::GfrStatus err) -> GpgError;

/**
 * @brief Drain the engine's error detail for a failed operation on this
 * thread, logging it under @p op.
 *
 * Call it right after the failing FFI call returns. A cancellation yields an
 * empty detail (it is the user's choice, not a failure needing a reason), but
 * the slot is drained either way so it cannot leak into a later operation.
 *
 * @param err the failed operation's engine status
 * @param op operation name for the log
 * @return QString the detail, possibly empty
 */
auto TakeEngineErrorDetail(Rust::GfrStatus err, const char* op) -> QString;

/// Runs one engine edit on a secret key block: given the key's metadata and
/// its secret block, sets the out pointer to the edited block, which the
/// engine allocated.
using SecretKeyEdit = std::function<Rust::GfrStatus(
    const GFKeyMetadata&, const Rust::GfrBuffer&, char**)>;

/**
 * @brief Edit a secret key in place and store the result.
 *
 * Replaced, not merged: a merge with the stored key would bring back what the
 * edit removed -- a deleted UID or subkey, a superseded self-signature.
 *
 * @param fpr the key to edit
 * @param what the edit, for the log and the engine's error detail
 * @return GPG_ERR_NO_ERROR, the engine's own error for a refused edit, or
 * GPG_ERR_GENERAL when there is no secret key to edit or storing failed
 */
auto EditSecretKeyRpgp(OpenPGPContext& ctx, const QString& fpr,
                       const char* what, const SecretKeyEdit& edit) -> GpgError;

/**
 * @brief The encrypt result for an encryption that never reached the engine
 * because none of the recipients has a public key block in the key database.
 *
 * Every recipient is listed as invalid with GPG_ERR_NO_PUBKEY, so the GUI can
 * say which keys were missing instead of showing an unexplained failure.
 *
 * @param recipients the recipients that were asked for
 * @return GFEncryptResult
 */
auto NoUsableRecipientsResult(const QList<GpgKey>& recipients)
    -> GFEncryptResult;

/**
 * @brief
 *
 * @param r
 * @return GFEncryptResult
 */
auto GF_CORE_EXPORT GfrEncryptResultC2GFEncryptResult(
    const Rust::GfrEncryptResultC& r) -> GFEncryptResult;

/**
 * @brief
 *
 * @param r
 * @return GFDecryptResult
 */
auto GF_CORE_EXPORT GfrDecryptResultC2GFDecryptResult(
    const Rust::GfrDecryptResultC& r) -> GFDecryptResult;
/**
 * @brief
 * @param r
 * @return GFSignResult
 */
auto GF_CORE_EXPORT GfrSignResultC2GFSignResult(const Rust::GfrSignResultC& r)
    -> GFSignResult;

/**
 * @brief
 *
 * @param r
 * @return GFVerifyResult
 */
auto GF_CORE_EXPORT GfrVerifyResultC2GFVerifyResult(
    const Rust::GfrVerifyResultC& r) -> GFVerifyResult;

/**
 * @brief
 *
 * @param r
 * @return GFEncryptAndSignResult
 */
auto GF_CORE_EXPORT GfrEncryptAndSignResultC2GFEncryptAndSignResult(
    const Rust::GfrEncryptAndSignResultC& r) -> GFEncryptAndSignResult;

/**
 * @brief
 *
 * @param r
 * @return GFDecryptAndVerifyResult
 */
auto GF_CORE_EXPORT GfrDecryptAndVerifyResultC2GFDecryptAndVerifyResult(
    const Rust::GfrDecryptAndVerifyResultC& r) -> GFDecryptAndVerifyResult;

/**
 * @brief
 *
 * @param in_buffer
 * @param err
 * @param gfr_result
 * @return std::tuple<GpgError, GFEncryptResult>
 */
auto HandleEncryptResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                         Rust::GfrEncryptResultC gfr_result)
    -> std::tuple<GpgError, GFEncryptResult>;

/**
 * @brief
 *
 * @param in_buffer
 * @param err
 * @param gfr_result
 * @return std::tuple<GpgError, GFDecryptResult>
 */
auto HandleDecryptResult(GFKeyDatabase& key_db, const GFBuffer& in_buffer,
                         Rust::GfrStatus err,
                         Rust::GfrDecryptResultC gfr_result)
    -> std::tuple<GpgError, GFDecryptResult>;

/**
 * @brief
 *
 * @param in_buffer
 * @param err
 * @param gfr_result
 * @return std::tuple<GpgError, GFSignResult>
 */
auto HandleSignResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                      Rust::GfrSignResultC gfr_result)
    -> std::tuple<GpgError, GFSignResult>;

/**
 * @brief
 *
 * @param verify_result
 * @return GFVerifyResult
 */
auto HandleVerifyResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                        Rust::GfrVerifyResultC gfr_result)
    -> std::tuple<GpgError, GFVerifyResult>;

}  // namespace GpgFrontend