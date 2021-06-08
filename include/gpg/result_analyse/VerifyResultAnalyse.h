//
// Created by eric on 2021/6/7.
//

#ifndef GPGFRONTEND_VERIFYRESULTANALYSE_H
#define GPGFRONTEND_VERIFYRESULTANALYSE_H

#include "gpg/GpgContext.h"
#include "gpg/GpgKeySignature.h"

#include "ResultAnalyse.h"

class VerifyResultAnalyse : public ResultAnalyse{
public:

    explicit VerifyResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_verify_result_t result);

private:

    GpgME::GpgContext *mCtx;

    bool printSigner(QTextStream &stream, gpgme_signature_t sign);

};


#endif //GPGFRONTEND_VERIFYRESULTANALYSE_H
