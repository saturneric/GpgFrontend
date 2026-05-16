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
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

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