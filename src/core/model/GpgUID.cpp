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

#include "core/model/GpgUID.h"

GpgFrontend::GpgUID::GpgUID() = default;

GpgFrontend::GpgUID::GpgUID(gpgme_user_id_t uid)
    : uid_ref_(uid, [&](gpgme_user_id_t uid) {}) {}

GpgFrontend::GpgUID::GpgUID(GpgUID &&o) noexcept { swap(uid_ref_, o.uid_ref_); }

std::string GpgFrontend::GpgUID::GetName() const { return uid_ref_->name; }

std::string GpgFrontend::GpgUID::GetEmail() const { return uid_ref_->email; }

std::string GpgFrontend::GpgUID::GetComment() const {
  return uid_ref_->comment;
}

std::string GpgFrontend::GpgUID::GetUID() const { return uid_ref_->uid; }

bool GpgFrontend::GpgUID::GetRevoked() const { return uid_ref_->revoked; }

bool GpgFrontend::GpgUID::GetInvalid() const { return uid_ref_->invalid; }

std::unique_ptr<std::vector<GpgFrontend::GpgTOFUInfo>>
GpgFrontend::GpgUID::GetTofuInfos() const {
  auto infos = std::make_unique<std::vector<GpgTOFUInfo>>();
  auto info_next = uid_ref_->tofu;
  while (info_next != nullptr) {
    infos->push_back(GpgTOFUInfo(info_next));
    info_next = info_next->next;
  }
  return infos;
}

std::unique_ptr<std::vector<GpgFrontend::GpgKeySignature>>
GpgFrontend::GpgUID::GetSignatures() const {
  auto sigs = std::make_unique<std::vector<GpgKeySignature>>();
  auto sig_next = uid_ref_->signatures;
  while (sig_next != nullptr) {
    sigs->push_back(GpgKeySignature(sig_next));
    sig_next = sig_next->next;
  }
  return sigs;
}
