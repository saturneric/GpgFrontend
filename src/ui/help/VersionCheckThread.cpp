//
// Created by Administrator on 2021/7/12.
//

#include "ui/help/VersionCheckThread.h"
#include "GpgFrontendBuildInfo.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

void VersionCheckThread::run() {
    qDebug() << "Start Version Thread to get latest version from Github";

    auto currentVersion = "v" + QString::number(VERSION_MAJOR) + "."
                          + QString::number(VERSION_MINOR) + "."
                          + QString::number(VERSION_PATCH);

    while(mNetworkReply->isRunning()) {
        QApplication::processEvents();
    }

    QByteArray bytes = mNetworkReply->readAll();

    Document d;
    d.Parse(bytes.constData());

    QString latestVersion = d["tag_name"].GetString();

    qDebug() << "Latest Version From Github" << latestVersion;

    QRegularExpression re("^[vV](\\d+\\.)?(\\d+\\.)?(\\*|\\d+)");
    QRegularExpressionMatch match = re.match(latestVersion);
    if (match.hasMatch()) {
        latestVersion = match.captured(0); // matched == "23 def"
        qDebug() << "Latest Version Matched" << latestVersion;
    } else {
        latestVersion = currentVersion;
        qDebug() << "Latest Version Unknown" << latestVersion;
    }

    if(latestVersion != currentVersion) {
        emit upgradeVersion(currentVersion, latestVersion);
    }

}

VersionCheckThread::VersionCheckThread(QNetworkReply *networkReply):mNetworkReply(networkReply) {

}
