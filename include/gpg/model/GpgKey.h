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

#ifndef GPGFRONTEND_GPGKEY_H
#define GPGFRONTEND_GPGKEY_H

#include "GpgUID.h"
#include "GpgSubKey.h"

namespace GpgFrontend {

    class GpgKey {
    public:

        [[nodiscard]] bool good() const { return _key_ref == nullptr; }

        [[nodiscard]] std::string id() const { return _key_ref->subkeys->keyid; }

        [[nodiscard]] std::string name() const { return _key_ref->uids->name; };

        [[nodiscard]] std::string email() const { return _key_ref->uids->email; }

        [[nodiscard]] std::string comment() const { return _key_ref->uids->comment; }

        [[nodiscard]] std::string fpr() const { return _key_ref->fpr; }

        [[nodiscard]] std::string protocol() const { return gpgme_get_protocol_name(_key_ref->protocol); }

        [[nodiscard]] std::string owner_trust() const {
            switch (_key_ref->owner_trust) {
                case GPGME_VALIDITY_UNKNOWN:
                    return "Unknown";
                case GPGME_VALIDITY_UNDEFINED:
                    return "Undefined";
                case GPGME_VALIDITY_NEVER:
                    return "Never";
                case GPGME_VALIDITY_MARGINAL:
                    return "Marginal";
                case GPGME_VALIDITY_FULL:
                    return "FULL";
                case GPGME_VALIDITY_ULTIMATE:
                    return "Ultimate";
            }
        }

        [[nodiscard]] std::string pubkey_algo() const { return gpgme_pubkey_algo_name(_key_ref->subkeys->pubkey_algo); }

        [[nodiscard]] QDateTime last_update() const { return QDateTime::fromTime_t(_key_ref->last_update); }

        [[nodiscard]] QDateTime expires() const { return QDateTime::fromTime_t(_key_ref->subkeys->expires); };

        [[nodiscard]] QDateTime create_time() const { return QDateTime::fromTime_t(_key_ref->subkeys->timestamp); };

        [[nodiscard]] unsigned int length() const { return _key_ref->subkeys->length; }


        [[nodiscard]] bool can_encrypt() const { return _key_ref->can_encrypt; }

        [[nodiscard]] bool can_sign() const { return _key_ref->can_sign; }

        [[nodiscard]] bool canSignActual() const;

        [[nodiscard]] bool can_certify() const { return _key_ref->can_certify; }

        [[nodiscard]] bool can_authenticate() const { return _key_ref->can_authenticate; }


        [[nodiscard]] bool is_private_key() const { return _key_ref->secret; }

        [[nodiscard]] bool expired() const { return _key_ref->expired; }

        [[nodiscard]] bool revoked() const { return _key_ref->revoked; }

        [[nodiscard]] bool disabled() const { return _key_ref->disabled; }

        [[nodiscard]] bool has_master_key() const { return _key_ref->subkeys->secret; }

        [[nodiscard]] std::unique_ptr<std::vector<GpgSubKey>> subKeys() const;

        [[nodiscard]] std::unique_ptr<std::vector<GpgUID>> uids() const;

        explicit GpgKey() = default;

        explicit GpgKey(gpgme_key_t &&key);

        GpgKey(const gpgme_key_t &key) = delete;

        GpgKey(GpgKey &&k) noexcept;

        GpgKey &operator=(GpgKey &&k) noexcept;

        GpgKey &operator=(const gpgme_key_t &key) = delete;

        explicit operator gpgme_key_t() const { return _key_ref.get(); }

    private:

        using KeyRefHandler = std::unique_ptr<struct _gpgme_key, std::function<void(gpgme_key_t)>>;

        KeyRefHandler _key_ref;
    };

}


#endif //GPGFRONTEND_GPGKEY_H
