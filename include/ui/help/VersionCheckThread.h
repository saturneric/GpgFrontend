//
// Created by Administrator on 2021/7/12.
//

#ifndef GPGFRONTEND_VERSIONCHECKTHREAD_H
#define GPGFRONTEND_VERSIONCHECKTHREAD_H

#include "GpgFrontend.h"

class VersionCheckThread : public QThread {
Q_OBJECT

public:

    VersionCheckThread(QNetworkReply *networkReply);

signals:

    void upgradeVersion(const QString &currentVersion, const QString &latestVersion);

protected:

    void run() override;

private:

    QNetworkReply* mNetworkReply;

};


#endif //GPGFRONTEND_VERSIONCHECKTHREAD_H
