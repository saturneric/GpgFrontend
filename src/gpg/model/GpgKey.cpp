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

#include "gpg/model/GpgKey.h"

GpgFrontend::GpgKey::GpgKey(gpgme_key_t &&key) : _key_ref(std::move(key)) {}

GpgFrontend::GpgKey::GpgKey(GpgKey &&k) noexcept { swap(_key_ref, k._key_ref); }

GpgFrontend::GpgKey &GpgFrontend::GpgKey::operator=(GpgKey &&k) noexcept {
  swap(_key_ref, k._key_ref);
  return *this;
}

std::unique_ptr<std::vector<GpgFrontend::GpgSubKey>>
GpgFrontend::GpgKey::subKeys() const {
  auto p_keys = std::make_unique<std::vector<GpgSubKey>>();
  auto next = _key_ref->subkeys;
  while (next != nullptr) {
    p_keys->push_back(std::move(GpgSubKey(next)));
    next = next->next;
  }
  return p_keys;
}

std::unique_ptr<std::vector<GpgFrontend::GpgUID>>
GpgFrontend::GpgKey::uids() const {
  auto p_uids = std::make_unique<std::vector<GpgUID>>();
  auto uid_next = _key_ref->uids;
  while (uid_next != nullptr) {
    p_uids->push_back(std::move(GpgUID(uid_next)));
    uid_next = uid_next->next;
  }
  return p_uids;
}

bool GpgFrontend::GpgKey::CanSignActual() const {
  auto subkeys = subKeys();
  if (std::any_of(subkeys->begin(), subkeys->end(),
                  [](const GpgSubKey &subkey) -> bool {
                    return subkey.secret() && subkey.can_sign() &&
                           !subkey.disabled() && !subkey.revoked() &&
                           !subkey.expired();
                  }))
    return true;
  else
    return false;
}

bool GpgFrontend::GpgKey::CanAuthActual() const {
  auto subkeys = subKeys();
  if (std::any_of(subkeys->begin(), subkeys->end(),
                  [](const GpgSubKey &subkey) -> bool {
                    return subkey.secret() && subkey.can_authenticate() &&
                           !subkey.disabled() && !subkey.revoked() &&
                           !subkey.expired();
                  }))
    return true;
  else
    return false;
}

/**
 * check if key can certify(actually)
 * @param key target key
 * @return if key certify
 */
bool GpgFrontend::GpgKey::CanCertActual() const {
  return has_master_key() && !expired() && !revoked() && !disabled();
}

/**
 * check if key can encrypt(actually)
 * @param key target key
 * @return if key encrypt
 */
bool GpgFrontend::GpgKey::CanEncrActual() const {
  auto subkeys = subKeys();
  if (std::any_of(subkeys->begin(), subkeys->end(),
                  [](const GpgSubKey &subkey) -> bool {
                    return subkey.can_encrypt() && !subkey.disabled() &&
                           !subkey.revoked() && !subkey.expired();
                  }))
    return true;
  else
    return false;
}
