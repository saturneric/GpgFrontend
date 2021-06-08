//
// Created by eric on 2021/6/8.
//

#ifndef GPGFRONTEND_RESULTANALYSE_H
#define GPGFRONTEND_RESULTANALYSE_H

#include "GpgFrontend.h"

class ResultAnalyse {
public:
    ResultAnalyse() = default;

    [[nodiscard]] const QString &getResultReport() const;

    [[nodiscard]] int getStatus() const;

protected:
    QString resultText;
    QTextStream stream{&resultText};

    int status = 1;

    void setStatus(int mStatus);
};


#endif //GPGFRONTEND_RESULTANALYSE_H
