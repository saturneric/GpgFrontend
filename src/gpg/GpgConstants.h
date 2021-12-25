/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPG_CONSTANTS_H
#define GPG_CONSTANTS_H

#include <gpg-error.h>
#include <gpgme.h>

#include <cassert>
#include <functional>
#include <memory>
#include <string>

#include "GpgFrontend.h"

const int RESTART_CODE = 1000;

namespace GpgFrontend {

using ByteArray = std::string;
using ByteArrayPtr = std::unique_ptr<ByteArray>;
using StdBypeArrayPtr = std::unique_ptr<ByteArray>;
using BypeArrayRef = ByteArray&;
using BypeArrayConstRef = const ByteArray&;
using StringArgsPtr = std::unique_ptr<std::vector<std::string>>;
using StringArgsRef = std::vector<std::string>&;

using GpgError = gpgme_error_t;

// Result Deleter
struct _result_ref_deletor {
  void operator()(void* _result);
};

using GpgEncrResult = std::shared_ptr<struct _gpgme_op_encrypt_result>;
using GpgDecrResult = std::shared_ptr<struct _gpgme_op_decrypt_result>;
using GpgSignResult = std::shared_ptr<struct _gpgme_op_sign_result>;
using GpgVerifyResult = std::shared_ptr<struct _gpgme_op_verify_result>;
using GpgGenKeyResult = std::shared_ptr<struct _gpgme_op_genkey_result>;

// Convert from  gpgme_xxx_result to GpgXXXResult
GpgEncrResult _new_result(gpgme_encrypt_result_t&& result);
GpgDecrResult _new_result(gpgme_decrypt_result_t&& result);
GpgSignResult _new_result(gpgme_sign_result_t&& result);
GpgVerifyResult _new_result(gpgme_verify_result_t&& result);
GpgGenKeyResult _new_result(gpgme_genkey_result_t&& result);

// Error Info Printer
GpgError check_gpg_error(GpgError err);
GpgError check_gpg_error(GpgError gpgmeError, const std::string& comment);
gpg_err_code_t check_gpg_error_2_err_code(
    gpgme_error_t err, gpgme_error_t predict = GPG_ERR_NO_ERROR);

// Fingerprint
std::string beautify_fingerprint(BypeArrayConstRef fingerprint);

// File Operation
std::string read_all_data_in_file(const std::string& path);
bool write_buffer_to_file(const std::string& path,
                          const std::string& out_buffer);

std::string get_file_extension(const std::string& path);
std::string get_only_file_name_with_path(const std::string& path);

// Check
int text_is_signed(BypeArrayRef text);

class GpgConstants {
 public:
  static const char* PGP_CRYPT_BEGIN;
  static const char* PGP_CRYPT_END;
  static const char* PGP_SIGNED_BEGIN;
  static const char* PGP_SIGNED_END;
  static const char* PGP_SIGNATURE_BEGIN;
  static const char* PGP_SIGNATURE_END;
  static const char* GPG_FRONTEND_SHORT_CRYPTO_HEAD;
};
}  // namespace GpgFrontend

#endif  // GPG_CONSTANTS_H
