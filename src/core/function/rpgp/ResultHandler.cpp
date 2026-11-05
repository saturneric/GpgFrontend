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

#include "core/function/rpgp/KeyStorage.h"
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

/// Drain the engine's pending error detail and, unless the failure was a user
/// cancellation, store it on the result and log it. The slot is always drained
/// (even on cancel) so a stale message cannot leak into a later operation; a
/// cancellation is a deliberate user action, not a failure that needs a reason.
void RecordEngineDetail(Rust::GfrStatus err, QString& detail_out,
                        const char* op) {
  auto detail = FetchRustErrorDetail();
  if (err == Rust::GfrStatus::ErrorCanceled) return;
  detail_out = detail;
  if (!detail.isEmpty()) {
    LOG_E() << op << "failed, engine detail:" << detail;
  }
}
}  // namespace

auto FromRpgpStatus(Rust::GfrStatus err) -> GFError {
  const auto failed = [err](GFErrorCode code) -> GFError {
    return {code, GFErrorOrigin::kRPGP, static_cast<std::int32_t>(err)};
  };

  switch (err) {
    case Rust::GfrStatus::Success:
      return {};
    case Rust::GfrStatus::ErrorCanceled:
      return failed(GFErrorCode::kCANCELED);
    case Rust::GfrStatus::ErrorFetchPasswordFailed:
      return failed(GFErrorCode::kNO_PASSPHRASE);
    case Rust::GfrStatus::ErrorBadPassphrase:
    case Rust::GfrStatus::ErrorWrongPassword:
      return failed(GFErrorCode::kBAD_PASSPHRASE);
    case Rust::GfrStatus::ErrorDataCorrupted:
      return failed(GFErrorCode::kBAD_DATA);
    case Rust::GfrStatus::ErrorIo:
      return failed(GFErrorCode::kIO);
    case Rust::GfrStatus::ErrorInvalidInput:
    case Rust::GfrStatus::ErrorInvalidData:
      return failed(GFErrorCode::kINVALID_DATA);
    case Rust::GfrStatus::ErrorArmorFailed:
    case Rust::GfrStatus::ErrorBadArmor:
      return failed(GFErrorCode::kBAD_ARMOR);
    case Rust::GfrStatus::ErrorUnsupportedAlgorithm:
      return failed(GFErrorCode::kUNSUPPORTED_ALGORITHM);
    case Rust::GfrStatus::ErrorUnsupportedFeature:
      return failed(GFErrorCode::kNOT_SUPPORTED);
    case Rust::GfrStatus::ErrorBadSignature:
      return failed(GFErrorCode::kBAD_SIGNATURE);
    case Rust::GfrStatus::ErrorNoPublicKey:
      return failed(GFErrorCode::kNO_PUBLIC_KEY);
    case Rust::GfrStatus::ErrorNoSecretKey:
      return failed(GFErrorCode::kNO_SECRET_KEY);
    case Rust::GfrStatus::ErrorExpiredKey:
      return failed(GFErrorCode::kKEY_EXPIRED);
    case Rust::GfrStatus::ErrorRevokedKey:
      return failed(GFErrorCode::kKEY_REVOKED);
    case Rust::GfrStatus::ErrorDecryptionFailed:
      return failed(GFErrorCode::kDECRYPT_FAILED);
    case Rust::GfrStatus::ErrorNoKey:
      return failed(GFErrorCode::kNO_KEY);
    case Rust::GfrStatus::ErrorUnusableKey:
      return failed(GFErrorCode::kUNUSABLE_PUBLIC_KEY);
    case Rust::GfrStatus::ErrorNoData:
      return failed(GFErrorCode::kNO_DATA);
    case Rust::GfrStatus::ErrorPasswordFailed:
      return failed(GFErrorCode::kBAD_PASSPHRASE);
    case Rust::GfrStatus::ErrorPasswordMismatch:
      return failed(GFErrorCode::kINVALID_PASSPHRASE);
    case Rust::GfrStatus::ErrorInternal:
      return failed(GFErrorCode::kINTERNAL);
    case Rust::GfrStatus::ErrorPanic:
      return failed(GFErrorCode::kBUG);
    default:
      return failed(GFErrorCode::kGENERAL);
  }
}

namespace {

auto ParseEncryptResultMeta(const Rust::GfrEncryptMetadataC& m)
    -> GFEncryptResult {
  GFEncryptResult result;

  for (size_t i = 0; i < m.invalid_recipient_count; ++i) {
    const auto& inv_rec = m.invalid_recipients[i];

    // The engine's own reason, so the analyser can tell the user why the key
    // was skipped.
    const auto reason = FromRpgpStatus(inv_rec.reason);

    result.invalid_recipients.push_back({
        QString::fromUtf8(inv_rec.fpr),
        reason,
    });
  }

  for (size_t i = 0; i < m.recipient_count; ++i) {
    const auto& rec = m.recipients[i];
    GFError status;
    if (rec.status == Rust::GfrRecipientStatus::Success) {
      status = GFError{};
    } else if (rec.status == Rust::GfrRecipientStatus::NoKey) {
      status = RpgpError(GFErrorCode::kNO_KEY);
    } else {
      status = RpgpError(GFErrorCode::kGENERAL);
    }
    result.recipients.push_back({
        QString::fromUtf8(rec.key_id).toUpper(),
        QString::fromUtf8(rec.pub_algo),
        status,
    });
  }

  return result;
}

/// The signature mode as GpgSignResult names it for the GnuPG engine, so the
/// "Mode" shown for a signature reads the same on either engine.
auto SignMode2SigType(Rust::GfrSignMode mode) -> QString {
  switch (mode) {
    case Rust::GfrSignMode::Inline:
      return u"Normal"_s;
    case Rust::GfrSignMode::ClearText:
      return u"Clear"_s;
    case Rust::GfrSignMode::Detached:
      return u"Detach"_s;
    default:
      return u"Unknown"_s;
  }
}

auto ParseSignatures(const Rust::GfrSignatureResultC* sigs, size_t count)
    -> QList<GFSignature> {
  QList<GFSignature> out;
  for (size_t i = 0; i < count; ++i) {
    const auto& sig = sigs[i];

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
      case Rust::GfrSignatureStatus::KeyRevoked:
        sig_status = GFSignatureStatus::kKEY_REVOKED;
        break;
      case Rust::GfrSignatureStatus::KeyExpired:
        sig_status = GFSignatureStatus::kKEY_EXPIRED;
        break;
      case Rust::GfrSignatureStatus::KeyNotAuthorized:
        sig_status = GFSignatureStatus::kKEY_NOT_AUTHORIZED;
        break;
      case Rust::GfrSignatureStatus::SignatureExpired:
        sig_status = GFSignatureStatus::kSIGNATURE_EXPIRED;
        break;
      case Rust::GfrSignatureStatus::WeakHash:
        sig_status = GFSignatureStatus::kWEAK_HASH;
        break;
      case Rust::GfrSignatureStatus::KeyExpiredSince:
        sig_status = GFSignatureStatus::kKEY_EXPIRED_SINCE;
        break;
      case Rust::GfrSignatureStatus::KeyRevokedSince:
        sig_status = GFSignatureStatus::kKEY_REVOKED_SINCE;
        break;
      case Rust::GfrSignatureStatus::UnknownError:
      default:
        break;
    }

    out.push_back({
        QString::fromUtf8(sig.issuer_fpr).toUpper(),
        sig_status,
        sig.created_at,
        sig.pub_algo,
        sig.hash_algo,
        SignMode2SigType(sig.sig_type),
    });
  }
  return out;
}

auto ParseSignResultMeta(const Rust::GfrSignMetadataC& m) -> GFSignResult {
  GFSignResult result;
  result.signatures = ParseSignatures(m.signatures, m.signature_count);
  return result;
}

auto ParseDecryptResultMeta(const Rust::GfrDecryptMetadataC& m)
    -> GFDecryptResult {
  GFDecryptResult result;

  result.filename = QString::fromUtf8(m.filename);
  result.symkey_algo = QString::fromUtf8(m.sym_algo);
  result.cipher_warning = QString::fromUtf8(m.cipher_warning);
  for (size_t i = 0; i < m.recipient_count; ++i) {
    const auto& rec = m.recipients[i];
    GFError status;
    if (rec.status == Rust::GfrRecipientStatus::Success) {
      status = GFError{};
    } else if (rec.status == Rust::GfrRecipientStatus::NoKey) {
      // A recipient this keyring holds no secret key for.
      status = RpgpError(GFErrorCode::kNO_SECRET_KEY);
    } else {
      status = RpgpError(GFErrorCode::kGENERAL);
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
  result.signatures = ParseSignatures(m.signatures, m.signature_count);
  return result;
}

}  // namespace

auto TakeEngineErrorDetail(Rust::GfrStatus err, const char* op) -> QString {
  QString detail;
  RecordEngineDetail(err, detail, op);
  return detail;
}

auto EditSecretKeyRpgp(OpenPGPContext& ctx, const QString& fpr,
                       const char* what, const SecretKeyEdit& edit) -> GFError {
  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return RpgpError(GFErrorCode::kGENERAL);
  }

  const auto key = key_db->GetKeyByIdentifier(fpr);
  if (!key || key->blocks.secret_key.Empty()) {
    LOG_E() << what << "needs a secret key the database does not hold:" << fpr;
    return RpgpError(GFErrorCode::kGENERAL);
  }

  const auto& secret = key->blocks.secret_key;
  const Rust::GfrBuffer block{reinterpret_cast<const uint8_t*>(secret.Data()),
                              secret.Size()};
  char* out = nullptr;
  const auto status = edit(key->metadata, block, &out);
  if (status != Rust::GfrStatus::Success) {
    LOG_E() << what << "failed, engine status:" << static_cast<int>(status);
    if (out != nullptr) Rust::gfr_crypto_free_string(out);
    TakeEngineErrorDetail(status, what);
    return FromRpgpStatus(status);
  }
  if (out == nullptr) {
    LOG_E() << what << "returned no key block for:" << fpr;
    return RpgpError(GFErrorCode::kGENERAL);
  }

  // Straight into secure memory: a QString copy of the secret key block would
  // be freed without being wiped.
  const GFBuffer edited(out, std::strlen(out));
  Rust::gfr_crypto_free_string(out);

  if (!ReplaceKeyInDatabaseRpgp(ctx.GetChannel(), edited, key->blocks)) {
    LOG_E() << "failed to store the key after:" << what << fpr;
    return RpgpError(GFErrorCode::kGENERAL);
  }
  return GFError{};
}

auto NoUsableRecipientsResult(const QList<GpgKey>& recipients)
    -> GFEncryptResult {
  GFEncryptResult result;
  for (const auto& recipient : recipients) {
    result.invalid_recipients.push_back(
        {recipient.Fingerprint(), RpgpError(GFErrorCode::kNO_PUBLIC_KEY)});
  }
  return result;
}

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
    -> std::tuple<GFError, GFEncryptResult> {
  LOG_D() << "Handling encrypt result. Rust status: " << static_cast<int>(err);

  GFError gf_err;
  GFEncryptResult result;
  if (err != Rust::GfrStatus::Success) {
    RecordEngineDetail(err, result.error_detail, "Encryption");

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Encryption cancelled by user.";
      gf_err = RpgpError(GFErrorCode::kCANCELED, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorFetchPasswordFailed) {
      LOG_E() << "Encryption failed: passphrase could not be obtained.";
      gf_err = RpgpError(GFErrorCode::kNO_PASSPHRASE, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Encryption failed: No data to encrypt.";
      gf_err = RpgpError(GFErrorCode::kINVALID_DATA, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorNoKey) {
      LOG_E() << "Encryption failed: No valid recipient keys provided.";
      gf_err = RpgpError(GFErrorCode::kNO_KEY, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorUnusableKey) {
      LOG_E() << "Encryption failed: no recipient key may be used.";
      // The engine lists every refused recipient with its own reason.
      result.invalid_recipients =
          ParseEncryptResultMeta(gfr_result.meta).invalid_recipients;
      gf_err = RpgpError(GFErrorCode::kUNUSABLE_PUBLIC_KEY, err);
      goto end;
    }

    LOG_E() << "Encryption failed, engine status:" << static_cast<int>(err);
    gf_err = FromRpgpStatus(err);
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
    -> std::tuple<GFError, GFDecryptResult> {
  LOG_D() << "Handling decrypt result. Rust status: " << static_cast<int>(err);

  GFError gf_err;
  GFDecryptResult result;
  if (err != Rust::GfrStatus::Success) {
    RecordEngineDetail(err, result.error_detail, "Decryption");

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Decryption cancelled by user.";
      gf_err = RpgpError(GFErrorCode::kCANCELED, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorFetchPasswordFailed) {
      LOG_E() << "Decryption failed: passphrase could not be obtained.";
      gf_err = RpgpError(GFErrorCode::kNO_PASSPHRASE, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Decryption failed: No encrypted data found.";
      gf_err = RpgpError(GFErrorCode::kINVALID_DATA, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorBadPassphrase) {
      LOG_E() << "Decryption failed: Incorrect passphrase.";
      gf_err = RpgpError(GFErrorCode::kBAD_PASSPHRASE, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorDecryptionFailed) {
      LOG_E() << "Decryption failed: Decryption process failed. This could be "
                 "due to incorrect passphrase, bad key, or corrupted data.";
      gf_err = RpgpError(GFErrorCode::kDECRYPT_FAILED, err);
      goto end;
    }

    // RFC 9580 §13.7: a failed MDC/AEAD check must read as an integrity
    // problem, not as a generic failure. No output was kept by the engine.
    if (err == Rust::GfrStatus::ErrorDataCorrupted) {
      LOG_E() << "Decryption failed: the message failed its integrity check.";
      gf_err = RpgpError(GFErrorCode::kBAD_DATA, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorIo) {
      LOG_E() << "Decryption failed: I/O error reading the input or writing "
                 "the output.";
      gf_err = RpgpError(GFErrorCode::kIO, err);
      goto end;
    }

    // A wrong passphrase comes back as ErrorBadPassphrase (handled above).
    // ErrorNoKey for a recipient whose secret key is in the database means
    // that key could not be used: its stored secret does not parse (or is a
    // stub), or its key id collides with another key's. That is an unusable
    // secret key, not a passphrase problem.
    if (err == Rust::GfrStatus::ErrorNoKey) {
      LOG_E() << "Decryption failed: no usable secret key.";

      bool has_secret_recipient = false;
      result.recipients = SniffRecipients(key_db, in_buffer);
      for (auto& rec : result.recipients) {
        if (rec.status.code != GFErrorCode::kNO_SECRET_KEY) {
          rec.status = RpgpError(GFErrorCode::kUNUSABLE_SECRET_KEY, err);
          has_secret_recipient = true;
        }
      }
      // No secret key at all is GnuPG's NO_SECKEY, the same answer signing
      // without a secret key gives.
      gf_err = has_secret_recipient
                   ? RpgpError(GFErrorCode::kUNUSABLE_SECRET_KEY, err)
                   : RpgpError(GFErrorCode::kNO_SECRET_KEY, err);
      goto end;
    }

    LOG_E() << "Decryption failed, engine status:" << static_cast<int>(err);
    gf_err = FromRpgpStatus(err);
    goto end;
  }

  // success case, convert the result
  result = GfrDecryptResultC2GFDecryptResult(gfr_result);

end:
  return {gf_err, result};
}

auto HandleSignResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                      Rust::GfrSignResultC gfr_result)
    -> std::tuple<GFError, GFSignResult> {
  LOG_D() << "Handling sign result. Rust status: " << static_cast<int>(err);

  GFError gf_err;
  GFSignResult result;
  if (err != Rust::GfrStatus::Success) {
    RecordEngineDetail(err, result.error_detail, "Signing");

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Signing cancelled by user.";
      gf_err = RpgpError(GFErrorCode::kCANCELED, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorFetchPasswordFailed) {
      LOG_E() << "Signing failed: passphrase could not be obtained.";
      gf_err = RpgpError(GFErrorCode::kNO_PASSPHRASE, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Signing failed: No data to sign.";
      gf_err = RpgpError(GFErrorCode::kINVALID_DATA, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorBadPassphrase) {
      LOG_E() << "Signing failed: Incorrect passphrase.";
      gf_err = RpgpError(GFErrorCode::kBAD_PASSPHRASE, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorUnusableKey) {
      LOG_E() << "Signing failed: no key of the signer may sign.";
      gf_err = RpgpError(GFErrorCode::kUNUSABLE_SECRET_KEY, err);
      goto end;
    }

    LOG_E() << "Signing failed, engine status:" << static_cast<int>(err);
    gf_err = FromRpgpStatus(err);
    goto end;
  }

  // success case, convert the result
  result = GfrSignResultC2GFSignResult(gfr_result);

end:
  return {gf_err, result};
}

auto HandleVerifyResult(const GFBuffer& in_buffer, Rust::GfrStatus err,
                        Rust::GfrVerifyResultC gfr_result)
    -> std::tuple<GFError, GFVerifyResult> {
  LOG_D() << "Handling verify result. Rust status: " << static_cast<int>(err);

  GFError gf_err;
  GFVerifyResult result;
  if (err != Rust::GfrStatus::Success) {
    RecordEngineDetail(err, result.error_detail, "Verification");

    if (err == Rust::GfrStatus::ErrorCanceled) {
      LOG_D() << "Verification cancelled by user.";
      gf_err = RpgpError(GFErrorCode::kCANCELED, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorFetchPasswordFailed) {
      LOG_E() << "Verification failed: passphrase could not be obtained.";
      gf_err = RpgpError(GFErrorCode::kNO_PASSPHRASE, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      LOG_E() << "Verification failed: No signature data found.";
      gf_err = RpgpError(GFErrorCode::kINVALID_DATA, err);
      goto end;
    }

    if (err == Rust::GfrStatus::ErrorIo) {
      LOG_E() << "Verification failed: I/O error occurred while reading data.";
      gf_err = RpgpError(GFErrorCode::kIO, err);
      goto end;
    }

    // A failed verification never converts the engine's metadata.
    LOG_E() << "Verification failed, engine status:" << static_cast<int>(err);
    gf_err = FromRpgpStatus(err);
    goto end;
  }

  LOG_D() << "Verification success. Processing results...";

  // success case, convert the result
  result = GfrVerifyResultC2GFVerifyResult(gfr_result);

end:
  return {gf_err, result};
}

}  // namespace GpgFrontend
