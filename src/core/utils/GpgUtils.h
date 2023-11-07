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

#pragma once

#include "core/GpgModel.h"
#include "core/function/result_analyse/GpgResultAnalyse.h"

namespace GpgFrontend {

/**
 * @brief Result Deleter
 *
 */
struct ResultRefDeletor {
  void operator()(void* _result);
};

// Convert from  gpgme_xxx_result to GpgXXXResult

/**
 * @brief
 *
 * @param result
 * @return GpgEncrResult
 */
auto GPGFRONTEND_CORE_EXPORT NewResult(gpgme_encrypt_result_t&& result)
    -> GpgEncrResult;

/**
 * @brief
 *
 * @param result
 * @return GpgDecrResult
 */
auto GPGFRONTEND_CORE_EXPORT NewResult(gpgme_decrypt_result_t&& result)
    -> GpgDecrResult;

/**
 * @brief
 *
 * @param result
 * @return GpgSignResult
 */
auto GPGFRONTEND_CORE_EXPORT NewResult(gpgme_sign_result_t&& result)
    -> GpgSignResult;

/**
 * @brief
 *
 * @param result
 * @return GpgVerifyResult
 */
auto GPGFRONTEND_CORE_EXPORT NewResult(gpgme_verify_result_t&& result)
    -> GpgVerifyResult;

/**
 * @brief
 *
 * @param result
 * @return GpgGenKeyResult
 */
auto GPGFRONTEND_CORE_EXPORT NewResult(gpgme_genkey_result_t&& result)
    -> GpgGenKeyResult;

// Error Info Printer

/**
 * @brief
 *
 * @param err
 * @return GpgError
 */
auto GPGFRONTEND_CORE_EXPORT CheckGpgError(GpgError err) -> GpgError;

/**
 * @brief
 *
 * @param gpgmeError
 * @param comment
 * @return GpgError
 */
auto GPGFRONTEND_CORE_EXPORT CheckGpgError(GpgError gpgmeError,
                                           const std::string& comment)
    -> GpgError;

/**
 * @brief
 *
 * @param err
 * @param predict
 * @return gpg_err_code_t
 */
auto GPGFRONTEND_CORE_EXPORT CheckGpgError2ErrCode(
    gpgme_error_t err, gpgme_error_t predict = GPG_ERR_NO_ERROR)
    -> gpg_err_code_t;

// Check

/**
 * @brief
 *
 * @param text
 * @return int
 */
auto GPGFRONTEND_CORE_EXPORT TextIsSigned(BypeArrayRef text) -> int;

}  // namespace GpgFrontend