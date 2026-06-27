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

#include "GpgEncryptResult.h"

namespace GpgFrontend {
GpgEncryptResult::GpgEncryptResult(gpgme_encrypt_result_t r)
    : result_ref_(QSharedPointer<struct _gpgme_op_encrypt_result>(
          (gpgme_result_ref(r), r), [](gpgme_encrypt_result_t p) {
            if (p != nullptr) {
              gpgme_result_unref(p);
            }
          })) {}

GpgEncryptResult::GpgEncryptResult(const GFEncryptResult& r)
    : gf_result_ref_(QSharedPointer<GFEncryptResult>::create(r)) {}

GpgEncryptResult::GpgEncryptResult() = default;

GpgEncryptResult::~GpgEncryptResult() = default;

auto GpgEncryptResult::IsGood() -> bool { return result_ref_ != nullptr; }

auto GpgEncryptResult::GetRaw() -> gpgme_encrypt_result_t {
  return result_ref_.get();
}

auto GpgEncryptResult::InvalidRecipients()
    -> QContainer<std::tuple<QString, GpgError>> {
  QContainer<std::tuple<QString, GpgError>> result;

  if (gf_result_ref_ != nullptr) {
    for (const auto& r : gf_result_ref_->invalid_recipients) {
      result.push_back({r.fpr, r.reason});
    }
    return result;
  }

  if (result_ref_ == nullptr) {
    return result;
  }

  for (auto* invalid_key = result_ref_->invalid_recipients;
       invalid_key != nullptr; invalid_key = invalid_key->next) {
    try {
      result.push_back({
          QString{invalid_key->fpr},
          invalid_key->reason,
      });
    } catch (...) {
      FLOG_W(
          "caught exception when processing invalid_recipients, "
          "maybe nullptr of fpr");
    }
  }
  return result;
}

auto GpgEncryptResult::Recipients() -> QContainer<GpgRecipient> {
  QContainer<GpgRecipient> result;

  if (gf_result_ref_ != nullptr) {
    for (const auto& r : gf_result_ref_->recipients) {
      result.push_back(GpgRecipient(r));
    }
  }

  return result;
}

auto GpgEncryptResult::ErrorDetail() -> QString {
  if (gf_result_ref_ != nullptr) {
    return gf_result_ref_->error_detail;
  }
  return {};
}

}  // namespace GpgFrontend