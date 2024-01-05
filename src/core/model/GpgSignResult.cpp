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

#include "GpgSignResult.h"

namespace GpgFrontend {
GpgSignResult::GpgSignResult(gpgme_sign_result_t r)
    : result_ref_(std::shared_ptr<struct _gpgme_op_sign_result>(
          (gpgme_result_ref(r), r), [](gpgme_sign_result_t p) {
            if (p != nullptr) {
              gpgme_result_unref(p);
            }
          })) {}

GpgSignResult::GpgSignResult() = default;

GpgSignResult::~GpgSignResult() = default;

auto GpgSignResult::IsGood() -> bool { return result_ref_ != nullptr; }

auto GpgSignResult::GetRaw() -> gpgme_sign_result_t {
  return result_ref_.get();
}

auto GpgSignResult::InvalidSigners()
    -> std::vector<std::tuple<std::string, GpgError>> {
  std::vector<std::tuple<std::string, GpgError>> result;
  for (auto* invalid_key = result_ref_->invalid_signers; invalid_key != nullptr;
       invalid_key = invalid_key->next) {
    try {
      result.emplace_back(std::string{invalid_key->fpr}, invalid_key->reason);
    } catch (...) {
      GF_CORE_LOG_ERROR(
          "caught exception when processing invalid_signers, "
          "maybe nullptr of fpr");
    }
  }
  return result;
}
}  // namespace GpgFrontend