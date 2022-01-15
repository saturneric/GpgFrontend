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

/**
 * @brief
 *
 */
class GpgSubKey {
 public:
  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetID() const { return _subkey_ref->keyid; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetFingerprint() const { return _subkey_ref->fpr; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetPubkeyAlgo() const {
    return gpgme_pubkey_algo_name(_subkey_ref->pubkey_algo);
  }

  /**
   * @brief
   *
   * @return unsigned int
   */
  [[nodiscard]] unsigned int GetKeyLength() const {
    return _subkey_ref->length;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasEncryptionCapability() const {
    return _subkey_ref->can_encrypt;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasSigningCapability() const {
    return _subkey_ref->can_sign;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasCertificationCapability() const {
    return _subkey_ref->can_certify;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasAuthenticationCapability() const {
    return _subkey_ref->can_authenticate;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsPrivateKey() const { return _subkey_ref->secret; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsExpired() const { return _subkey_ref->expired; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsRevoked() const { return _subkey_ref->revoked; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsDisabled() const { return _subkey_ref->disabled; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsSecretKey() const { return _subkey_ref->secret; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsCardKey() const { return _subkey_ref->is_cardkey; }

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetCreateTime() const {
    return boost::posix_time::from_time_t(_subkey_ref->timestamp);
  }

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetExpireTime() const {
    return boost::posix_time::from_time_t(_subkey_ref->expires);
  }

  /**
   * @brief Construct a new Gpg Sub Key object
   *
   */
  GpgSubKey() = default;

  /**
   * @brief Construct a new Gpg Sub Key object
   *
   * @param subkey
   */
  explicit GpgSubKey(gpgme_subkey_t subkey);

  /**
   * @brief Construct a new Gpg Sub Key object
   *
   * @param o
   */
  GpgSubKey(GpgSubKey&& o) noexcept { swap(_subkey_ref, o._subkey_ref); }

  /**
   * @brief Construct a new Gpg Sub Key object
   *
   */
  GpgSubKey(const GpgSubKey&) = delete;

  /**
   * @brief
   *
   * @param o
   * @return GpgSubKey&
   */
  GpgSubKey& operator=(GpgSubKey&& o) noexcept {
    swap(_subkey_ref, o._subkey_ref);
    return *this;
  };

  /**
   * @brief
   *
   * @return GpgSubKey&
   */
  GpgSubKey& operator=(const GpgSubKey&) = delete;

  /**
   * @brief
   *
   * @param o
   * @return true
   * @return false
   */
  bool operator==(const GpgSubKey& o) const {
    return GetFingerprint() == o.GetFingerprint();
  }

 private:
  using SubkeyRefHandler =
      std::unique_ptr<struct _gpgme_subkey,
                      std::function<void(gpgme_subkey_t)>>;  ///<

  SubkeyRefHandler _subkey_ref = nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGSUBKEY_H
