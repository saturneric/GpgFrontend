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

#ifndef GPGFRONTEND_GPGSUBKEY_H
#define GPGFRONTEND_GPGSUBKEY_H

#include <boost/date_time.hpp>
#include <string>

#include "core/GpgConstants.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgSubKey {
 public:
  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetID() const;

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
  [[nodiscard]] std::string GetPubkeyAlgo() const;

  /**
   * @brief
   *
   * @return unsigned int
   */
  [[nodiscard]] unsigned int GetKeyLength() const;

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
  [[nodiscard]] bool IsHasSigningCapability() const;

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
  [[nodiscard]] bool IsHasAuthenticationCapability() const;

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
  [[nodiscard]] bool IsSecretKey() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsCardKey() const;

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetCreateTime() const;

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetExpireTime() const;

  /**
   * @brief Construct a new Gpg Sub Key object
   *
   */
  GpgSubKey();

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
  GpgSubKey(GpgSubKey&& o) noexcept;

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
  GpgSubKey& operator=(GpgSubKey&& o) noexcept;

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
  bool operator==(const GpgSubKey& o) const;

 private:
  using SubkeyRefHandler =
      std::unique_ptr<struct _gpgme_subkey,
                      std::function<void(gpgme_subkey_t)>>;  ///<

  SubkeyRefHandler _subkey_ref = nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGSUBKEY_H
