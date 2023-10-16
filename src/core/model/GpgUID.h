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

#ifndef GPGFRONTEND_GPGUID_H
#define GPGFRONTEND_GPGUID_H

#include "GpgKeySignature.h"
#include "GpgTOFUInfo.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgUID {
 public:
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
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetUID() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool GetRevoked() const;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool GetInvalid() const;

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgTOFUInfo>>
   */
  [[nodiscard]] std::unique_ptr<std::vector<GpgTOFUInfo>> GetTofuInfos() const;

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgKeySignature>>
   */
  [[nodiscard]] std::unique_ptr<std::vector<GpgKeySignature>> GetSignatures()
      const;

  /**
   * @brief Construct a new Gpg U I D object
   *
   */
  GpgUID();

  /**
   * @brief Construct a new Gpg U I D object
   *
   * @param uid
   */
  explicit GpgUID(gpgme_user_id_t uid);

  /**
   * @brief Construct a new Gpg U I D object
   *
   * @param o
   */
  GpgUID(GpgUID &&o) noexcept;

  /**
   * @brief Construct a new Gpg U I D object
   *
   */
  GpgUID(const GpgUID &) = delete;

  /**
   * @brief
   *
   * @param o
   * @return GpgUID&
   */
  GpgUID &operator=(GpgUID &&o) noexcept;

  /**
   * @brief
   *
   * @return GpgUID&
   */
  GpgUID &operator=(const GpgUID &) = delete;

 private:
  using UidRefHandler =
      std::unique_ptr<struct _gpgme_user_id,
                      std::function<void(gpgme_user_id_t)>>;  ///<

  UidRefHandler uid_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGUID_H