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
#include "core/struct/cache_object/KeyGroupCO.h"

namespace GpgFrontend {

class GpgKeyGroupGetter;

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeyGroup : public GpgAbstractKey {
 public:
  /**
   * @brief Construct a new Gpg Key object
   *
   */
  GpgKeyGroup();

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param key
   */
  GpgKeyGroup(QString name, QString email, QString comment,
              QStringList key_ids);

  /**
   * @brief Construct a new Gpg Key Group object
   *
   * @param kg_co
   */
  explicit GpgKeyGroup(const KeyGroupCO& kg_co);

  /**
   * @brief Construct a new Gpg Key object
   *
   * @param k
   */
  GpgKeyGroup(const GpgKeyGroup&);

  /**
   * @brief Destroy the Gpg Key objects
   *
   */
  virtual ~GpgKeyGroup() override;

  /**
   * @brief
   *
   * @param k
   * @return GpgKey&
   */
  auto operator=(const GpgKeyGroup&) -> GpgKeyGroup&;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto KeyType() const -> GpgAbstractKeyType override;

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
  [[nodiscard]] auto Name() const -> QString override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Email() const -> QString override;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto Comment() const -> QString override;

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
  [[nodiscard]] auto ExpirationTime() const -> QDateTime override;

  /**
   * @brief Create a time object
   *
   * @return QDateTime
   */
  [[nodiscard]] auto CreationTime() const -> QDateTime override;

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
   * @return KeyGroupCO
   */
  [[nodiscard]] auto ToCacheObject() const -> KeyGroupCO;

  /**
   * @brief
   *
   * @return KeyGroupCO
   */
  [[nodiscard]] auto KeyIds() const -> QStringList;

  /**
   * @brief Set the Key Ids object
   *
   */
  void SetKeyIds(QStringList);

  /**
   * @brief Set the Disabled object
   *
   */
  void SetKeyGroupGetter(GpgKeyGroupGetter*);

 private:
  QString id_;
  QString name_;
  QString email_;
  QString comment_;
  QStringList key_ids_;
  QDateTime creation_time_;

  GpgKeyGroupGetter* getter_ = nullptr;
};

}  // namespace GpgFrontend
