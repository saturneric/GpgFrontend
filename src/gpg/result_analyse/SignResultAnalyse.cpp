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

#include "gpg/result_analyse/SignResultAnalyse.h"

SignResultAnalyse::SignResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_sign_result_t result) {

    qDebug() << "Start Sign Result Analyse";

    stream << tr("[#] Sign Operation ");

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR)
        stream << tr("[Success]") << Qt::endl;
    else {
        stream << tr("[Failed] ") << gpgme_strerror(error) << Qt::endl;
        setStatus(-1);
    }

    if (result != nullptr && (result->signatures != nullptr || result->invalid_signers != nullptr)) {
        stream << "------------>" << Qt::endl;
        auto new_sign = result->signatures;

        while (new_sign != nullptr) {
            stream << tr("[>] New Signature: ") << Qt::endl;

            qDebug() << "Signers Fingerprint: " << new_sign->fpr;

            stream << tr("    Sign Mode: ");
            if (new_sign->type == GPGME_SIG_MODE_NORMAL)
                stream << tr("Normal");
            else if (new_sign->type == GPGME_SIG_MODE_CLEAR)
                stream << tr("Clear");
            else if (new_sign->type == GPGME_SIG_MODE_DETACH)
                stream << tr("Detach");

            stream << Qt::endl;

            GpgKey singerKey = ctx->getKeyByFpr(new_sign->fpr);
            if(singerKey.good) {
                stream << tr("    Signer: ") << singerKey.uids.first().uid << Qt::endl;
            } else {
                stream << tr("    Signer: ") << tr("<unknown>") << Qt::endl;
            }
            stream << tr("    Public Key Algo: ") << gpgme_pubkey_algo_name(new_sign->pubkey_algo) << Qt::endl;
            stream << tr("    Hash Algo: ") << gpgme_hash_algo_name(new_sign->hash_algo) << Qt::endl;
            stream << tr("    Date & Time: ") << QDateTime::fromTime_t(new_sign->timestamp).toString() << Qt::endl;

            stream << Qt::endl;

            new_sign = new_sign->next;
        }

        auto invalid_signer = result->invalid_signers;

        if (invalid_signer != nullptr)
            stream << tr("Invalid Signers: ") << Qt::endl;

        while (invalid_signer != nullptr) {
            setStatus(0);
            stream << tr("[>] Signer: ") << Qt::endl;
            stream << tr("      Fingerprint: ") << invalid_signer->fpr << Qt::endl;
            stream << tr("      Reason: ") << gpgme_strerror(invalid_signer->reason) << Qt::endl;
            stream << Qt::endl;

            invalid_signer = invalid_signer->next;
        }
        stream << "<------------" << Qt::endl;

    }

}
