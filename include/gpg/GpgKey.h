//
// Created by eric on 2021/5/21.
//

#ifndef GPGFRONTEND_GPGKEY_H
#define GPGFRONTEND_GPGKEY_H

#include <gpgme.h>

#include "GpgSubKey.h"

class GpgKey {
public:
    GpgKey() {
        is_private_key = false;
    }

    QString id;
    QString name;
    QString email;
    QString fpr;
    QString protocol;
    int owner_trust;
    QDateTime last_update;

    bool can_encrypt{};
    bool can_sign{};
    bool can_certify{};
    bool can_authenticate{};


    bool is_private_key{};
    bool expired{};
    bool revoked{};
    bool disabled{};

    QVector<GpgSubKey> subKeys;

    GpgKey(gpgme_key_t key) {
        is_private_key = key->secret;
        fpr = key->fpr;
        protocol = key->protocol;
        expired = (key->expired != 0u);
        revoked = (key->revoked != 0u);

        disabled = key->disabled;

        can_authenticate = key->can_authenticate;
        can_certify = key->can_certify;
        can_encrypt = key->can_encrypt;
        can_sign = key->can_sign;

        last_update =  QDateTime(QDateTime::fromTime_t(key->last_update));
        owner_trust = key->owner_trust;

        if (key->uids) {
            name = QString::fromUtf8(key->uids->name);
            email = QString::fromUtf8(key->uids->email);
        }

        gpgme_subkey_t next = key->subkeys;

        while(next != nullptr) {
            subKeys.push_back(GpgSubKey(next));
            next = next->next;
        }

        if(!subKeys.isEmpty()) {
            id = subKeys.first().id;
        } else {
            id = "";
        }


    }
};


#endif //GPGFRONTEND_GPGKEY_H
