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

#ifndef GPGFRONTEND_GPGFILEOPERA_H
#define GPGFRONTEND_GPGFILEOPERA_H

#include "GpgFrontend.h"
#include "gpg/GpgContext.h"

class GpgFileOpera {
public:
    static gpgme_error_t encryptFile(GpgME::GpgContext *ctx, QVector<GpgKey> &keys, const QString &mPath,
                                     gpgme_encrypt_result_t *result);

    static gpgme_error_t decryptFile(GpgME::GpgContext *ctx, const QString &mPath, gpgme_decrypt_result_t *result);

    static gpgme_error_t signFile(GpgME::GpgContext *ctx, QVector<GpgKey> &keys, const QString &mPath,
                                  gpgme_sign_result_t *result);

    static gpgme_error_t verifyFile(GpgME::GpgContext *ctx, const QString &mPath, gpgme_verify_result_t *result);

    static gpg_error_t
    encryptSignFile(GpgME::GpgContext *ctx, QVector<GpgKey> &keys, const QString &mPath, gpgme_encrypt_result_t *encr_res, gpgme_sign_result_t *sign_res);

    static gpg_error_t decryptVerifyFile(GpgME::GpgContext *ctx, const QString &mPath, gpgme_decrypt_result_t *decr_res, gpgme_verify_result_t *verify_res);

};


#endif //GPGFRONTEND_GPGFILEOPERA_H
