//
// Created by eric on 2021/6/8.
//

#include "gpg/result_analyse/ResultAnalyse.h"

const QString &ResultAnalyse::getResultReport() const{
    return resultText;
}

int ResultAnalyse::getStatus() const {
    return status;
}

void ResultAnalyse::setStatus(int mStatus) {
    if(mStatus < status)
        status = mStatus;
}
