//
// Created by eric on 2021/6/7.
//

#include "GpgFrontend.h"
#include "gpg/result_analyse/VerifyResultAnalyse.h"

VerifyResultAnalyse::VerifyResultAnalyse(GpgME::GpgContext *ctx, gpgme_signature_t sign) : mCtx(ctx) {

    textSteam << "Verify Report: " << endl;

    if (sign == nullptr){
        textSteam << "> Not Signature Found" << endl;
        status = -1;
        return;
    }


    textSteam << "> It was Signed ON " << QDateTime::fromTime_t(sign->timestamp).toString() << endl;

    textSteam << endl << "> It Contains:" << endl;

    bool canContinue = true;

    while (sign && canContinue) {

        switch (gpg_err_code(sign->status)) {
            case GPG_ERR_BAD_SIGNATURE:
                textSteam << QApplication::tr("One or More Bad Signatures.") << endl;
                canContinue = false;
                setStatus(-1);
                break;
            case GPG_ERR_NO_ERROR:
                textSteam << QApplication::tr("A ");
                if(sign->summary & GPGME_SIGSUM_GREEN) {
                    textSteam << QApplication::tr("Good ");
                }
                if(sign->summary & GPGME_SIGSUM_RED) {
                    textSteam << QApplication::tr("Bad ");
                }
                if(sign->summary & GPGME_SIGSUM_SIG_EXPIRED) {
                    textSteam << QApplication::tr("Expired ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_MISSING) {
                    textSteam << QApplication::tr("Missing Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_REVOKED) {
                    textSteam << QApplication::tr("Revoked Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_KEY_EXPIRED) {
                    textSteam << QApplication::tr("Expired Key's ");
                }
                if(sign->summary & GPGME_SIGSUM_CRL_MISSING) {
                    textSteam << QApplication::tr("Missing CRL's ");
                }

                if(sign->summary & GPGME_SIGSUM_VALID) {
                    textSteam << QApplication::tr("Signature Fully Valid.") << endl;
                } else {
                    textSteam << QApplication::tr("Signature NOT Fully Valid.") << endl;
                }

                if(!(sign->status & GPGME_SIGSUM_KEY_MISSING)) {
                    if(!printSigner(textSteam, sign)) {
                        setStatus(0);
                    }
                } else {
                    textSteam << QApplication::tr("Key is NOT present with ID 0x") << QString(sign->fpr) << endl;
                }

                setStatus(1);

                break;
            case GPG_ERR_NO_PUBKEY:
                textSteam << QApplication::tr("A signature could NOT be verified due to a Missing Key\n");
                setStatus(-1);
                break;
            case GPG_ERR_CERT_REVOKED:
                textSteam << QApplication::tr("A signature is valid but the key used to verify the signature has been revoked\n");
                if(!printSigner(textSteam, sign)) {
                    setStatus(0);
                }
                setStatus(-1);
                break;
            case GPG_ERR_SIG_EXPIRED:
                textSteam << QApplication::tr("A signature is valid but expired\n");
                if(!printSigner(textSteam, sign)) {
                    setStatus(0);
                }
                setStatus(-1);
                break;
            case GPG_ERR_KEY_EXPIRED:
                textSteam << QApplication::tr("A signature is valid but the key used to verify the signature has expired.\n");
                if(!printSigner(textSteam, sign)) {
                    setStatus(0);
                }
                break;
            case GPG_ERR_GENERAL:
                textSteam << QApplication::tr("There was some other error which prevented the signature verification.\n");
                status = -1;
                canContinue = false;
                break;
            default:
                textSteam << QApplication::tr("Error for key with fingerprint ") <<
                          GpgME::GpgContext::beautifyFingerprint(QString(sign->fpr));
                setStatus(-1);
        }
        textSteam << endl;
        sign = sign->next;
    }
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

const QString &VerifyResultAnalyse::getResultReport() const{
    return verifyLabelText;
}

int VerifyResultAnalyse::getStatus() const {
    return status;
}

void VerifyResultAnalyse::setStatus(int mStatus) {
    if(mStatus < status)
        status = mStatus;
}
