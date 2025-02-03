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

#include "GpgVerifyResult.h"

#include "core/model/GpgSignature.h"

namespace GpgFrontend {
GpgVerifyResult::GpgVerifyResult(gpgme_verify_result_t r)
    : result_ref_(std::shared_ptr<struct _gpgme_op_verify_result>(
          (gpgme_result_ref(r), r), [](gpgme_verify_result_t p) {
            if (p != nullptr) {
              gpgme_result_unref(p);
            }
          })) {}

GpgVerifyResult::GpgVerifyResult() = default;

GpgVerifyResult::~GpgVerifyResult() = default;

auto GpgVerifyResult::IsGood() const -> bool { return result_ref_ != nullptr; }

auto GpgVerifyResult::GetRaw() const -> gpgme_verify_result_t {
  return result_ref_.get();
}

auto GpgVerifyResult::GetSignature() const -> QContainer<GpgSignature> {
  QContainer<GpgSignature> signatures;

  auto* signature = result_ref_->signatures;
  while (signature != nullptr) {
    signatures.push_back(GpgSignature{signature});
    signature = signature->next;
  }
  return signatures;
}
}  // namespace GpgFrontend