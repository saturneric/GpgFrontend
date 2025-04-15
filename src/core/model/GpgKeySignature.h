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

/**
 * @brief
 *
 */
#include "core/typedef/GpgErrorTypedef.h"
namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgKeySignature {
 public:
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
  [[nodiscard]] auto IsExpired() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsInvalid() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsExportable() const -> bool;

  /**
   * @brief
   *
   * @return gpgme_error_t
   */
  [[nodiscard]] auto GetStatus() const -> GpgError;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetKeyID() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetPubkeyAlgo() const -> QString;

  /**
   * @brief Create a time object
   *
   * @return QDateTime
   */
  [[nodiscard]] auto GetCreateTime() const -> QDateTime;

  /**
   * @brief
   *
   * @return QDateTime
   */
  [[nodiscard]] auto GetExpireTime() const -> QDateTime;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetUID() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetName() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetEmail() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto GetComment() const -> QString;

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   */
  GpgKeySignature();

  /**
   * @brief Destroy the Gpg Key Signature object
   *
   */
  ~GpgKeySignature();

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   * @param sig
   */
  explicit GpgKeySignature(gpgme_key_sig_t sig);

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   */
  GpgKeySignature(const GpgKeySignature &);

  /**
   * @brief
   *
   * @return GpgKeySignature&
   */
  auto operator=(const GpgKeySignature &) -> GpgKeySignature &;

 private:
  gpgme_key_sig_t signature_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend
