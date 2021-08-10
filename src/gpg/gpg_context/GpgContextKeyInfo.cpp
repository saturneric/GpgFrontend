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
bool GpgME::GpgContext::checkIfKeyCanSign(const GpgKey &key) {
    if (std::any_of(key.subKeys.begin(), key.subKeys.end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.secret && subkey.can_sign && !subkey.disabled && !subkey.revoked && !subkey.expired;
    }))
        return true;
    else return false;
}

/**
 * check if key can certify(actually)
 * @param key target key
 * @return if key certify
 */
bool GpgME::GpgContext::checkIfKeyCanCert(const GpgKey &key) {
    return key.has_master_key && !key.expired && !key.revoked && !key.disabled;
}

/**
 * check if key can authenticate(actually)
 * @param key target key
 * @return if key authenticate
 */
bool GpgME::GpgContext::checkIfKeyCanAuth(const GpgKey &key) {
    if (std::any_of(key.subKeys.begin(), key.subKeys.end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.secret && subkey.can_authenticate && !subkey.disabled && !subkey.revoked && !subkey.expired;
    }))
        return true;
    else return false;
}

/**
 * check if key can encrypt(actually)
 * @param key target key
 * @return if key encrypt
 */
bool GpgME::GpgContext::checkIfKeyCanEncr(const GpgKey &key) {
    if (std::any_of(key.subKeys.begin(), key.subKeys.end(), [](const GpgSubKey &subkey) -> bool {
        return subkey.can_encrypt && !subkey.disabled && !subkey.revoked && !subkey.expired;
    }))
        return true;
    else return false;
}

/**
 * Get target key
 * @param fpr master key's fingerprint
 * @return the key
 */
GpgKey GpgME::GpgContext::getKeyByFpr(const QString &fpr) {
    for (const auto &key : mKeyList) {
        if (key.fpr == fpr) return key;
        else
            for (auto &subkey : key.subKeys) {
                if (subkey.fpr == fpr) return key;
            }
    }
    return GpgKey(nullptr);
}


/**
 * Get target key
 * @param id master key's id
 * @return the key
 */
const GpgKey &GpgME::GpgContext::getKeyById(const QString &id) {

    for (const auto &key : mKeyList) {
        if (key.id == id)
            return key;
        else {
            auto sub_keys = key.subKeys;
            for (const auto &subkey : sub_keys) {
                if (subkey.id == id) return key;
            }
        }
    }

    throw std::runtime_error("key not found");
}
