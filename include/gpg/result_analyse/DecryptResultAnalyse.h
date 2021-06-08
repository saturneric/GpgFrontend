//
// Created by eric on 2021/6/9.
//

#ifndef GPGFRONTEND_DECRYPTRESULTANALYSE_H
#define GPGFRONTEND_DECRYPTRESULTANALYSE_H

#include "gpg/GpgContext.h"
#include "ResultAnalyse.h"

class DecryptResultAnalyse: public ResultAnalyse{
public:
    explicit DecryptResultAnalyse(GpgME::GpgContext *ctx, gpgme_error_t error, gpgme_decrypt_result_t result);

private:

    GpgME::GpgContext *mCtx;

    bool printReci(QTextStream &stream, gpgme_recipient_t reci);
};


#endif //GPGFRONTEND_DECRYPTRESULTANALYSE_H
