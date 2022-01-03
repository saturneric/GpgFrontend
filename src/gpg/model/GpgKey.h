/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
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

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include "GpgSubKey.h"
#include "GpgUID.h"

namespace GpgFrontend {

class GpgKey {
 public:
  [[nodiscard]] bool good() const { return _key_ref != nullptr; }

  [[nodiscard]] std::string id() const { return _key_ref->subkeys->keyid; }

  [[nodiscard]] std::string name() const { return _key_ref->uids->name; };

  [[nodiscard]] std::string email() const { return _key_ref->uids->email; }

  [[nodiscard]] std::string comment() const { return _key_ref->uids->comment; }

  [[nodiscard]] std::string fpr() const { return _key_ref->fpr; }

  [[nodiscard]] std::string protocol() const {
    return gpgme_get_protocol_name(_key_ref->protocol);
  }

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
    return "Invalid";
  }

  [[nodiscard]] std::string pubkey_algo() const {
    return gpgme_pubkey_algo_name(_key_ref->subkeys->pubkey_algo);
  }

  [[nodiscard]] boost::posix_time::ptime last_update() const {
    return boost::posix_time::from_time_t(
        static_cast<time_t>(_key_ref->last_update));
  }

  [[nodiscard]] boost::posix_time::ptime expires() const {
    return boost::posix_time::from_time_t(_key_ref->subkeys->expires);
  };

  [[nodiscard]] boost::posix_time::ptime create_time() const {
    return boost::posix_time::from_time_t(_key_ref->subkeys->timestamp);
  };

  [[nodiscard]] unsigned int length() const {
    return _key_ref->subkeys->length;
  }

  [[nodiscard]] bool can_encrypt() const { return _key_ref->can_encrypt; }

  [[nodiscard]] bool CanEncrActual() const;

  [[nodiscard]] bool can_sign() const { return _key_ref->can_sign; }

  [[nodiscard]] bool CanSignActual() const;

  [[nodiscard]] bool can_certify() const { return _key_ref->can_certify; }

  [[nodiscard]] bool CanCertActual() const;

  [[nodiscard]] bool can_authenticate() const {
    return _key_ref->can_authenticate;
  }

  [[nodiscard]] bool CanAuthActual() const;

  [[nodiscard]] bool HasCardKey() const {
    auto subkeys = subKeys();
    return std::any_of(
        subkeys->begin(), subkeys->end(),
        [](const GpgSubKey& subkey) -> bool { return subkey.is_cardkey(); });
  }

  [[nodiscard]] bool is_private_key() const { return _key_ref->secret; }

  [[nodiscard]] bool expired() const { return _key_ref->expired; }

  [[nodiscard]] bool revoked() const { return _key_ref->revoked; }

  [[nodiscard]] bool disabled() const { return _key_ref->disabled; }

  [[nodiscard]] bool has_master_key() const {
    return _key_ref->subkeys->secret;
  }

  [[nodiscard]] std::unique_ptr<std::vector<GpgSubKey>> subKeys() const;

  [[nodiscard]] std::unique_ptr<std::vector<GpgUID>> uids() const;

  GpgKey() = default;

  explicit GpgKey(gpgme_key_t&& key);

  ~GpgKey() = default;

  GpgKey(const gpgme_key_t& key) = delete;

  GpgKey(GpgKey&& k) noexcept;

  GpgKey& operator=(GpgKey&& k) noexcept;

  GpgKey& operator=(const gpgme_key_t& key) = delete;

  bool operator==(const GpgKey& o) const { return o.id() == this->id(); }

  bool operator<=(const GpgKey& o) const { return this->id() < o.id(); }

  explicit operator gpgme_key_t() const { return _key_ref.get(); }

  [[nodiscard]] GpgKey copy() const {
    gpgme_key_ref(_key_ref.get());
    auto* _new_key_ref = _key_ref.get();
    return GpgKey(std::move(_new_key_ref));
  }

 private:
  struct _key_ref_deletor {
    void operator()(gpgme_key_t _key) {
      if (_key != nullptr) gpgme_key_unref(_key);
    }
  };

  using KeyRefHandler = std::unique_ptr<struct _gpgme_key, _key_ref_deletor>;

  KeyRefHandler _key_ref = nullptr;
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGKEY_H
