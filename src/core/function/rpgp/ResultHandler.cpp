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

#include "ResultHandler.h"

#include "core/utils/RustUtils.h"

namespace GpgFrontend {

namespace {

/// Retrieve and consume the detailed, human-readable error message recorded by
/// the rPGP engine for the most recent failure on this thread. Mirrors how the
/// GnuPG engine exposes `gpg_strerror`, giving the user the specific cause
/// (e.g. "checksum mismatch", "unsupported algorithm") instead of a generic
/// category. Returns an empty string when the engine recorded no detail.
///
/// Must be called on the same thread that ran the operation, immediately after
/// it returns, because the underlying slot is thread-local.
auto FetchRustErrorDetail() -> QString {
  char* msg = Rust::gfr_get_last_error_msg();
  if (msg == nullptr) return {};
  auto detail = QString::fromUtf8(msg);
  Rust::gfr_crypto_free_string(msg);
  return detail;
}
auto ParseEncryptResultMeta(const Rust::GfrEncryptMetadataC& m)
    -> GFEncryptResult {
  GFEncryptResult result;

  for (size_t i = 0; i < m.invalid_recipient_count; ++i) {
    const auto& inv_rec = m.invalid_recipients[i];

    GpgError reason;
    if (inv_rec.reason == Rust::GfrStatus::ErrorNoKey) {
      reason = GPG_ERR_NO_KEY;
    } else if (inv_rec.reason == Rust::GfrStatus::ErrorInvalidData) {
      reason = GPG_ERR_INV_DATA;
    } else {
      reason = GPG_ERR_GENERAL;
    }

    result.invalid_recipients.push_back({
        QString::fromUtf8(inv_rec.fpr),
        reason,
    });
  }

  return result;
}

auto ParseSignResultMeta(const Rust::GfrSignMetadataC& m) -> GFSignResult {
  GFSignResult result;

  for (size_t i = 0; i < m.signature_count; ++i) {
    const auto& sig = m.signatures[i];

    GFSignatureStatus sig_status;
    switch (sig.status) {
      case Rust::GfrSignatureStatus::Valid:
        sig_status = GFSignatureStatus::kVALID;
        break;
      case Rust::GfrSignatureStatus::BadSignature:
        sig_status = GFSignatureStatus::kBAD_SIGNATURE;
        break;
      case Rust::GfrSignatureStatus::NoKey:
        sig_status = GFSignatureStatus::kNO_KEY;
        break;
      default:
        sig_status = GFSignatureStatus::kUNKNOWN_ERROR;
        break;
    }

    result.signatures.push_back({
        QString::fromUtf8(sig.issuer_fpr).toUpper(),
        sig_status,
        sig.created_at,
        sig.pub_algo,
        sig.hash_algo,
    });
  }

  return result;
}

auto ParseDecryptResultMeta(const Rust::GfrDecryptMetadataC& m)
    -> GFDecryptResult {
  GFDecryptResult result;

  result.filename = QString::fromUtf8(m.filename);
  for (size_t i = 0; i < m.recipient_count; ++i) {
    const auto& rec = m.recipients[i];
    GpgError status;
    if (rec.status == Rust::GfrRecipientStatus::Success) {
      status = GPG_ERR_NO_ERROR;
    } else if (rec.status == Rust::GfrRecipientStatus::NoKey) {
      status = GPG_ERR_NO_KEY;
    } else {
      status = GPG_ERR_GENERAL;
    }
    result.recipients.push_back({
        QString::fromUtf8(rec.key_id).toUpper(),
        QString::fromUtf8(rec.pub_algo),
        status,
    });
  }

  return result;
}

auto ParseVerifyResultMeta(const Rust::GfrVerifyMetadataC& m)
    -> GFVerifyResult {
  GFVerifyResult result;

  result.is_verified = m.is_verified;
  for (size_t i = 0; i < m.signature_count; ++i) {
    const auto& sig = m.signatures[i];

    auto sig_status = GFSignatureStatus::kUNKNOWN_ERROR;
    switch (sig.status) {
      case Rust::GfrSignatureStatus::Valid:
        sig_status = GFSignatureStatus::kVALID;
        break;
      case Rust::GfrSignatureStatus::BadSignature:
        sig_status = GFSignatureStatus::kBAD_SIGNATURE;
        break;
      case Rust::GfrSignatureStatus::NoKey:
        sig_status = GFSignatureStatus::kNO_KEY;
        break;
      case Rust::GfrSignatureStatus::UnknownError:
      default:
        sig_status = GFSignatureStatus::kUNKNOWN_ERROR;
        break;
    }

    result.signatures.push_back({
        QString::fromUtf8(sig.issuer_fpr).toUpper(),
        sig_status,
        sig.created_at,
        sig.pub_algo,
        sig.hash_algo,
    });
  }

  return result;
}

}  // namespace

auto GfrEncryptResultC2GFEncryptResult(const Rust::GfrEncryptResultC& r)
    -> GFEncryptResult {
  GFEncryptResult result = ParseEncryptResultMeta(r.meta);
  result.data = GFBuffer(reinterpret_cast<const char*>(r.data), r.data_len);
  return result;
}

auto GfrDecryptResultC2GFDecryptResult(const Rust::GfrDecryptResultC& r)
    -> GFDecryptResult {
  GFDecryptResult result = ParseDecryptResultMeta(r.meta);

  result.data = GFBuffer(reinterpret_cast<const char*>(r.data), r.data_len);
  return result;
}

auto GfrSignResultC2GFSignResult(const Rust::GfrSignResultC& r)
    -> GFSignResult {
  GFSignResult result = ParseSignResultMeta(r.meta);
  result.data = GFBuffer(reinterpret_cast<const char*>(r.data), r.data_len);
  return result;
}

auto GfrVerifyResultC2GFVerifyResult(const Rust::GfrVerifyResultC& r)
    -> GFVerifyResult {
  GFVerifyResult result = ParseVerifyResultMeta(r.meta);
  return result;
}

auto GfrEncryptAndSignResultC2GFEncryptAndSignResult(
    const Rust::GfrEncryptAndSignResultC& r) -> GFEncryptAndSignResult {
  GFEncryptAndSignResult result;
  result.data = GFBuffer(reinterpret_cast<const char*>(r.data), r.data_len);
  result.sign_result = ParseSignResultMeta(r.sign_meta);
  result.encrypt_result = ParseEncryptResultMeta(r.encrypt_meta);
  return result;
}

auto GfrDecryptAndVerifyResultC2GFDecryptAndVerifyResult(
    const Rust::GfrDecryptAndVerifyResultC& r) -> GFDecryptAndVerifyResult {
  GFDecryptAndVerifyResult result;
  result.data = GFBuffer(reinterpret_cast<const char*>(r.data), r.data_len);
  result.decrypt_result = ParseDecryptResultMeta(r.decrypt_meta);
  result.verify_result = ParseVerifyResultMeta(r.verify_meta);
  return result;
}

auto HandleEncryptResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                         Rust::GfrEncryptResultC gfr_result)
    -> std::tuple<GpgError, GFEncryptResult> {
  LOG_D() << "Handling encrypt result. Rust status: " << static_cast<int>(err);

  GpgError gf_err = GPG_ERR_NO_ERROR;
  GFEncryptResult result;
  if (err != Rust::GfrStatus::Success) {
    result.error_detail = FetchRustErrorDetail();
    if (!result.error_detail.isEmpty()) {
      LOG_E() << "Encryption failed, engine detail:" << result.error_detail;
    }

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Encryption cancelled by user.";
      gf_err = GPG_ERR_CANCELED;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Encryption failed: No data to encrypt.";
      gf_err = GPG_ERR_INV_DATA;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorNoKey) {
      LOG_E() << "Encryption failed: No valid recipient keys provided.";
      gf_err = GPG_ERR_NO_KEY;
      goto end;
    }

    LOG_E() << "Encryption failed: An unknown error occurred.";
    gf_err = GPG_ERR_GENERAL;
    goto end;
  }

  // success case, convert the result
  result = GfrEncryptResultC2GFEncryptResult(gfr_result);

end:
  return {gf_err, result};
}

auto HandleDecryptResult(GFKeyDatabase& key_db, const GFBuffer& in_buffer,
                         Rust::GfrStatus err,
                         Rust::GfrDecryptResultC gfr_result)
    -> std::tuple<GpgError, GFDecryptResult> {
  LOG_D() << "Handling decrypt result. Rust status: " << static_cast<int>(err);

  GpgError gf_err = GPG_ERR_NO_ERROR;
  GFDecryptResult result;
  if (err != Rust::GfrStatus::Success) {
    result.error_detail = FetchRustErrorDetail();
    if (!result.error_detail.isEmpty()) {
      LOG_E() << "Decryption failed, engine detail:" << result.error_detail;
    }

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Decryption cancelled by user.";
      gf_err = GPG_ERR_CANCELED;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Decryption failed: No encrypted data found.";
      gf_err = GPG_ERR_INV_DATA;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorBadPassphrase) {
      LOG_E() << "Decryption failed: Incorrect passphrase.";
      gf_err = GPG_ERR_BAD_PASSPHRASE;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorDecryptionFailed) {
      LOG_E() << "Decryption failed: Decryption process failed. This could be "
                 "due to incorrect passphrase, bad key, or corrupted data.";
      gf_err = GPG_ERR_DECRYPT_FAILED;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorNoKey) {
      LOG_E() << "Decryption failed: Incorrect passphrase or bad key.";

      bool has_valid_recipient = false;
      result.recipients = SniffRecipients(key_db, in_buffer);
      for (auto& rec : result.recipients) {
        if (rec.status != GPG_ERR_NO_KEY) {
          rec.status = GPG_ERR_BAD_PASSPHRASE;
          has_valid_recipient = true;
        }
      }
      gf_err = has_valid_recipient ? GPG_ERR_BAD_PASSPHRASE : GPG_ERR_NO_KEY;
      goto end;
    }

    gf_err = GPG_ERR_GENERAL;
    goto end;
  }

  // success case, convert the result
  result = GfrDecryptResultC2GFDecryptResult(gfr_result);

end:
  return {gf_err, result};
}

auto HandleSignResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                      Rust::GfrSignResultC gfr_result)
    -> std::tuple<GpgError, GFSignResult> {
  LOG_D() << "Handling sign result. Rust status: " << static_cast<int>(err);

  GpgError gf_err = GPG_ERR_NO_ERROR;
  GFSignResult result;
  if (err != Rust::GfrStatus::Success) {
    result.error_detail = FetchRustErrorDetail();
    if (!result.error_detail.isEmpty()) {
      LOG_E() << "Signing failed, engine detail:" << result.error_detail;
    }

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Signing cancelled by user.";
      gf_err = GPG_ERR_CANCELED;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Signing failed: No data to sign.";
      gf_err = GPG_ERR_INV_DATA;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorBadPassphrase) {
      LOG_E() << "Signing failed: Incorrect passphrase.";
      gf_err = GPG_ERR_BAD_PASSPHRASE;
      goto end;
    }

    gf_err = GPG_ERR_GENERAL;
    goto end;
  }

  // success case, convert the result
  result = GfrSignResultC2GFSignResult(gfr_result);

end:
  return {gf_err, result};
}

auto HandleVerifyResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                        Rust::GfrVerifyResultC gfr_result)
    -> std::tuple<GpgError, GFVerifyResult> {
  LOG_D() << "Handling verify result. Rust status: " << static_cast<int>(err);

  GpgError gf_err = GPG_ERR_NO_ERROR;
  GFVerifyResult result;
  if (err != Rust::GfrStatus::Success) {
    result.error_detail = FetchRustErrorDetail();
    if (!result.error_detail.isEmpty()) {
      LOG_E() << "Verification failed, engine detail:" << result.error_detail;
    }

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Verification cancelled by user.";
      gf_err = GPG_ERR_CANCELED;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Verification failed: No signature data found.";
      gf_err = GPG_ERR_INV_DATA;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorIo) {
      LOG_E() << "Verification failed: I/O error occurred while reading data.";
      gf_err = GPG_ERR_EIO;
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInternal) {
      LOG_E() << "Verification failed: An internal error occurred in the "
                 "verification process.";
      gf_err = GPG_ERR_NO_DATA;
      goto end;
    }

    gf_err = GPG_ERR_GENERAL;
  }

  LOG_D() << "Verification success. Processing results...";

  // success case, convert the result
  result = GfrVerifyResultC2GFVerifyResult(gfr_result);

end:
  return {gf_err, result};
}

}  // namespace GpgFrontend
