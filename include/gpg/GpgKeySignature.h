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

struct GpgKeySignature {

    bool revoked{};
    bool expired{};
    bool invalid{};
    bool exportable{};

    gpgme_error_t status{};

    QString keyid;
    QString pubkey_algo;

    QDateTime create_time;
    QDateTime expire_time;

    QString uid;
    QString name;
    QString email;
    QString comment;

    gpgme_sigsum_t summary;

    GpgKeySignature() = default;

    explicit GpgKeySignature(gpgme_key_sig_t key_sig);

    GpgKeySignature(GpgKeySignature &&) noexcept = default;
    GpgKeySignature(const GpgKeySignature &) = default;
    GpgKeySignature& operator=(GpgKeySignature &&) noexcept = default;
    GpgKeySignature& operator=(const GpgKeySignature &) = default;

};


#endif //GPGFRONTEND_GPGKEYSIGNATURE_H
