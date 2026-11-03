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

#include "GpgDecryptResult.h"

namespace GpgFrontend {

GpgDecryptResult::GpgDecryptResult(const GFDecryptResult& r)
    : gf_result_ref_(QSharedPointer<GFDecryptResult>::create(r)) {}

GpgDecryptResult::GpgDecryptResult() = default;

GpgDecryptResult::~GpgDecryptResult() = default;

auto GpgDecryptResult::IsGood() -> bool { return gf_result_ref_ != nullptr; }

auto GpgDecryptResult::Recipients() -> QList<GpgRecipient> {
  QList<GpgRecipient> result;
  if (gf_result_ref_ == nullptr) return result;
  for (const auto& rec : gf_result_ref_->recipients) {
    result.push_back(GpgRecipient{rec});
  }
  return result;
}

auto GpgDecryptResult::UnsupportedAlgorithm() -> QString {
  if (gf_result_ref_ == nullptr) return {};
  return gf_result_ref_->unsupported_algorithm;
}

auto GpgDecryptResult::Filename() -> QString {
  if (gf_result_ref_ == nullptr) return {};
  return gf_result_ref_->filename;
}

auto GpgDecryptResult::MIME() -> bool {
  return gf_result_ref_ != nullptr && gf_result_ref_->is_mime;
}

auto GpgDecryptResult::MessageIntegrityProtected() -> bool {
  return gf_result_ref_ != nullptr && gf_result_ref_->integrity_protected;
}

auto GpgDecryptResult::CipherWarning() -> QString {
  if (gf_result_ref_ != nullptr) return gf_result_ref_->cipher_warning;
  return {};
}

auto GpgDecryptResult::SymmetricEncryptionAlgorithm() -> QString {
  if (gf_result_ref_ == nullptr) return {};
  return gf_result_ref_->symkey_algo;
}

auto GpgDecryptResult::ErrorDetail() -> QString {
  if (gf_result_ref_ != nullptr) {
    return gf_result_ref_->error_detail;
  }
  return {};
}

}  // namespace GpgFrontend