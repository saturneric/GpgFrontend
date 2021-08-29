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

#ifndef GPGFRONTEND_GPGKEYSIGNATURE_H
#define GPGFRONTEND_GPGKEYSIGNATURE_H

#include "GpgFrontend.h"

namespace GpgFrontend {

    class GpgKeySignature {
    public:

        [[nodiscard]] bool revoked() const { return _signature_ref->revoked; }
        [[nodiscard]] bool expired() const { return _signature_ref->expired; }
        [[nodiscard]] bool invalid() const { return _signature_ref->invalid; }
        [[nodiscard]] bool exportable() const { return _signature_ref->exportable; }

        [[nodiscard]] gpgme_error_t status() const { return _signature_ref->status; }

        [[nodiscard]] std::string keyid() const { return _signature_ref->keyid; }
        [[nodiscard]] std::string pubkey_algo() const { return gpgme_pubkey_algo_name(_signature_ref->pubkey_algo); }

        [[nodiscard]] QDateTime create_time() const { return QDateTime::fromTime_t(_signature_ref->timestamp); }
        [[nodiscard]] QDateTime expire_time() const { return QDateTime::fromTime_t(_signature_ref->expires); }

        [[nodiscard]] std::string uid() const { return _signature_ref->uid; }
        [[nodiscard]] std::string name() const { return _signature_ref->name; }
        [[nodiscard]] std::string email() const { return _signature_ref->email; }
        [[nodiscard]] std::string comment() const { return _signature_ref->comment; }

        GpgKeySignature() = default;

        explicit GpgKeySignature(gpgme_key_sig_t sig);

        GpgKeySignature(GpgKeySignature &&) noexcept = default;

        GpgKeySignature(const GpgKeySignature &) = delete;

        GpgKeySignature &operator=(GpgKeySignature &&) noexcept = default;

        GpgKeySignature &operator=(const GpgKeySignature &) = delete;

    private:

        using KeySignatrueRefHandler = std::unique_ptr<struct _gpgme_key_sig, std::function<void(gpgme_key_sig_t)>>;

        KeySignatrueRefHandler _signature_ref = nullptr;
    };

}


#endif //GPGFRONTEND_GPGKEYSIGNATURE_H
