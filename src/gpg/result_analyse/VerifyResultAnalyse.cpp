//
// Created by eric on 2021/6/7.
//

#include "GpgFrontend.h"
#include "gpg/result_analyse/VerifyResultAnalyse.h"

VerifyResultAnalyse::VerifyResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_verify_result_t result)
        : mCtx(ctx) {

    if(result == nullptr) {
        return;
    }

    stream << "# Verify Report: " << endl << "-----" << endl;
    stream << "Status: " << gpgme_strerror(error) << endl;

    auto sign = result->signatures;

    if (sign == nullptr){
        stream << "> Not Signature Found" << endl;
        status = -1;
        return;
    }


    stream << "> It was Signed ON " << QDateTime::fromTime_t(sign->timestamp).toString() << endl;

    stream << endl << "> It Contains:" << endl;

    bool canContinue = true;

    while (sign && canContinue) {

        switch (gpg_err_code(sign->status)) {
            case GPG_ERR_BAD_SIGNATURE:
                stream << QApplication::tr("One or More Bad Signatures.") << endl;
                canContinue = false;
                setStatus(-1);
                break;
            case GPG_ERR_NO_ERROR:
                stream << QApplication::tr("A ");
                if(sign->summary & GPGME_SIGSUM_GREEN) {
                    stream << QApplication::tr("Good ");
                }
                if(sign->summary & GPGME_SIGSUM_RED) {
                    stream << QApplication::tr("Bad ");
                }
                if(sign->summary & GPGME_SIGSUM_SIG_EXPIRED) {
                    stream << QApplication::tr("Expired ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_MISSING) {
                    stream << QApplication::tr("Missing Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_REVOKED) {
                    stream << QApplication::tr("Revoked Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_EXPIRED) {
                    stream << QApplication::tr("Expired Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_CRL_MISSING) {
                    stream << QApplication::tr("Missing CRL's ");
                }

                if(sign->summary & GPGME_SIGSUM_VALID) {
                    stream << QApplication::tr("Signature Fully Valid.") << endl;
                } else {
                    stream << QApplication::tr("Signature NOT Fully Valid.") << endl;
                }

                if(!(sign->status & GPGME_SIGSUM_KEY_MISSING)) {
                    if(!printSigner(stream, sign)) {
                        setStatus(0);
                    }
                } else {
                    stream << QApplication::tr("Key is NOT present with ID 0x") << QString(sign->fpr) << endl;
                }

                setStatus(1);

                break;
            case GPG_ERR_NO_PUBKEY:
                stream << QApplication::tr("A signature could NOT be verified due to a Missing Key\n");
                setStatus(-1);
                break;
            case GPG_ERR_CERT_REVOKED:
                stream << QApplication::tr("A signature is valid but the key used to verify the signature has been revoked\n");
                if(!printSigner(stream, sign)) {
                    setStatus(0);
                }
                setStatus(-1);
                break;
            case GPG_ERR_SIG_EXPIRED:
                stream << QApplication::tr("A signature is valid but expired\n");
                if(!printSigner(stream, sign)) {
                    setStatus(0);
                }
                setStatus(-1);
                break;
            case GPG_ERR_KEY_EXPIRED:
                stream << QApplication::tr("A signature is valid but the key used to verify the signature has expired.\n");
                if(!printSigner(stream, sign)) {
                    setStatus(0);
                }
                break;
            case GPG_ERR_GENERAL:
                stream << QApplication::tr("There was some other error which prevented the signature verification.\n");
                status = -1;
                canContinue = false;
                break;
            default:
                stream << QApplication::tr("Error for key with fingerprint ") <<
                       GpgME::GpgContext::beautifyFingerprint(QString(sign->fpr));
                setStatus(-1);
        }
        stream << endl;
        sign = sign->next;
    }

    stream << "-----" << endl;
    stream << endl;
}

bool VerifyResultAnalyse::printSigner(QTextStream &stream, gpgme_signature_t sign) {
    bool keyFound = true;
    stream << QApplication::tr("Signed By: ");
    auto key = mCtx->getKeyByFpr(sign->fpr);
    if(!key.good) {
        stream << "<Unknown>";
        setStatus(0);
        keyFound = false;
    }
    stream << key.name;
    if (!key.email.isEmpty()) {
        stream << "<" << key.email <<  ">";
    }
    stream << endl;
    return keyFound;

}