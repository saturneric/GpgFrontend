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

#include <gpgme.h>

#include "GpgFrontendCore.h"

const int RESTART_CODE = 1000;       ///< only refresh ui
const int DEEP_RESTART_CODE = 1001;  // refresh core and ui

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
struct _result_ref_deletor {
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
GPGFRONTEND_CORE_EXPORT GpgEncrResult
_new_result(gpgme_encrypt_result_t&& result);

/**
 * @brief
 *
 * @param result
 * @return GpgDecrResult
 */
GPGFRONTEND_CORE_EXPORT GpgDecrResult
_new_result(gpgme_decrypt_result_t&& result);

/**
 * @brief
 *
 * @param result
 * @return GpgSignResult
 */
GPGFRONTEND_CORE_EXPORT GpgSignResult _new_result(gpgme_sign_result_t&& result);

/**
 * @brief
 *
 * @param result
 * @return GpgVerifyResult
 */
GPGFRONTEND_CORE_EXPORT GpgVerifyResult
_new_result(gpgme_verify_result_t&& result);

/**
 * @brief
 *
 * @param result
 * @return GpgGenKeyResult
 */
GPGFRONTEND_CORE_EXPORT GpgGenKeyResult
_new_result(gpgme_genkey_result_t&& result);

// Error Info Printer

/**
 * @brief
 *
 * @param err
 * @return GpgError
 */
GPGFRONTEND_CORE_EXPORT GpgError check_gpg_error(GpgError err);

/**
 * @brief
 *
 * @param gpgmeError
 * @param comment
 * @return GpgError
 */
GPGFRONTEND_CORE_EXPORT GpgError check_gpg_error(GpgError gpgmeError,
                                                 const std::string& comment);

/**
 * @brief
 *
 * @param err
 * @param predict
 * @return gpg_err_code_t
 */
GPGFRONTEND_CORE_EXPORT gpg_err_code_t check_gpg_error_2_err_code(
    gpgme_error_t err, gpgme_error_t predict = GPG_ERR_NO_ERROR);

// Fingerprint

/**
 * @brief
 *
 * @param fingerprint
 * @return std::string
 */
GPGFRONTEND_CORE_EXPORT std::string beautify_fingerprint(
    BypeArrayConstRef fingerprint);

// File Operation

/**
 * @brief
 *
 * @param path
 * @return std::string
 */
std::string read_all_data_in_file(const std::string& path);

/**
 * @brief
 *
 * @param path
 * @param out_buffer
 * @return true
 * @return false
 */
GPGFRONTEND_CORE_EXPORT bool write_buffer_to_file(
    const std::string& path, const std::string& out_buffer);

/**
 * @brief Get the file extension object
 *
 * @param path
 * @return std::string
 */
std::string get_file_extension(const std::string& path);

/**
 * @brief Get the only file name with path object
 *
 * @param path
 * @return std::string
 */
std::string get_only_file_name_with_path(const std::string& path);

// Check

/**
 * @brief
 *
 * @param text
 * @return int
 */
int text_is_signed(BypeArrayRef text);

// Channels
const int GPGFRONTEND_DEFAULT_CHANNEL = 0;    ///<
const int GPGFRONTEND_NON_ASCII_CHANNEL = 2;  ///<

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
