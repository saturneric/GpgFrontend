/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <mutex>

#include "core/model/GpgSubKey.h"
#include "core/model/GpgUID.h"

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
  [[nodiscard]] auto IsGood() const -> bool;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetId() const -> std::string;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetName() const -> std::string;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetEmail() const -> std::string;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetComment() const -> std::string;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetFingerprint() const -> std::string;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetProtocol() const -> std::string;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetOwnerTrust() const -> std::string;

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] auto GetOwnerTrustLevel() const -> int;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] auto GetPublicKeyAlgo() const -> std::string;

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] auto GetLastUpdateTime() const -> boost::posix_time::ptime;

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] auto GetExpireTime() const -> boost::posix_time::ptime;

  /**
   * @brief Create a time object
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] auto GetCreateTime() const -> boost::posix_time::ptime;

  /**
   * @brief s
   *
   * @return unsigned int
   */
  [[nodiscard]] auto GetPrimaryKeyLength() const -> unsigned int;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasEncryptionCapability() const -> bool;

  /**
   * @brief

   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasActualEncryptionCapability() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasSigningCapability() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasActualSigningCapability() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasCertificationCapability() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasActualCertificationCapability() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasAuthenticationCapability() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasActualAuthenticationCapability() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasCardKey() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsPrivateKey() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsExpired() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsRevoked() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsDisabled() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasMasterKey() const -> bool;

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgSubKey>>
   */
  [[nodiscard]] auto GetSubKeys() const
      -> std::unique_ptr<std::vector<GpgSubKey>>;

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgUID>>
   */
  [[nodiscard]] auto GetUIDs() const -> std::unique_ptr<std::vector<GpgUID>>;

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
  GpgKey(GpgKey&&) noexcept;

  /**
   * @brief
   *
   * @param k
   * @return GpgKey&
   */
  auto operator=(GpgKey&&) noexcept -> GpgKey&;

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param k
   */
  GpgKey(const GpgKey&);

  /**
   * @brief
   *
   * @param k
   * @return GpgKey&
   */
  auto operator=(const GpgKey&) -> GpgKey&;

  /**
   * @brief
   *
   * @param key
   * @return GpgKey&
   */
  auto operator=(const gpgme_key_t&) -> GpgKey& = delete;

  /**
   * @brief
   *
   * @param o
   * @return true
   * @return false
   */
  auto operator==(const GpgKey&) const -> bool;

  /**
   * @brief
   *
   * @param o
   * @return true
   * @return false
   */
  auto operator<=(const GpgKey&) const -> bool;

  /**
   * @brief
   *
   * @return gpgme_key_t
   */
  explicit operator gpgme_key_t() const;

 private:
  /**
   * @brief
   *
   */
  struct GPGFRONTEND_CORE_EXPORT KeyRefDeleter {
    void operator()(gpgme_key_t _key);
  };

  using KeyRefHandler = std::unique_ptr<struct _gpgme_key, KeyRefDeleter>;  ///<

  KeyRefHandler key_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend
