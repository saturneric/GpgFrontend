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

#include "GpgTOFUInfo.h"

namespace GpgFrontend {

GpgTOFUInfo::GpgTOFUInfo() = default;

GpgTOFUInfo::GpgTOFUInfo(gpgme_tofu_info_t tofu_info)
    : tofu_info_ref_(tofu_info, [&](gpgme_tofu_info_t tofu_info) {}) {}

GpgTOFUInfo::GpgTOFUInfo(GpgTOFUInfo&& o) noexcept {
  swap(tofu_info_ref_, o.tofu_info_ref_);
}

auto GpgTOFUInfo::operator=(GpgTOFUInfo&& o) noexcept -> GpgTOFUInfo& {
  swap(tofu_info_ref_, o.tofu_info_ref_);
  return *this;
};

auto GpgTOFUInfo::GetValidity() const -> unsigned {
  return tofu_info_ref_->validity;
}

auto GpgTOFUInfo::GetPolicy() const -> unsigned {
  return tofu_info_ref_->policy;
}

auto GpgTOFUInfo::GetSignCount() const -> unsigned long {
  return tofu_info_ref_->signcount;
}

auto GpgTOFUInfo::GetEncrCount() const -> unsigned long {
  return tofu_info_ref_->encrcount;
}

auto GpgTOFUInfo::GetSignFirst() const -> unsigned long {
  return tofu_info_ref_->signfirst;
}

/**
 * @brief
 *
 * @return unsigned long
 */
auto GpgTOFUInfo::GetSignLast() const -> unsigned long {
  return tofu_info_ref_->signlast;
}

/**
 * @brief
 *
 * @return unsigned long
 */
auto GpgTOFUInfo::GetEncrLast() const -> unsigned long {
  return tofu_info_ref_->encrlast;
}

/**
 * @brief
 *
 * @return std::string
 */
auto GpgTOFUInfo::GetDescription() const -> std::string {
  return tofu_info_ref_->description;
}

}  // namespace GpgFrontend