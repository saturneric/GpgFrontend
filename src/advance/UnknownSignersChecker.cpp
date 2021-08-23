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

#include "advance/UnknownSignersChecker.h"


UnknownSignersChecker::UnknownSignersChecker(GpgME::GpgContext *ctx, gpgme_verify_result_t result) :
        appPath(qApp->applicationDirPath()), settings(RESOURCE_DIR(appPath) + "/conf/gpgfrontend.ini"), mCtx(ctx),
        mResult(result) {

}

void UnknownSignersChecker::start() {

    auto sign = mResult->signatures;
    bool canContinue = true;

    while (sign && canContinue) {

        switch (gpg_err_code(sign->status)) {
            case GPG_ERR_BAD_SIGNATURE:
                break;
            case GPG_ERR_NO_ERROR:
                if (!(sign->status & GPGME_SIGSUM_KEY_MISSING))
                    check_signer(sign);
                break;
            case GPG_ERR_NO_PUBKEY:

            case GPG_ERR_CERT_REVOKED:
            case GPG_ERR_SIG_EXPIRED:
            case GPG_ERR_KEY_EXPIRED:
                check_signer(sign);
                break;
            case GPG_ERR_GENERAL:
                canContinue = false;
                break;
            default:
                break;
        }
        sign = sign->next;
    }

    if(!unknownFprs.isEmpty()) {
        PubkeyGetter pubkeyGetter(mCtx, unknownFprs);
        pubkeyGetter.start();
        if (!pubkeyGetter.result()) {

        }
    }
}

void UnknownSignersChecker::check_signer(gpgme_signature_t sign) {

    auto key = mCtx->getKeyByFpr(sign->fpr);
    if (!key.good) {
        qDebug() << "Find Unknown FingerPrint " << sign->fpr;
        unknownFprs.append(sign->fpr);
    }

}
