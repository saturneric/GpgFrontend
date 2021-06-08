//
// Created by eric on 2021/6/8.
//

#ifndef GPGFRONTEND_SIGNRESULTANALYSE_H
#define GPGFRONTEND_SIGNRESULTANALYSE_H

#include "GpgFrontend.h"

#include "ResultAnalyse.h"

class SignResultAnalyse : public ResultAnalyse{
public:
    explicit SignResultAnalyse(gpgme_error_t error, gpgme_sign_result_t result);


private:

};

#endif //GPGFRONTEND_SIGNRESULTANALYSE_H
