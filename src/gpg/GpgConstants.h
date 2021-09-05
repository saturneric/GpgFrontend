/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#include "GpgFrontend.h"

#include <QtCore>
#include <functional>

#include <gpgme.h>
#include <memory>
#include <string>

const int RESTART_CODE = 1000;

namespace GpgFrontend {

using BypeArrayPtr = std::unique_ptr<QByteArray>;
using StdBypeArrayPtr = std::unique_ptr<std::string>;
using BypeArrayRef = QByteArray &;

using GpgError = gpgme_error_t;

using GpgEncrResult =
    std::unique_ptr<struct _gpgme_op_encrypt_result,
                    std::function<void(gpgme_encrypt_result_t)>>;
using GpgDecrResult =
    std::unique_ptr<struct _gpgme_op_decrypt_result,
                    std::function<void(gpgme_decrypt_result_t)>>;
using GpgSignResult = std::unique_ptr<struct _gpgme_op_sign_result,
                                      std::function<void(gpgme_sign_result_t)>>;
using GpgVerifyResult =
    std::unique_ptr<struct _gpgme_op_verify_result,
                    std::function<void(gpgme_verify_result_t)>>;

// Error Info Printer
GpgError check_gpg_error(GpgError err);

GpgError check_gpg_error(GpgError gpgmeError, const std::string &comment);

// Fingerprint
std::string beautify_fingerprint(std::string fingerprint);

// Check
int text_is_signed(const QByteArray &text);

class GpgConstants {
public:
  static const char *PGP_CRYPT_BEGIN;
  static const char *PGP_CRYPT_END;
  static const char *PGP_SIGNED_BEGIN;
  static const char *PGP_SIGNED_END;
  static const char *PGP_SIGNATURE_BEGIN;
  static const char *PGP_SIGNATURE_END;
  static const char *GPG_FRONTEND_SHORT_CRYPTO_HEAD;
};
} // namespace GpgFrontend

#endif // GPG_CONSTANTS_H
