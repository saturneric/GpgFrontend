//
// Created by eric on 2021/6/9.
//

#ifndef GPGFRONTEND_ENCRYPTRESULTANALYSE_H
#define GPGFRONTEND_ENCRYPTRESULTANALYSE_H

#include "ResultAnalyse.h"

class EncryptResultAnalyse : public ResultAnalyse{
public:
    explicit EncryptResultAnalyse(gpgme_error_t error, gpgme_encrypt_result_t result);

};


#endif //GPGFRONTEND_ENCRYPTRESULTANALYSE_H
