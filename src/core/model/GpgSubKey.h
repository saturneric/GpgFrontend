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
#include "core/model/GpgAbstractKey.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgSubKey : public GpgAbstractKey {
 public:
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
  explicit GpgSubKey(QSharedPointer<struct _gpgme_key> key_ref,
                     gpgme_subkey_t s_key);

  /**
   * @brief Construct a new Gpg Sub Key object
   *
   */
  GpgSubKey(const GpgSubKey&);

  /**
   * @brief Destroy the Gpg Sub Key object
   *
   */
  virtual ~GpgSubKey() override;

  /**
   * @brief
   *
   * @return GpgSubKey&
   */
  auto operator=(const GpgSubKey&) -> GpgSubKey&;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto ID() const -> QString override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Fingerprint() const -> QString override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsSubKey() const -> bool override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsGood() const -> bool override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto PublicKeyAlgo() const -> QString override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Algo() const -> QString override;

  /**
   * @brief
   *
   * @return unsigned int
   */
  [[nodiscard]] auto KeyLength() const -> unsigned int;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasEncrCap() const -> bool override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasSignCap() const -> bool override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasCertCap() const -> bool override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsHasAuthCap() const -> bool override;

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
  [[nodiscard]] auto IsExpired() const -> bool override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsRevoked() const -> bool override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsDisabled() const -> bool override;

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
  [[nodiscard]] auto CreationTime() const -> QDateTime override;

  /**
   * @brief
   *
   * @return QDateTime
   */
  [[nodiscard]] auto ExpirationTime() const -> QDateTime override;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsADSK() const -> bool;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto SmartCardSerialNumber() -> QString;

 private:
  QSharedPointer<struct _gpgme_key> key_ref_;
  gpgme_subkey_t s_key_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend
