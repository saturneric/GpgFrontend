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

#include "core/model/GpgAbstractKey.h"
#include "core/model/GpgSubKey.h"
#include "core/model/GpgUID.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKey : public GpgAbstractKey {
  Q_DECLARE_TR_FUNCTIONS(GpgKey)
 public:
  /**
   * @brief Construct a new Gpg Key object
   *
   */
  GpgKey();

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param key
   */
  explicit GpgKey(gpgme_key_t key);

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param k
   */
  GpgKey(const GpgKey&);

  /**
   * @brief Destroy the Gpg Key objects
   *
   */
  virtual ~GpgKey() override;

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
   * @return gpgme_key_t
   */
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator gpgme_key_t() const;

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
  [[nodiscard]] auto ID() const -> QString override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Name() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Email() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Comment() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Fingerprint() const -> QString override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Protocol() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto OwnerTrust() const -> QString;

  /**
   * @brief
   *
   * @return int
   */
  [[nodiscard]] auto OwnerTrustLevel() const -> int;

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
   * @return QDateTime
   */
  [[nodiscard]] auto LastUpdateTime() const -> QDateTime;

  /**
   * @brief
   *
   * @return QDateTime
   */
  [[nodiscard]] auto ExpirationTime() const -> QDateTime override;

  /**
   * @brief Create a time object
   *
   * @return QDateTime
   */
  [[nodiscard]] auto CreationTime() const -> QDateTime override;

  /**
   * @brief s
   *
   * @return unsigned int
   */
  [[nodiscard]] auto PrimaryKeyLength() const -> unsigned int;

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
  [[nodiscard]] auto IsHasActualEncrCap() const -> bool;

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
  [[nodiscard]] auto IsHasActualSignCap() const -> bool;

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
  [[nodiscard]] auto IsHasActualCertCap() const -> bool;

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
  [[nodiscard]] auto IsHasActualAuthCap() const -> bool;

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
  [[nodiscard]] auto IsPrivateKey() const -> bool override;

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
  [[nodiscard]] auto IsHasMasterKey() const -> bool;

  /**
   * @brief
   *
   * @return std::unique_ptr<QContainer<GpgSubKey>>
   */
  [[nodiscard]] auto SubKeys() const -> QContainer<GpgSubKey>;

  /**
   * @brief
   *
   * @return std::unique_ptr<QContainer<GpgUID>>
   */
  [[nodiscard]] auto UIDs() const -> QContainer<GpgUID>;

  /**
   * @brief  the Primary Key object
   *
   * @return GpgSubKey
   */
  [[nodiscard]] auto PrimaryKey() const -> GpgSubKey;

 private:
  QSharedPointer<struct _gpgme_key> key_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend
