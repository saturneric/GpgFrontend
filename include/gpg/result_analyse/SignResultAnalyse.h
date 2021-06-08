//
// Created by eric on 2021/6/8.
//

#ifndef GPGFRONTEND_SIGNRESULTANALYSE_H
#define GPGFRONTEND_SIGNRESULTANALYSE_H

#include "GpgFrontend.h"

class SignResultAnalyse {
public:
    SignResultAnalyse(gpgme_sign_result_t result);

    [[nodiscard]] const QString &getResultReport() const;

    [[nodiscard]] int getStatus() const;

private:
    QString resultText;
    QTextStream stream{&resultText};

    int status = 1;

    void setStatus(int mStatus) {
        if(mStatus < status) status = mStatus;
    }


};

#endif //GPGFRONTEND_SIGNRESULTANALYSE_H
