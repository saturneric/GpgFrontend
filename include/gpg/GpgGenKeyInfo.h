//
// Created by eric on 2021/5/13.
//

#ifndef GPG4USB_GPGGENKEYINFO_H
#define GPG4USB_GPGGENKEYINFO_H

#include <QString>
#include <QTime>

struct GenKeyInfo {
    bool isSubKey = false;
    QString userid;
    QString algo;
    int keySize;
    QDateTime expired;
    bool nonExpired = false;
    bool allowSigning = true;
    bool allowEncryption = true;
    QString passPhrase;
};

#endif //GPG4USB_GPGGENKEYINFO_H
