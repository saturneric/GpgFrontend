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

GpgDecryptResult::GpgDecryptResult(gpgme_decrypt_result_t r)
    : result_ref_(QSharedPointer<struct _gpgme_op_decrypt_result>(
          (gpgme_result_ref(r), r), [](gpgme_decrypt_result_t p) {
            if (p != nullptr) {
              gpgme_result_unref(p);
            }
          })) {}

GpgDecryptResult::GpgDecryptResult(const GFDecryptResult& r)
    : gf_result_ref_(QSharedPointer<GFDecryptResult>::create(r)) {}

GpgDecryptResult::GpgDecryptResult() = default;

GpgDecryptResult::~GpgDecryptResult() = default;

auto GpgDecryptResult::IsGood() -> bool {
  return result_ref_ != nullptr || gf_result_ref_ != nullptr;
}

auto GpgDecryptResult::GetRaw() -> gpgme_decrypt_result_t {
  return result_ref_.get();
}

auto GpgDecryptResult::Recipients() -> QContainer<GpgRecipient> {
  QContainer<GpgRecipient> result;

  if (gf_result_ref_ != nullptr) {
    for (const auto& rec : gf_result_ref_->recipients) {
      result.push_back(GpgRecipient{rec});
    }
    return result;
  }

  if (result_ref_ == nullptr) {
    return result;
  }

  for (auto* reci = result_ref_->recipients; reci != nullptr;
       reci = reci->next) {
    try {
      result.push_back(GpgRecipient{reci});
    } catch (...) {
      FLOG_W(
          "caught exception when processing invalid_recipients, "
          "maybe nullptr of fpr");
    }
  }
  return result;
}

auto GpgDecryptResult::UnsupportedAlgorithm() -> QString {
  if (gf_result_ref_ != nullptr) {
    return {};
  }
  if (result_ref_ != nullptr && result_ref_->unsupported_algorithm != nullptr) {
    return QString{result_ref_->unsupported_algorithm};
  }
  return {};
}

auto GpgDecryptResult::Filename() -> QString {
  if (gf_result_ref_ != nullptr) {
    return gf_result_ref_->filename;
  }
  if (result_ref_ != nullptr && result_ref_->file_name != nullptr) {
    return result_ref_->file_name;
  }
  return {};
}

auto GpgDecryptResult::MIME() -> bool {
  if (gf_result_ref_ != nullptr) {
    return false;  // RPGP does not support MIME, treat as non-MIME
  }
  if (result_ref_ != nullptr) {
    return result_ref_->is_mime != 0;
  }
  return {};
}

auto GpgDecryptResult::MessageIntegrityProtected() -> bool {
  if (gf_result_ref_ != nullptr) {
    return true;  // RPGP always has MDC
  }
  if (result_ref_ != nullptr) {
    return result_ref_->legacy_cipher_nomdc == 0;
  }
  return {};
}

auto GpgDecryptResult::SymmetricEncryptionAlgorithm() -> QString {
  if (gf_result_ref_ != nullptr) {
    return {};
  }
  if (result_ref_ != nullptr && result_ref_->symkey_algo != nullptr) {
    return QString{result_ref_->symkey_algo};
  }
  return {};
}

auto GpgDecryptResult::ErrorDetail() -> QString {
  if (gf_result_ref_ != nullptr) {
    return gf_result_ref_->error_detail;
  }
  return {};
}

}  // namespace GpgFrontend