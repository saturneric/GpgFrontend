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

#include "GpgFrontend.h"
#include "gpg/result_analyse/VerifyResultAnalyse.h"

VerifyResultAnalyse::VerifyResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_verify_result_t result)
        : mCtx(ctx) {

    stream << tr("[#] Verify Operation ");

    if (gpgme_err_code(error) == GPG_ERR_NO_ERROR)
        stream << tr("[Success]") << Qt::endl;
    else {
        stream << tr("[Failed] ") << gpgme_strerror(error) << Qt::endl;
        setStatus(-1);
    }


    if(result != nullptr && result->signatures) {
        stream << "------------>" << Qt::endl;
        auto sign = result->signatures;

        if (sign == nullptr) {
            stream << "[>] Not Signature Found" << Qt::endl;
            setStatus(0);
            return;
        }

        stream << "[>] Signed On " << QDateTime::fromTime_t(sign->timestamp).toString() << Qt::endl;

        stream << Qt::endl << "[>] Signatures:" << Qt::endl;

        bool canContinue = true;

        while (sign && canContinue) {

            switch (gpg_err_code(sign->status)) {
                case GPG_ERR_BAD_SIGNATURE:
                    stream << QApplication::tr("One or More Bad Signatures.") << Qt::endl;
                    canContinue = false;
                    setStatus(-1);
                    break;
                case GPG_ERR_NO_ERROR:
                    stream  << QApplication::tr("A ");
                    if (sign->summary & GPGME_SIGSUM_GREEN) {
                        stream << QApplication::tr("Good ");
                    }
                    if (sign->summary & GPGME_SIGSUM_RED) {
                        stream << QApplication::tr("Bad ");
                    }
                    if (sign->summary & GPGME_SIGSUM_SIG_EXPIRED) {
                        stream << QApplication::tr("Expired ");
                    }
                    if (sign->summary & GPGME_SIGSUM_KEY_MISSING) {
                        stream << QApplication::tr("Missing Key's ");
                    }
                    if (sign->summary & GPGME_SIGSUM_KEY_REVOKED) {
                        stream << QApplication::tr("Revoked Key's ");
                    }
                    if (sign->summary & GPGME_SIGSUM_KEY_EXPIRED) {
                        stream << QApplication::tr("Expired Key's ");
                    }
                    if (sign->summary & GPGME_SIGSUM_CRL_MISSING) {
                        stream << QApplication::tr("Missing CRL's ");
                    }

                    if (sign->summary & GPGME_SIGSUM_VALID) {
                        stream << QApplication::tr("Signature Fully Valid.") << Qt::endl;
                    } else {
                        stream << QApplication::tr("Signature NOT Fully Valid.") << Qt::endl;
                    }

                    if (!(sign->status & GPGME_SIGSUM_KEY_MISSING)) {
                        if (!printSigner(stream, sign)) {
                            setStatus(0);
                        }
                    } else {
                        stream << QApplication::tr("Key is NOT present with ID 0x") << QString(sign->fpr) << Qt::endl;
                    }

                    setStatus(1);

                    break;
                case GPG_ERR_NO_PUBKEY:
                    stream << QApplication::tr("A signature could NOT be verified due to a Missing Key\n");
                    setStatus(-1);
                    break;
                case GPG_ERR_CERT_REVOKED:
                    stream << QApplication::tr(
                            "A signature is valid but the key used to verify the signature has been revoked\n");
                    if (!printSigner(stream, sign)) {
                        setStatus(0);
                    }
                    setStatus(-1);
                    break;
                case GPG_ERR_SIG_EXPIRED:
                    stream << QApplication::tr("A signature is valid but expired\n");
                    if (!printSigner(stream, sign)) {
                        setStatus(0);
                    }
                    setStatus(-1);
                    break;
                case GPG_ERR_KEY_EXPIRED:
                    stream << QApplication::tr(
                            "A signature is valid but the key used to verify the signature has expired.\n");
                    if (!printSigner(stream, sign)) {
                        setStatus(0);
                    }
                    break;
                case GPG_ERR_GENERAL:
                    stream << QApplication::tr(
                            "There was some other error which prevented the signature verification.\n");
                    status = -1;
                    canContinue = false;
                    break;
                default:
                    stream << QApplication::tr("Error for key with fingerprint ") <<
                           GpgME::GpgContext::beautifyFingerprint(QString(sign->fpr));
                    setStatus(-1);
            }
            stream << Qt::endl;
            sign = sign->next;
        }
        stream << "<------------" << Qt::endl;
    }
}

bool VerifyResultAnalyse::printSigner(QTextStream &stream, gpgme_signature_t sign) {
    bool keyFound = true;
    stream << QApplication::tr("Signed By: ");
    auto key = mCtx->getKeyByFpr(sign->fpr);
    if(!key.good) {
        stream << tr("<Unknown>");
        setStatus(0);
        keyFound = false;
    }
    stream << key.name;
    if (!key.email.isEmpty()) {
        stream << "<" << key.email <<  ">";
    }
    stream << Qt::endl;
    return keyFound;

}