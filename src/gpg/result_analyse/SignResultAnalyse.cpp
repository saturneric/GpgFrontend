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

SignResultAnalyse::SignResultAnalyse(gpgme_error_t error, gpgme_sign_result_t result) {

    if(result == nullptr) {
        return;
    }

    stream << "# Sign Report: " << endl
        << "-----" << endl;
    stream << "Status: " << gpgme_strerror(error) << endl << endl;

    auto new_sign = result->signatures;

    while(new_sign != nullptr) {
        stream << "> A New Signature: " << endl;

        stream << "Sign mode: ";
        if(new_sign->type & GPGME_SIG_MODE_NORMAL)
            stream << "Normal";
        else if(new_sign->type & GPGME_SIG_MODE_CLEAR)
            stream << "Clear";
        else if(new_sign->type & GPGME_SIG_MODE_DETACH)
            stream << "Detach";

        stream << endl;

        stream << "Public key algo: " << gpgme_pubkey_algo_name(new_sign->pubkey_algo) << endl;
        stream << "Hash algo: " << gpgme_hash_algo_name(new_sign->hash_algo) << endl;
        stream << "Date of signature: " << QDateTime::fromTime_t(new_sign->timestamp).toString() << endl;

        stream << endl;

        new_sign = new_sign->next;
    }

    auto invalid_signer = result->invalid_signers;

    if(invalid_signer!= nullptr)
        stream << "Invalid Signers: " << endl;

    while(invalid_signer != nullptr) {
        setStatus(0);
        stream << "Fingerprint: " << invalid_signer->fpr << endl;
        stream << "Reason: " << gpgme_strerror(invalid_signer->reason) << endl;
        stream << endl;

        invalid_signer = invalid_signer->next;
    }

    stream << "-----" << endl;
    stream << endl;

}
