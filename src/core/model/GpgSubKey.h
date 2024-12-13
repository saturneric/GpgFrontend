/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include <gpgme.h>

#include "core/GpgFrontendCoreExport.h"

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
   * @return QString
   */
  [[nodiscard]] auto GetID() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetFingerprint() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetPubkeyAlgo() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetKeyAlgo() const -> QString;

  /**
   * @brief
   *
   * @return unsigned int
   */
  [[nodiscard]] auto GetKeyLength() const -> unsigned int;

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
  [[nodiscard]] auto IsHasSigningCapability() const -> bool;

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
  [[nodiscard]] auto IsHasAuthenticationCapability() const -> bool;

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
  [[nodiscard]] auto IsSecretKey() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsCardKey() const -> bool;

  /**
   * @brief
   *
   * @return QDateTime
   */
  [[nodiscard]] auto GetCreateTime() const -> QDateTime;

  /**
   * @brief
   *
   * @return QDateTime
   */
  [[nodiscard]] QDateTime GetExpireTime() const;

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
   */
  GpgSubKey(const GpgSubKey&);

  /**
   * @brief
   *
   * @return GpgSubKey&
   */
  auto operator=(const GpgSubKey&) -> GpgSubKey&;

  /**
   * @brief
   *
   * @param o
   * @return true
   * @return false
   */
  auto operator==(const GpgSubKey& o) const -> bool;

 private:
  gpgme_subkey_t subkey_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend
