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

#ifdef HAS_RUST_SUPPORT

#include "core/GpgCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

/**
 * @brief
 *
 * @param algo_id
 * @return Rust::GfrKeyAlgo
 */
auto GF_CORE_EXPORT KeyAlgoId2GfrKeyAlgo(const QString& algo_id)
    -> Rust::GfrKeyAlgo;

/**
 * @brief convert GfrKeyAlgo to human readable key algo name
 *
 * @param algo
 * @return QString
 */
auto GF_CORE_EXPORT GfrKeyAlgo2KeyAlgoName(Rust::GfrKeyAlgo algo) -> QString;

/**
 * @brief
 *
 * @param in_buffer
 * @return QStringList
 */
auto GF_CORE_EXPORT SniffRecipientKeyIds(const GFBuffer& in_buffer)
    -> QStringList;

/**
 * @brief
 *
 * @param in_buffer
 * @return QStringList
 */
auto GF_CORE_EXPORT SniffIssuerKeyIds(const GFBuffer& in_buffer) -> QStringList;

/**
 * @brief
 *
 * @param key_db
 * @param key_ids
 * @return QString
 */
auto GF_CORE_EXPORT GetKeyBlocksForVerification(GFKeyDatabase& key_db,
                                                const QStringList& key_ids)
    -> QContainer<QByteArray>;

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
 * @param key_db
 * @param key_ids
 * @param secret
 * @return QContainer<QByteArray>
 */
auto GF_CORE_EXPORT GetArmoredKeyBlocksForKeys(GFKeyDatabase& key_db,
                                               const QStringList& key_ids,
                                               bool secret)
    -> QContainer<QByteArray>;

}  // namespace GpgFrontend

#endif  // HAS_RUST_SUPPORT