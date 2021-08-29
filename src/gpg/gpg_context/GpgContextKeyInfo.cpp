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

#include "gpg/GpgContext.h"

/**
 * check if key can sign(actually)
 * @param key target key
 * @return if key sign
 */
bool GpgFrontend::GpgContext::checkIfKeyCanSign(const GpgKey &key) {
    auto subkeys = key.subKeys();
    if (std::any_of(subkeys->begin(), subkeys->end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.secret() && subkey.can_sign() && !subkey.disabled() && !subkey.revoked() && !subkey.expired();
    }))
        return true;
    else return false;
}

/**
 * check if key can certify(actually)
 * @param key target key
 * @return if key certify
 */
bool GpgFrontend::GpgContext::checkIfKeyCanCert(const GpgKey &key) {
    return key.has_master_key() && !key.expired() && !key.revoked() && !key.disabled();
}

/**
 * check if key can authenticate(actually)
 * @param key target key
 * @return if key authenticate
 */
bool GpgFrontend::GpgContext::checkIfKeyCanAuth(const GpgKey &key) {
    auto subkeys = key.subKeys();
    if (std::any_of(subkeys->begin(), subkeys->end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.secret() && subkey.can_authenticate() && !subkey.disabled() && !subkey.revoked() && !subkey.expired();
    }))
        return true;
    else return false;
}

/**
 * check if key can encrypt(actually)
 * @param key target key
 * @return if key encrypt
 */
bool GpgFrontend::GpgContext::checkIfKeyCanEncr(const GpgKey &key) {
    auto subkeys = key.subKeys();
    if (std::any_of(subkeys->begin(), subkeys->end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.can_encrypt() && !subkey.disabled() && !subkey.revoked() && !subkey.expired();
    }))
        return true;
    else return false;
}
