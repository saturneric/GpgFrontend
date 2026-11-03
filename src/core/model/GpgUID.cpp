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

#include "core/model/GpgUID.h"

namespace GpgFrontend {

GpgUID::GpgUID() = default;

GpgUID::GpgUID(QSharedPointer<GFKeyMetadata> km_ref, const GFUserId &um_ref)
    : km_ref_(std::move(km_ref)),
      um_ref_(QSharedPointer<GFUserId>::create(um_ref)) {}

GpgUID::GpgUID(const GpgUID &) = default;

auto GpgUID::operator=(const GpgUID &) -> GpgUID & = default;

auto GpgUID::GetName() const -> QString {
  return um_ref_ != nullptr ? um_ref_->name : QString{};
}

auto GpgUID::GetEmail() const -> QString {
  return um_ref_ != nullptr ? um_ref_->email : QString{};
}

auto GpgUID::GetComment() const -> QString {
  return um_ref_ != nullptr ? um_ref_->comment : QString{};
}

auto GpgUID::GetUID() const -> QString {
  if (um_ref_ == nullptr) return {};
  return um_ref_->raw.isEmpty() ? um_ref_->ToString() : um_ref_->raw;
}

auto GpgUID::GetRevoked() const -> bool {
  return um_ref_ != nullptr && um_ref_->is_revoked;
}

auto GpgUID::GetInvalid() const -> bool {
  return um_ref_ != nullptr && um_ref_->is_invalid;
}

auto GpgUID::GetTofuInfos() const -> std::unique_ptr<QList<GpgTOFUInfo>> {
  auto infos = std::make_unique<QList<GpgTOFUInfo>>();
  if (um_ref_ == nullptr) return infos;
  for (const auto &info : um_ref_->tofu) infos->push_back(GpgTOFUInfo(info));
  return infos;
}

auto GpgUID::GetSignatures() const -> std::unique_ptr<QList<GpgKeySignature>> {
  auto sigs = std::make_unique<QList<GpgKeySignature>>();
  if (um_ref_ == nullptr) return sigs;
  for (const auto &sig : um_ref_->signatures) {
    sigs->push_back(GpgKeySignature(sig));
  }
  return sigs;
}

}  // namespace GpgFrontend
