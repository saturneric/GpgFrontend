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

#include "GpgFrontendCore.h"

const int kRestartCode = 1000;      ///< only refresh ui
const int kDeepRestartCode = 1001;  // refresh core and ui

namespace GpgFrontend {
using ByteArray = std::string;                                    ///<
using ByteArrayPtr = std::shared_ptr<ByteArray>;                  ///<
using StdBypeArrayPtr = std::shared_ptr<ByteArray>;               ///<
using BypeArrayRef = ByteArray&;                                  ///<
using BypeArrayConstRef = const ByteArray&;                       ///<
using StringArgsPtr = std::unique_ptr<std::vector<std::string>>;  ///<
using StringArgsRef = std::vector<std::string>&;                  ///<

using GpgError = gpgme_error_t;

/**
 * @brief Result Deleter
 *
 */
struct ResultRefDeletor {
  void operator()(void* _result);
};

using GpgEncrResult = std::shared_ptr<struct _gpgme_op_encrypt_result>;   ///<
using GpgDecrResult = std::shared_ptr<struct _gpgme_op_decrypt_result>;   ///<
using GpgSignResult = std::shared_ptr<struct _gpgme_op_sign_result>;      ///<
using GpgVerifyResult = std::shared_ptr<struct _gpgme_op_verify_result>;  ///<
using GpgGenKeyResult = std::shared_ptr<struct _gpgme_op_genkey_result>;  ///<

// Convert from  gpgme_xxx_result to GpgXXXResult

/**
 * @brief
 *
 * @param result
 * @return GpgEncrResult
 */
GPGFRONTEND_CORE_EXPORT auto NewResult(gpgme_encrypt_result_t&& result)
    -> GpgEncrResult;

/**
 * @brief
 *
 * @param result
 * @return GpgDecrResult
 */
GPGFRONTEND_CORE_EXPORT auto NewResult(gpgme_decrypt_result_t&& result)
    -> GpgDecrResult;

/**
 * @brief
 *
 * @param result
 * @return GpgSignResult
 */
GPGFRONTEND_CORE_EXPORT auto NewResult(gpgme_sign_result_t&& result)
    -> GpgSignResult;

/**
 * @brief
 *
 * @param result
 * @return GpgVerifyResult
 */
GPGFRONTEND_CORE_EXPORT auto NewResult(gpgme_verify_result_t&& result)
    -> GpgVerifyResult;

/**
 * @brief
 *
 * @param result
 * @return GpgGenKeyResult
 */
GPGFRONTEND_CORE_EXPORT auto NewResult(gpgme_genkey_result_t&& result)
    -> GpgGenKeyResult;

// Error Info Printer

/**
 * @brief
 *
 * @param err
 * @return GpgError
 */
GPGFRONTEND_CORE_EXPORT auto CheckGpgError(GpgError err) -> GpgError;

/**
 * @brief
 *
 * @param gpgmeError
 * @param comment
 * @return GpgError
 */
GPGFRONTEND_CORE_EXPORT auto CheckGpgError(GpgError gpgmeError,
                                           const std::string& comment)
    -> GpgError;

/**
 * @brief
 *
 * @param err
 * @param predict
 * @return gpg_err_code_t
 */
GPGFRONTEND_CORE_EXPORT auto CheckGpgError2ErrCode(
    gpgme_error_t err, gpgme_error_t predict = GPG_ERR_NO_ERROR)
    -> gpg_err_code_t;

// Fingerprint

/**
 * @brief
 *
 * @param fingerprint
 * @return std::string
 */
GPGFRONTEND_CORE_EXPORT auto BeautifyFingerprint(BypeArrayConstRef fingerprint)
    -> std::string;

// File Operation

/**
 * @brief
 *
 * @param path
 * @return std::string
 */
auto ReadAllDataInFile(const std::string& path) -> std::string;

/**
 * @brief
 *
 * @param path
 * @param out_buffer
 * @return true
 * @return false
 */
auto GPGFRONTEND_CORE_EXPORT WriteBufferToFile(const std::string& path,
                                               const std::string& out_buffer)
    -> bool;

auto GPGFRONTEND_CORE_EXPORT CompareSoftwareVersion(const std::string& a,
                                                    const std::string& b)
    -> int;

/**
 * @brief Get the file extension object
 *
 * @param path
 * @return std::string
 */
auto GetFileExtension(const std::string& path) -> std::string;

/**
 * @brief Get the only file name with path object
 *
 * @param path
 * @return std::string
 */
auto GetOnlyFileNameWithPath(const std::string& path) -> std::string;

// Check

/**
 * @brief
 *
 * @param text
 * @return int
 */
auto TextIsSigned(BypeArrayRef text) -> int;

// Channels
const int kGpgfrontendDefaultChannel = 0;   ///<
const int kGpgfrontendNonAsciiChannel = 2;  ///<

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgConstants {
 public:
  static const char* PGP_CRYPT_BEGIN;                 ///<
  static const char* PGP_CRYPT_END;                   ///<
  static const char* PGP_SIGNED_BEGIN;                ///<
  static const char* PGP_SIGNED_END;                  ///<
  static const char* PGP_SIGNATURE_BEGIN;             ///<
  static const char* PGP_SIGNATURE_END;               ///<
  static const char* PGP_PUBLIC_KEY_BEGIN;            ///<
  static const char* PGP_PRIVATE_KEY_BEGIN;           ///<
  static const char* GPG_FRONTEND_SHORT_CRYPTO_HEAD;  ///<
};

}  // namespace GpgFrontend
