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

#ifndef GPGFRONTEND_GPGKEYSIGNATURE_H
#define GPGFRONTEND_GPGKEYSIGNATURE_H

#include <boost/date_time.hpp>
#include <string>

#include "core/GpgConstants.h"

/**
 * @brief
 *
 */
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
  [[nodiscard]] bool IsRevoked() const;

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
  [[nodiscard]] bool IsInvalid() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsExportable() const;

  /**
   * @brief
   *
   * @return gpgme_error_t
   */
  [[nodiscard]] gpgme_error_t GetStatus() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetKeyID() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetPubkeyAlgo() const;

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
  [[nodiscard]] std::string GetUID() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetName() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetEmail() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetComment() const;

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
  GpgKeySignature(GpgKeySignature &&) noexcept;

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   */
  GpgKeySignature(const GpgKeySignature &) = delete;

  /**
   * @brief
   *
   * @return GpgKeySignature&
   */
  GpgKeySignature &operator=(GpgKeySignature &&) noexcept;

  /**
   * @brief
   *
   * @return GpgKeySignature&
   */
  GpgKeySignature &operator=(const GpgKeySignature &) = delete;

 private:
  using KeySignatrueRefHandler =
      std::unique_ptr<struct _gpgme_key_sig,
                      std::function<void(gpgme_key_sig_t)>>;  ///<

  KeySignatrueRefHandler signature_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGKEYSIGNATURE_H
