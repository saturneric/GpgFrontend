//
// Created by eric on 2021/6/7.
//

#ifndef GPGFRONTEND_VERIFYRESULTANALYSE_H
#define GPGFRONTEND_VERIFYRESULTANALYSE_H

#include "gpg/GpgContext.h"
#include "gpg/GpgKeySignature.h"

class VerifyResultAnalyse {
public:

    explicit VerifyResultAnalyse(GpgME::GpgContext *ctx, gpgme_signature_t signature);

    [[nodiscard]] const QString &getResultReport() const;

    [[nodiscard]] int getStatus() const;

private:

    GpgME::GpgContext *mCtx;
    QString verifyLabelText;
    QTextStream textSteam{&verifyLabelText};

    int status = 1;

    bool printSigner(QTextStream &stream, gpgme_signature_t sign);

    void setStatus(int mStatus);

};


#endif //GPGFRONTEND_VERIFYRESULTANALYSE_H
