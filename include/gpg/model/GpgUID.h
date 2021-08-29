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

#ifndef GPGFRONTEND_GPGUID_H
#define GPGFRONTEND_GPGUID_H

#include <utility>

#include "GpgFrontend.h"
#include "GpgKeySignature.h"

namespace GpgFrontend {

    class GpgUID {
    public:

        [[nodiscard]] QString name() const { return _uid_ref->name; }

        [[nodiscard]] QString email() const { return _uid_ref->email; }

        [[nodiscard]] QString comment() const { return _uid_ref->comment; }

        [[nodiscard]] QString uid() const { return _uid_ref->uid; }

        [[nodiscard]] QString hash() const { return _uid_ref->uidhash; }

        [[nodiscard]] bool revoked() const { return _uid_ref->revoked; }

        [[nodiscard]] bool invalid() const { return _uid_ref->invalid; }

        [[nodiscard]] std::unique_ptr<QVector<GpgKeySignature>> signatures() const {
            auto sigs = std::make_unique<QVector<GpgKeySignature>>();
            auto sig_next = _uid_ref->signatures;
            while (sig_next != nullptr) {
                sigs->push_back(GpgKeySignature(sig_next));
                sig_next = sig_next->next;
            }
            return sigs;
        }

        GpgUID() = default;

        explicit GpgUID(gpgme_user_id_t uid);

        GpgUID(GpgUID &&o) noexcept {swap(_uid_ref, o._uid_ref);}

        GpgUID(const GpgUID &) = delete;

        GpgUID& operator=(GpgUID &&o) noexcept {
            swap(_uid_ref, o._uid_ref);
            return *this;
        }

        GpgUID& operator=(const GpgUID &) = delete;

    private:

        using UidRefHandler = std::unique_ptr<struct _gpgme_user_id, std::function<void(gpgme_user_id_t)>>;

        UidRefHandler _uid_ref = nullptr;

    };

}

#endif //GPGFRONTEND_GPGUID_H