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

#ifndef GPGFRONTEND_GPGUID_H
#define GPGFRONTEND_GPGUID_H

#include "GpgKeySignature.h"
#include "GpgTOFUInfo.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GpgUID {
 public:
  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetName() const { return uid_ref_->name; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetEmail() const { return uid_ref_->email; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetComment() const { return uid_ref_->comment; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetUID() const { return uid_ref_->uid; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool GetRevoked() const { return uid_ref_->revoked; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool GetInvalid() const { return uid_ref_->invalid; }

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgTOFUInfo>>
   */
  [[nodiscard]] std::unique_ptr<std::vector<GpgTOFUInfo>> GetTofuInfos() const {
    auto infos = std::make_unique<std::vector<GpgTOFUInfo>>();
    auto info_next = uid_ref_->tofu;
    while (info_next != nullptr) {
      infos->push_back(GpgTOFUInfo(info_next));
      info_next = info_next->next;
    }
    return infos;
  }

  /**
   * @brief
   *
   * @return std::unique_ptr<std::vector<GpgKeySignature>>
   */
  [[nodiscard]] std::unique_ptr<std::vector<GpgKeySignature>> GetSignatures()
      const {
    auto sigs = std::make_unique<std::vector<GpgKeySignature>>();
    auto sig_next = uid_ref_->signatures;
    while (sig_next != nullptr) {
      sigs->push_back(GpgKeySignature(sig_next));
      sig_next = sig_next->next;
    }
    return sigs;
  }

  /**
   * @brief Construct a new Gpg U I D object
   *
   */
  GpgUID() = default;

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
  GpgUID(GpgUID &&o) noexcept { swap(uid_ref_, o.uid_ref_); }

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
  GpgUID &operator=(GpgUID &&o) noexcept {
    swap(uid_ref_, o.uid_ref_);
    return *this;
  }

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