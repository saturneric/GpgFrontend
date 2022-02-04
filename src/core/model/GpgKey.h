/**
 * Copyright (C) 2021 Saturneric
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_GPGKEY_H
#define GPGFRONTEND_GPGKEY_H

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include "GpgSubKey.h"
#include "GpgUID.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GpgKey {
 public:
  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsGood() const { return key_ref_ != nullptr; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetId() const { return key_ref_->subkeys->keyid; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetName() const { return key_ref_->uids->name; };

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetEmail() const { return key_ref_->uids->email; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetComment() const {
    return key_ref_->uids->comment;
  }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetFingerprint() const { return key_ref_->fpr; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetProtocol() const {
    return gpgme_get_protocol_name(key_ref_->protocol);
  }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetOwnerTrust() const {
    switch (key_ref_->owner_trust) {
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

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetPublicKeyAlgo() const {
    return gpgme_pubkey_algo_name(key_ref_->subkeys->pubkey_algo);
  }

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetLastUpdateTime() const {
    return boost::posix_time::from_time_t(
        static_cast<time_t>(key_ref_->last_update));
  }

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetExpireTime() const {
    return boost::posix_time::from_time_t(key_ref_->subkeys->expires);
  };

  /**
   * @brief Create a time object
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetCreateTime() const {
    return boost::posix_time::from_time_t(key_ref_->subkeys->timestamp);
  };

  /**
   * @brief s
   *
   * @return unsigned int
   */
  [[nodiscard]] unsigned int GetPrimaryKeyLength() const {
    return key_ref_->subkeys->length;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasEncryptionCapability() const {
    return key_ref_->can_encrypt;
  }

  /**
   * @brief

   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasActualEncryptionCapability() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasSigningCapability() const {
    return key_ref_->can_sign;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasActualSigningCapability() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasCertificationCapability() const {
    return key_ref_->can_certify;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasActualCertificationCapability() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasAuthenticationCapability() const {
    return key_ref_->can_authenticate;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasActualAuthenticationCapability() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasCardKey() const {
    auto subkeys = GetSubKeys();
    return std::any_of(
        subkeys->begin(), subkeys->end(),
        [](const GpgSubKey& subkey) -> bool { return subkey.IsCardKey(); });
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsPrivateKey() const { return key_ref_->secret; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsExpired() const { return key_ref_->expired; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsRevoked() const { return key_ref_->revoked; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsDisabled() const { return key_ref_->disabled; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasMasterKey() const {
    return key_ref_->subkeys->secret;
  }

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgSubKey>>
   */
  [[nodiscard]] std::unique_ptr<std::vector<GpgSubKey>> GetSubKeys() const;

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgUID>>
   */
  [[nodiscard]] std::unique_ptr<std::vector<GpgUID>> GetUIDs() const;

  /**
   * @brief Construct a new Gpg Key object
   *
   */
  GpgKey() = default;

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param key
   */
  explicit GpgKey(gpgme_key_t&& key);

  /**
   * @brief Destroy the Gpg Key objects
   *
   */
  ~GpgKey() = default;

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param key
   */
  GpgKey(const gpgme_key_t& key) = delete;

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param k
   */
  GpgKey(GpgKey&& k) noexcept;

  /**
   * @brief
   *
   * @param k
   * @return GpgKey&
   */
  GpgKey& operator=(GpgKey&& k) noexcept;

  /**
   * @brief
   *
   * @param key
   * @return GpgKey&
   */
  GpgKey& operator=(const gpgme_key_t& key) = delete;

  /**
   * @brief
   *
   * @param o
   * @return true
   * @return false
   */
  bool operator==(const GpgKey& o) const { return o.GetId() == this->GetId(); }

  /**
   * @brief
   *
   * @param o
   * @return true
   * @return false
   */
  bool operator<=(const GpgKey& o) const { return this->GetId() < o.GetId(); }

  /**
   * @brief
   *
   * @return gpgme_key_t
   */
  explicit operator gpgme_key_t() const { return key_ref_.get(); }

  /**
   * @brief
   *
   * @return GpgKey
   */
  [[nodiscard]] GpgKey Copy() const {
    gpgme_key_ref(key_ref_.get());
    auto* _new_key_ref = key_ref_.get();
    return GpgKey(std::move(_new_key_ref));
  }

 private:
  /**
   * @brief
   *
   */
  struct _key_ref_deleter {
    void operator()(gpgme_key_t _key) {
      if (_key != nullptr) gpgme_key_unref(_key);
    }
  };

  using KeyRefHandler =
      std::unique_ptr<struct _gpgme_key, _key_ref_deleter>;  ///<

  KeyRefHandler key_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGKEY_H
