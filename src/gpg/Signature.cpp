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

#include "gpg/Signature.h"

Signature::Signature(gpgme_key_sig_t key_sig) :
        revoked(key_sig->revoked), expired(key_sig->expired), invalid(key_sig->invalid),
        exportable(key_sig->exportable), status(key_sig->status),
        keyid(key_sig->keyid), pubkey_algo(gpgme_pubkey_algo_name(key_sig->pubkey_algo)),
        uid(key_sig->uid), name(key_sig->name), email(key_sig->email), comment(key_sig->comment),
        create_time(QDateTime::fromTime_t(key_sig->timestamp)), expire_time(QDateTime::fromTime_t(key_sig->expires)){

}
