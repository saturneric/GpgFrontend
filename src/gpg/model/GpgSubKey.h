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
#ifndef GPGFRONTEND_GPGSUBKEY_H
#define GPGFRONTEND_GPGSUBKEY_H

#include <boost/date_time.hpp>
#include <string>

#include "gpg/GpgConstants.h"

namespace GpgFrontend {

class GpgSubKey {
 public:
  [[nodiscard]] std::string id() const { return _subkey_ref->keyid; }

  [[nodiscard]] std::string fpr() const { return _subkey_ref->fpr; }

  [[nodiscard]] std::string pubkey_algo() const {
    return gpgme_pubkey_algo_name(_subkey_ref->pubkey_algo);
  }

  [[nodiscard]] unsigned int length() const { return _subkey_ref->length; }

  [[nodiscard]] bool can_encrypt() const { return _subkey_ref->can_encrypt; }

  [[nodiscard]] bool can_sign() const { return _subkey_ref->can_sign; }

  [[nodiscard]] bool can_certify() const { return _subkey_ref->can_certify; }

  [[nodiscard]] bool can_authenticate() const {
    return _subkey_ref->can_authenticate;
  }

  [[nodiscard]] bool is_private_key() const { return _subkey_ref->secret; }

  [[nodiscard]] bool expired() const { return _subkey_ref->expired; }

  [[nodiscard]] bool revoked() const { return _subkey_ref->revoked; }

  [[nodiscard]] bool disabled() const { return _subkey_ref->disabled; }

  [[nodiscard]] bool secret() const { return _subkey_ref->secret; }

  [[nodiscard]] bool is_cardkey() const { return _subkey_ref->is_cardkey; }

  [[nodiscard]] boost::gregorian::date timestamp() const {
    return boost::posix_time::from_time_t(_subkey_ref->timestamp).date();
  }

  [[nodiscard]] boost::gregorian::date expires() const {
    return boost::posix_time::from_time_t(_subkey_ref->expires).date();
  }

  GpgSubKey() = default;

  explicit GpgSubKey(gpgme_subkey_t subkey);

  GpgSubKey(GpgSubKey&& o) noexcept { swap(_subkey_ref, o._subkey_ref); }

  GpgSubKey(const GpgSubKey&) = delete;

  GpgSubKey& operator=(GpgSubKey&& o) noexcept {
    swap(_subkey_ref, o._subkey_ref);
    return *this;
  };

  GpgSubKey& operator=(const GpgSubKey&) = delete;

  bool operator==(const GpgSubKey& o) const { return fpr() == o.fpr(); }

 private:
  using SubkeyRefHandler = std::unique_ptr<struct _gpgme_subkey,
                                           std::function<void(gpgme_subkey_t)>>;

  SubkeyRefHandler _subkey_ref = nullptr;
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGSUBKEY_H
