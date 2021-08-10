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

#include "gpg/GpgContext.h"

/**
 * Generate a new subkey of a certain key pair
 * @param key target key pair
 * @param params opera args
 * @return error info
 */
gpgme_error_t GpgME::GpgContext::generateSubkey(const GpgKey &key, GenKeyInfo *params) {

    if (!params->isSubKey()) return GPG_ERR_CANCELED;

    auto algo_utf8 = (params->getAlgo() + params->getKeySizeStr()).toUtf8();
    const char *algo = algo_utf8.constData();
    unsigned long expires = QDateTime::currentDateTime().secsTo(params->getExpired());
    unsigned int flags = 0;

    if (!params->isSubKey()) flags |= GPGME_CREATE_CERT;
    if (params->isAllowEncryption()) flags |= GPGME_CREATE_ENCR;
    if (params->isAllowSigning()) flags |= GPGME_CREATE_SIGN;
    if (params->isAllowAuthentication()) flags |= GPGME_CREATE_AUTH;
    if (params->isNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;

    flags |= GPGME_CREATE_NOPASSWD;


    auto gpgmeError = gpgme_op_createsubkey(mCtx, key.key_refer,
                                            algo, 0, expires, flags);
    if (gpgme_err_code(gpgmeError) == GPG_ERR_NO_ERROR) {
        emit signalKeyUpdated(key.id);
        return gpgmeError;
    } else {
        checkErr(gpgmeError);
        return gpgmeError;
    }
}

