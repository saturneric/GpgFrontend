//
// Created by eric on 2021/5/22.
//

#include "gpg/GpgUID.h"

GpgUID::GpgUID(gpgme_user_id_t user_id) :
        uid(user_id->uid), name(user_id->name), email(user_id->email), comment(user_id->comment),
        revoked(user_id->revoked), invalid(user_id->invalid) {

    auto sig = user_id->signatures;

    while (sig != nullptr) {
        signatures.push_back(GpgKeySignature(sig));
        sig = sig->next;
    }

}