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

#include "GpgSignResult.h"
namespace GpgFrontend {
GpgSignResult::GpgSignResult(gpgme_sign_result_t r)
    : result_ref_(QSharedPointer<struct _gpgme_op_sign_result>(
          (gpgme_result_ref(r), r), [](gpgme_sign_result_t p) {
            if (p != nullptr) {
              gpgme_result_unref(p);
            }
          })) {}

GpgSignResult::GpgSignResult(const GFSignResult& r)
    : gf_result_ref_(QSharedPointer<GFSignResult>::create(r)) {}

GpgSignResult::GpgSignResult() = default;

GpgSignResult::~GpgSignResult() = default;

auto GpgSignResult::IsGood() -> bool {
  return result_ref_ != nullptr || gf_result_ref_ != nullptr;
}

auto GpgSignResult::GetRaw() -> gpgme_sign_result_t {
  // Only the native (GnuPG) path owns a gpgme result; rPGP-backed results keep
  // their data in gf_result_ref_ and have no raw handle, so this returns
  // nullptr for them. Callers must tolerate a null return.
  return result_ref_.get();
}

auto GpgSignResult::InvalidSigners()
    -> QContainer<std::pair<QString, GpgError>> {
  QContainer<std::pair<QString, GpgError>> result;

  if (gf_result_ref_ != nullptr) {
    return result;
  }

  for (auto* invalid_key = result_ref_->invalid_signers; invalid_key != nullptr;
       invalid_key = invalid_key->next) {
    try {
      result.push_back({
          QString{invalid_key->fpr},
          invalid_key->reason,
      });
    } catch (...) {
      FLOG_W(
          "caught exception when processing invalid_signers, "
          "maybe nullptr of fpr");
    }
  }
  return result;
}

auto GpgSignResult::HashAlgo() -> QString {
  if (gf_result_ref_ != nullptr) {
    return gf_result_ref_->signatures.empty()
               ? ""
               : gf_result_ref_->signatures.front().hash_algo;
  }
  if (result_ref_->signatures == nullptr) return {};
  return gpgme_hash_algo_name(result_ref_->signatures->hash_algo);
}

auto GpgSignResult::ErrorDetail() -> QString {
  if (gf_result_ref_ != nullptr) {
    return gf_result_ref_->error_detail;
  }
  return {};
}

auto GpgSignResult::Signatures() -> QContainer<GpgSignature> {
  QContainer<GpgSignature> signatures;

  if (gf_result_ref_ != nullptr) {
    for (const auto& sig : gf_result_ref_->signatures) {
      signatures.push_back(GpgSignature{sig});
    }
    return signatures;
  }

  auto* signature = result_ref_->signatures;
  while (signature != nullptr) {
    GFSignature sig;
    sig.created_at = signature->timestamp;
    sig.hash_algo = gpgme_hash_algo_name(signature->hash_algo);
    sig.pub_algo = gpgme_pubkey_algo_name(signature->pubkey_algo);
    sig.issuer_fpr = signature->fpr != nullptr ? signature->fpr : "";
    sig.status = GFSignatureStatus::kVALID;

    if ((signature->type & GPGME_SIG_MODE_NORMAL) != 0) {
      sig.sig_type = "Normal";
    } else if ((signature->type & GPGME_SIG_MODE_CLEAR) != 0) {
      sig.sig_type = "Clear";
    } else if ((signature->type & GPGME_SIG_MODE_DETACH) != 0) {
      sig.sig_type = "Detach";
    } else {
      sig.sig_type = "Unknown";
    }

    signatures.push_back(GpgSignature{sig});
    signature = signature->next;
  }
  return signatures;
}
}  // namespace GpgFrontend