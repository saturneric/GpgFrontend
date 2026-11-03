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

#include "core/function/gpg/GpgModelConvert.h"

#include "core/function/gpg/GpgErrorMap.h"

namespace GpgFrontend {

auto ConvertKeySignature(gpgme_key_sig_t sig) -> GFKeySignature {
  GFKeySignature out;
  if (sig == nullptr) return out;
  out.revoked = sig->revoked != 0;
  out.expired = sig->expired != 0;
  out.invalid = sig->invalid != 0;
  out.exportable = sig->exportable != 0;
  out.status = FromGpgError(sig->status);
  out.key_id = QString::fromUtf8(sig->keyid);
  out.pubkey_algo = QString::fromUtf8(gpgme_pubkey_algo_name(sig->pubkey_algo));
  out.created = sig->timestamp;
  out.expires = sig->expires;
  out.uid = QString::fromUtf8(sig->uid);
  out.name = QString::fromUtf8(sig->name);
  out.email = QString::fromUtf8(sig->email);
  out.comment = QString::fromUtf8(sig->comment);
  return out;
}

auto ConvertTofuInfo(gpgme_tofu_info_t info) -> GFTofuInfo {
  GFTofuInfo out;
  if (info == nullptr) return out;
  out.validity = info->validity;
  out.policy = info->policy;
  out.sign_count = info->signcount;
  out.encr_count = info->encrcount;
  out.sign_first = info->signfirst;
  out.sign_last = info->signlast;
  out.encr_last = info->encrlast;
  out.description = QString::fromUtf8(info->description);
  return out;
}

auto ConvertUserId(gpgme_user_id_t uid) -> GFUserId {
  GFUserId out;
  if (uid == nullptr) return out;
  out.name = QString::fromUtf8(uid->name);
  out.email = QString::fromUtf8(uid->email);
  out.comment = QString::fromUtf8(uid->comment);
  out.raw = QString::fromUtf8(uid->uid);
  out.is_revoked = uid->revoked != 0;
  out.is_invalid = uid->invalid != 0;
  for (auto* sig = uid->signatures; sig != nullptr; sig = sig->next) {
    out.signatures.append(ConvertKeySignature(sig));
  }
  for (auto* info = uid->tofu; info != nullptr; info = info->next) {
    out.tofu.append(ConvertTofuInfo(info));
  }
  return out;
}

namespace {
auto ValidityOf(gpgme_validity_t validity) -> GpgValidity {
  switch (validity) {
    case GPGME_VALIDITY_UNDEFINED:
      return GpgValidity::kUNDEFINED;
    case GPGME_VALIDITY_NEVER:
      return GpgValidity::kNEVER;
    case GPGME_VALIDITY_MARGINAL:
      return GpgValidity::kMARGINAL;
    case GPGME_VALIDITY_FULL:
      return GpgValidity::kFULL;
    case GPGME_VALIDITY_ULTIMATE:
      return GpgValidity::kULTIMATE;
    case GPGME_VALIDITY_UNKNOWN:
    default:
      return GpgValidity::kUNKNOWN;
  }
}
auto SummaryOf(gpgme_sigsum_t summary) -> std::uint32_t {
  std::uint32_t flags = kSIGSUM_NONE;
  const auto map = [&](unsigned gpgme_bit, std::uint32_t ours) {
    if ((summary & gpgme_bit) != 0) flags |= ours;
  };
  map(GPGME_SIGSUM_VALID, kSIGSUM_VALID);
  map(GPGME_SIGSUM_GREEN, kSIGSUM_GREEN);
  map(GPGME_SIGSUM_RED, kSIGSUM_RED);
  map(GPGME_SIGSUM_KEY_REVOKED, kSIGSUM_KEY_REVOKED);
  map(GPGME_SIGSUM_KEY_EXPIRED, kSIGSUM_KEY_EXPIRED);
  map(GPGME_SIGSUM_SIG_EXPIRED, kSIGSUM_SIG_EXPIRED);
  map(GPGME_SIGSUM_KEY_MISSING, kSIGSUM_KEY_MISSING);
  map(GPGME_SIGSUM_CRL_MISSING, kSIGSUM_CRL_MISSING);
  return flags;
}

}  // namespace

auto ConvertSubKey(gpgme_subkey_t s_key) -> GFSubKeyMetadata {
  GFSubKeyMetadata out;
  if (s_key == nullptr) return out;
  out.fpr = QString::fromUtf8(s_key->fpr);
  out.key_id = QString::fromUtf8(s_key->keyid);
  out.key_length = s_key->length;
  out.created_at = s_key->timestamp;
  out.expires_at = s_key->expires;
  out.has_secret = s_key->secret != 0;
  out.is_revoked = s_key->revoked != 0;
  out.can_sign = s_key->can_sign != 0;
  out.can_encrypt = s_key->can_encrypt != 0;
  out.can_auth = s_key->can_authenticate != 0;
  out.can_certify = s_key->can_certify != 0;
  out.keygrip = QString::fromUtf8(s_key->keygrip);
  out.is_disabled = s_key->disabled != 0;
  out.is_card_key = s_key->is_cardkey != 0;
  out.card_serial = QString::fromLatin1(s_key->card_number);
  out.can_renc = s_key->can_renc != 0;
  out.is_expired = s_key->expired != 0;
  out.algo_name = QString::fromUtf8(gpgme_pubkey_algo_name(s_key->pubkey_algo));
  auto* algo = gpgme_pubkey_algo_string(s_key);
  out.algo_string = QString::fromUtf8(algo);
  gpgme_free(algo);
  return out;
}

auto AdoptGpgmeKey(gpgme_key_t key) -> GFKeyMetadata {
  GFKeyMetadata out;
  if (key == nullptr) return out;
  out.native = std::shared_ptr<void>(key, [](void* ptr) {
    if (ptr != nullptr) gpgme_key_unref(static_cast<gpgme_key_t>(ptr));
  });

  out.fpr = QString::fromUtf8(key->fpr);
  out.has_secret = key->secret != 0;
  out.is_revoked = key->revoked != 0;
  out.is_disabled = key->disabled != 0;
  out.is_expired = key->expired != 0;
  out.update_time = static_cast<qint64>(key->last_update);
  out.owner_trust = ValidityOf(key->owner_trust);
  out.protocol = QString::fromUtf8(gpgme_get_protocol_name(key->protocol));

  for (auto* s_key = key->subkeys; s_key != nullptr; s_key = s_key->next) {
    out.subkeys.append(ConvertSubKey(s_key));
  }
  if (!out.subkeys.isEmpty()) {
    // GnuPG lists the primary key first; the key's own facts are its.
    const auto& primary = out.subkeys.front();
    out.key_id = primary.key_id;
    out.key_length = primary.key_length;
    out.created_at = primary.created_at;
    out.expires_at = primary.expires_at;
    out.can_sign = primary.can_sign;
    out.can_encrypt = primary.can_encrypt;
    out.can_auth = primary.can_auth;
    out.can_certify = primary.can_certify;
  }

  for (auto* uid = key->uids; uid != nullptr; uid = uid->next) {
    out.user_ids.append(ConvertUserId(uid));
  }
  if (!out.user_ids.isEmpty()) out.user_ids.front().is_primary = true;
  return out;
}

auto ConvertSignature(gpgme_signature_t sig) -> GFSignature {
  GFSignature out;
  if (sig == nullptr) return out;
  out.issuer_fpr = QString::fromUtf8(sig->fpr);
  out.created_at = static_cast<uint32_t>(sig->timestamp);
  out.expires_at = static_cast<qint64>(sig->exp_timestamp);
  out.pub_algo = QString::fromUtf8(gpgme_pubkey_algo_name(sig->pubkey_algo));
  out.hash_algo = QString::fromUtf8(gpgme_hash_algo_name(sig->hash_algo));
  out.engine_status = FromGpgError(sig->status);
  out.validity = ValidityOf(sig->validity);
  out.summary = SummaryOf(sig->summary);
  out.has_trust_model = true;
  return out;
}

auto SignResultOf(gpgme_sign_result_t result) -> GpgSignResult {
  if (result == nullptr) return {};
  GFSignResult out;
  for (auto* invalid = result->invalid_signers; invalid != nullptr;
       invalid = invalid->next) {
    out.invalid_signers.append(
        {QString::fromUtf8(invalid->fpr), FromGpgError(invalid->reason)});
  }
  for (auto* signature = result->signatures; signature != nullptr;
       signature = signature->next) {
    GFSignature sig;
    sig.created_at = signature->timestamp;
    sig.hash_algo =
        QString::fromUtf8(gpgme_hash_algo_name(signature->hash_algo));
    sig.pub_algo =
        QString::fromUtf8(gpgme_pubkey_algo_name(signature->pubkey_algo));
    sig.issuer_fpr = QString::fromUtf8(signature->fpr);
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
    out.signatures.append(sig);
  }
  return GpgSignResult(out);
}

auto VerifyResultOf(gpgme_verify_result_t result) -> GpgVerifyResult {
  if (result == nullptr) return {};
  GFVerifyResult out;
  for (auto* sig = result->signatures; sig != nullptr; sig = sig->next) {
    out.signatures.append(ConvertSignature(sig));
  }
  return GpgVerifyResult(out);
}

auto ConvertRecipient(gpgme_recipient_t r) -> GFRecipient {
  GFRecipient out;
  if (r == nullptr) return out;
  out.key_id = QString::fromUtf8(r->keyid);
  out.pub_algo = QString::fromUtf8(gpgme_pubkey_algo_name(r->pubkey_algo));
  out.status = FromGpgError(r->status);
  return out;
}

auto EncryptResultOf(gpgme_encrypt_result_t result) -> GpgEncryptResult {
  if (result == nullptr) return {};
  GFEncryptResult out;
  for (auto* invalid = result->invalid_recipients; invalid != nullptr;
       invalid = invalid->next) {
    out.invalid_recipients.append(
        {QString::fromUtf8(invalid->fpr), FromGpgError(invalid->reason)});
  }
  return GpgEncryptResult(out);
}

auto DecryptResultOf(gpgme_decrypt_result_t result) -> GpgDecryptResult {
  if (result == nullptr) return {};
  GFDecryptResult out;
  out.filename = QString::fromUtf8(result->file_name);
  out.symkey_algo = QString::fromUtf8(result->symkey_algo);
  out.unsupported_algorithm = QString::fromUtf8(result->unsupported_algorithm);
  out.is_mime = result->is_mime != 0;
  out.integrity_protected = result->legacy_cipher_nomdc == 0;
  for (auto* r = result->recipients; r != nullptr; r = r->next) {
    out.recipients.append(ConvertRecipient(r));
  }
  return GpgDecryptResult(out);
}

auto GpgmeKeyOf(const GpgKey& key) -> gpgme_key_t {
  return static_cast<gpgme_key_t>(key.Native().get());
}

}  // namespace GpgFrontend
