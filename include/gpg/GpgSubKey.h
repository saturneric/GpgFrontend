//
// Created by eric on 2021/5/21.
//

#ifndef GPGFRONTEND_GPGSUBKEY_H
#define GPGFRONTEND_GPGSUBKEY_H


#include <gpgme.h>

struct GpgSubKey {

    QString id;
    QString fpr;

    bool can_encrypt{};
    bool can_sign{};
    bool can_certify{};
    bool can_authenticate{};


    bool is_private_key{};
    bool expired{};
    bool revoked{};
    bool disabled{};
    bool is_cardkey{};

    QDateTime timestamp;
    QDateTime expires;

    GpgSubKey() = default;

    explicit GpgSubKey(gpgme_subkey_t key) {

        id = key->keyid;
        fpr = key->fpr;

        expired = (key->expired != 0u);
        revoked = (key->revoked != 0u);

        disabled = key->disabled;

        can_authenticate = key->can_authenticate;
        can_certify = key->can_certify;
        can_encrypt = key->can_encrypt;
        can_sign = key->can_sign;
        is_cardkey = key->is_cardkey;
        is_private_key = key->secret;

        timestamp = QDateTime::fromTime_t(key->timestamp);
        expires = QDateTime::fromTime_t(key->expires);
    }

};


#endif //GPGFRONTEND_GPGSUBKEY_H
