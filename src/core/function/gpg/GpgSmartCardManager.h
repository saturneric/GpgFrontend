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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/gpg/GpgAssuanHelper.h"
#include "core/function/gpg/GpgComponentInfoGetter.h"
#include "core/function/gpg/GpgContext.h"
#include "core/model/GpgOpenPGPCard.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgSmartCardManager
    : public SingletonFunctionObject<GpgSmartCardManager> {
 public:
  /**
   * @brief Construct a new Gpg Key Manager object
   *
   * @param channel
   */
  explicit GpgSmartCardManager(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Get the Serial Numbers object
   *
   * @return QStringList
   */
  auto GetSerialNumbers() -> QStringList;

  /**
   * @brief
   *
   * @return std::tuple<bool, QString>
   */
  auto SelectCardBySerialNumber(const QString&)
      -> std::tuple<GpgError, QString>;

  /**
   * @brief
   *
   * @return QSharedPointer<GpgOpenPGPCard>
   */
  auto FetchCardInfoBySerialNumber(const QString&)
      -> QSharedPointer<GpgOpenPGPCard>;

  /**
   * @brief
   *
   * @param key
   * @param subkey_index
   * @return true
   * @return false
   */
  auto Fetch(const QString& serial_number) -> GpgError;

  /**
   * @brief
   *
   * @return std::tuple<bool, QString>
   */
  auto ModifyAttr(const QString& attr,
                  const QString& value) -> std::tuple<GpgError, QString>;

  /**
   * @brief
   *
   * @param pin_ref
   * @return std::tuple<bool, QString>
   */
  auto ModifyPin(const QString& pin_ref) -> std::tuple<GpgError, QString>;

  /**
   * @brief
   *
   * @return auto
   */
  auto GenerateKey(const QString& serial_number, const QString& name,
                   const QString& email, const QString& comment,
                   const QDateTime& expire,
                   bool non_expire) -> std::tuple<GpgError, QString>;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto IsSCDVersionSupported() -> bool;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
  GpgAssuanHelper& assuan_ =
      GpgAssuanHelper::GetInstance(SingletonFunctionObject::GetChannel());  ///<
  GpgComponentInfoGetter& info_ = GpgComponentInfoGetter::GetInstance(
      SingletonFunctionObject::GetChannel());  ///<

  QString cached_scd_serialno_status_hash_;
  QContainer<QString> cache_scd_card_serial_numbers_;
};

}  // namespace GpgFrontend
