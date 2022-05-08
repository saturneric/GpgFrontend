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

#include "GpgTOFUInfo.h"

GpgFrontend::GpgTOFUInfo::GpgTOFUInfo() = default;

GpgFrontend::GpgTOFUInfo::GpgTOFUInfo(gpgme_tofu_info_t tofu_info)
    : _tofu_info_ref(tofu_info, [&](gpgme_tofu_info_t tofu_info) {}) {}

GpgFrontend::GpgTOFUInfo::GpgTOFUInfo(GpgTOFUInfo&& o) noexcept {
  swap(_tofu_info_ref, o._tofu_info_ref);
}

GpgFrontend::GpgTOFUInfo& GpgFrontend::GpgTOFUInfo::operator=(
    GpgTOFUInfo&& o) noexcept {
  swap(_tofu_info_ref, o._tofu_info_ref);
  return *this;
};

unsigned GpgFrontend::GpgTOFUInfo::GetValidity() const {
  return _tofu_info_ref->validity;
}

unsigned GpgFrontend::GpgTOFUInfo::GetPolicy() const {
  return _tofu_info_ref->policy;
}

unsigned long GpgFrontend::GpgTOFUInfo::GetSignCount() const {
  return _tofu_info_ref->signcount;
}

unsigned long GpgFrontend::GpgTOFUInfo::GetEncrCount() const {
  return _tofu_info_ref->encrcount;
}

unsigned long GpgFrontend::GpgTOFUInfo::GetSignFirst() const {
  return _tofu_info_ref->signfirst;
}

/**
 * @brief
 *
 * @return unsigned long
 */
unsigned long GpgFrontend::GpgTOFUInfo::GetSignLast() const {
  return _tofu_info_ref->signlast;
}

/**
 * @brief
 *
 * @return unsigned long
 */
unsigned long GpgFrontend::GpgTOFUInfo::GetEncrLast() const {
  return _tofu_info_ref->encrlast;
}

/**
 * @brief
 *
 * @return std::string
 */
std::string GpgFrontend::GpgTOFUInfo::GetDescription() const {
  return _tofu_info_ref->description;
}