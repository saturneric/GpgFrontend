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

#ifndef GPGFRONTEND_GPGSIGNATURE_H
#define GPGFRONTEND_GPGSIGNATURE_H

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include "core/GpgConstants.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgSignature {
 public:
  /**
   * @brief
   *
   * @return gpgme_validity_t
   */
  [[nodiscard]] gpgme_validity_t GetValidity() const;

  /**
   * @brief
   *
   * @return gpgme_error_t
   */
  [[nodiscard]] gpgme_error_t GetStatus() const;

  /**
   * @brief
   *
   * @return gpgme_error_t
   */
  [[nodiscard]] gpgme_error_t GetSummary() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetPubkeyAlgo() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetHashAlgo() const;

  /**
   * @brief Create a time object
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
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetFingerprint() const;

  /**
   * @brief Construct a new Gpg Signature object
   *
   */
  GpgSignature();

  /**
   * @brief Destroy the Gpg Signature object
   *
   */
  ~GpgSignature();

  /**
   * @brief Construct a new Gpg Signature object
   *
   * @param sig
   */
  explicit GpgSignature(gpgme_signature_t sig);

  /**
   * @brief Construct a new Gpg Signature object
   *
   */
  GpgSignature(GpgSignature &&) noexcept;

  /**
   * @brief Construct a new Gpg Signature object
   *
   */
  GpgSignature(const GpgSignature &) = delete;

  /**
   * @brief
   *
   * @return GpgSignature&
   */
  GpgSignature &operator=(GpgSignature &&) noexcept;

  /**
   * @brief
   *
   * @return GpgSignature&
   */
  GpgSignature &operator=(const GpgSignature &) = delete;

 private:
  using KeySignatrueRefHandler =
      std::unique_ptr<struct _gpgme_signature,
                      std::function<void(gpgme_signature_t)>>;  ///<

  KeySignatrueRefHandler signature_ref_ = nullptr;  ///<
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGSIGNATURE_H
