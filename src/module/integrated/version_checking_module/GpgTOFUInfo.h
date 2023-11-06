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

#pragma once

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgTOFUInfo {
 public:
  /**
   * @brief
   *
   * @return unsigned
   */
  [[nodiscard]] unsigned GetValidity() const;
  /**
   * @brief
   *
   * @return unsigned
   */
  [[nodiscard]] unsigned GetPolicy() const;

  /**
   * @brief
   *
   * @return unsigned long
   */
  [[nodiscard]] unsigned long GetSignCount() const;

  /**
   * @brief
   *
   * @return unsigned long
   */
  [[nodiscard]] unsigned long GetEncrCount() const;

  /**
   * @brief
   *
   * @return unsigned long
   */
  [[nodiscard]] unsigned long GetSignFirst() const;

  /**
   * @brief
   *
   * @return unsigned long
   */
  [[nodiscard]] unsigned long GetSignLast() const;

  /**
   * @brief
   *
   * @return unsigned long
   */
  [[nodiscard]] unsigned long GetEncrLast() const;

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetDescription() const;

  /**
   * @brief Construct a new Gpg T O F U Info object
   *
   */
  GpgTOFUInfo();

  /**
   * @brief Construct a new Gpg T O F U Info object
   *
   * @param tofu_info
   */
  explicit GpgTOFUInfo(gpgme_tofu_info_t tofu_info);

  /**
   * @brief Construct a new Gpg T O F U Info object
   *
   * @param o
   */
  GpgTOFUInfo(GpgTOFUInfo&& o) noexcept;

  /**
   * @brief Construct a new Gpg T O F U Info object
   *
   */
  GpgTOFUInfo(const GpgTOFUInfo&) = delete;

  /**
   * @brief
   *
   * @param o
   * @return GpgTOFUInfo&
   */
  GpgTOFUInfo& operator=(GpgTOFUInfo&& o) noexcept;

  /**
   * @brief
   *
   * @return GpgTOFUInfo&
   */
  GpgTOFUInfo& operator=(const GpgTOFUInfo&) = delete;

 private:
  using SubkeyRefHandler =
      std::unique_ptr<struct _gpgme_tofu_info,
                      std::function<void(gpgme_tofu_info_t)>>;  ///<

  SubkeyRefHandler _tofu_info_ref = nullptr;  ///<
};

}  // namespace GpgFrontend
