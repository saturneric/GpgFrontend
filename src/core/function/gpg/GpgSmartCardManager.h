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
#include "core/function/gpg/GpgComponentManager.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GpgOpenPGPCard.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GpgSubKey;

/**
 * @brief
 *
 */
class GF_CORE_EXPORT GpgSmartCardManager
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
  auto ModifyAttr(const QString& attr, const QString& value)
      -> std::tuple<GpgError, QString>;

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
                   const QDateTime& expire, bool non_expire)
      -> std::tuple<GpgError, QString>;

  /**
   * @brief Move an existing on-disk (sub)key onto the OpenPGP smart card
   * @p serial_number, via the gpg-agent `KEYTOCARD` Assuan command.
   *
   * This is destructive: gpg-agent replaces the on-disk private key with a card
   * stub, so callers must warn/back up first. @p subkey_index indexes
   * `key->SubKeys()` (index 0 is the primary). @p card_slot selects the target
   * slot (1 = signature, 2 = encryption, 3 = authentication) and must match one
   * of the key's capabilities (see CandidateSlots). For ECDH encryption keys
   * the required KDF parameters are derived from the public key automatically.
   *
   * @return {GPG_ERR_NO_ERROR, status} on success; {error, message} otherwise
   */
  auto MoveKeyToCard(const GpgKeyPtr& key, int subkey_index,
                     const QString& serial_number, int card_slot)
      -> std::tuple<GpgError, QString>;

  /**
   * @brief The card slots a (sub)key may be stored in, derived from its
   * capabilities: signature/certify -> 1, encryption -> 2, authentication -> 3.
   */
  static auto CandidateSlots(const GpgSubKey& skey) -> QList<int>;

  /**
   * @brief Build the gpg-agent `KEYTOCARD` command line. Pure/side-effect free
   * so it can be unit-tested without a card. An empty @p ecdh is omitted; an
   * empty
   * @p serial becomes the "-" (no-check) placeholder.
   */
  static auto BuildKeyToCardCommand(const QString& hexgrip,
                                    const QString& serial, int slot,
                                    const QString& timestamp,
                                    const QString& ecdh) -> QString;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto IsSCDVersionSupported() -> bool;

 private:
  OpenPGPContext& ctx_ =
      OpenPGPContext::GetInstance(SingletonFunctionObject::GetChannel());  ///<
  GpgAssuanHelper& assuan_ =
      GpgAssuanHelper::GetInstance(SingletonFunctionObject::GetChannel());  ///<
  GpgComponentManager& info_ = GpgComponentManager::GetInstance(
      SingletonFunctionObject::GetChannel());  ///<

  QString cached_scd_serialno_status_hash_;
  QContainer<QString> cache_scd_card_serial_numbers_;
};

}  // namespace GpgFrontend
