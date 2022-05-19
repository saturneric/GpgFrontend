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

#include "GpgSubKey.h"
#include "GpgUID.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKey {
 public:
  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsGood() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetId() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetName() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetEmail() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetComment() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetFingerprint() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetProtocol() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetOwnerTrust() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetPublicKeyAlgo() const;

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetLastUpdateTime() const;

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetExpireTime() const;

  /**
   * @brief Create a time object
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetCreateTime() const;

  /**
   * @brief s
   *
   * @return unsigned int
   */
  [[nodiscard]] unsigned int GetPrimaryKeyLength() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasEncryptionCapability() const;

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
  [[nodiscard]] bool IsHasSigningCapability() const;

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
  [[nodiscard]] bool IsHasCertificationCapability() const;

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
  [[nodiscard]] bool IsHasAuthenticationCapability() const;

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
  [[nodiscard]] bool IsHasCardKey() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsPrivateKey() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsExpired() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsRevoked() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsDisabled() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsHasMasterKey() const;

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
  bool operator==(const GpgKey& o) const;

  /**
   * @brief
   *
   * @param o
   * @return true
   * @return false
   */
  bool operator<=(const GpgKey& o) const;

  /**
   * @brief
   *
   * @return gpgme_key_t
   */
  explicit operator gpgme_key_t() const;

  /**
   * @brief
   *
   * @return GpgKey
   */
  [[nodiscard]] GpgKey Copy() const;

 private:
  /**
   * @brief
   *
   */
  struct GPGFRONTEND_CORE_EXPORT _key_ref_deleter {
    void operator()(gpgme_key_t _key);
  };

  using KeyRefHandler =
      std::unique_ptr<struct _gpgme_key, _key_ref_deleter>;  ///<

  KeyRefHandler key_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGKEY_H
