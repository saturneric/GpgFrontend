/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "gpg/GpgKey.h"

void GpgKey::parse(gpgme_key_t key) {
    if(key != nullptr) {
        good = true;
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

        last_update = QDateTime(QDateTime::fromTime_t(key->last_update));

        switch (key->owner_trust) {
            case GPGME_VALIDITY_UNKNOWN:
                owner_trust = "Unknown";
                break;
            case GPGME_VALIDITY_UNDEFINED:
                owner_trust = "Undefined";
                break;
            case GPGME_VALIDITY_NEVER:
                owner_trust = "Never";
                break;
            case GPGME_VALIDITY_MARGINAL:
                owner_trust = "Marginal";
                break;
            case GPGME_VALIDITY_FULL:
                owner_trust = "FULL";
                break;
            case GPGME_VALIDITY_ULTIMATE:
                owner_trust = "Ultimate";
                break;
        }

        auto uid = key->uids;

        while(uid != nullptr) {
            uids.push_back(UID(uid));
            uid = uid->next;
        }


        if (!uids.isEmpty()) {
            name = uids.first().name;
            email = uids.first().email;
            comment = uids.first().comment;
        }

        auto next = key->subkeys;

        while (next != nullptr) {
            subKeys.push_back(GpgSubKey(next));
            next = next->next;
        }

        if (!subKeys.isEmpty()) {
            id = subKeys.first().id;
            expires = subKeys.first().expires;
            pubkey_algo = subKeys.first().pubkey_algo;
            create_time = subKeys.first().timestamp;
            length = subKeys.first().length;
        } else {
            id = "";
        }
    }
}
