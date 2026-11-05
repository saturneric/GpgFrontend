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

#include "KeyManagement.h"

#include <optional>

#include "core/GFCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/openpgp/KeyGroupRepository.h"
#include "core/function/rpgp/KeyImportExport.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/function/rpgp/ResultHandler.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

auto DeleteKeysRpgpImpl(OpenPGPContext& ctx, const GpgAbstractKeyPtrList& keys)
    -> bool {
  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return false;
  }

  // Every key is tried, and any one that could not be deleted is a failure:
  // this used to stop at a missing key after deleting the ones before it, and
  // report a key the database refused to delete as deleted.
  auto all_deleted = true;
  for (const auto& key : keys) {
    LOG_D() << "Deleting key: " << key->ID();

    if (key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP) {
      KeyGroupRepository::GetInstance(ctx.GetChannel()).Remove(key->ID());
      continue;
    }

    if (!key_db->GetKeyMetadata(key->ID())) {
      LOG_E() << "key not found in database: " << key->ID();
      all_deleted = false;
      continue;
    }

    if (!key_db->DeleteKey(key->ID())) {
      LOG_E() << "failed to delete key from database: " << key->ID();
      all_deleted = false;
    }
  }
  return all_deleted;
}

namespace {
/**
 * @brief Change the passphrase of a secret key block and write the result back.
 *
 * An empty target_fpr re-protects the whole key -- primary and every subkey --
 * under one new passphrase; otherwise only the key at target_fpr is touched.
 */
auto modify_key_passphrase(OpenPGPContext& ctx, const GpgKeyPtr& key,
                           const QString& target_fpr) -> GFError {
  const auto target_fpr_utf8 = target_fpr.toUtf8();
  // Reports a rejected passphrase or a cancel as such; collapsing it into a
  // general error would show "General error" for a wrong password.
  return EditSecretKeyRpgp(
      ctx, key->Fingerprint(), "Changing the passphrase",
      [&](const GFKeyMetadata&, const Rust::GfrBuffer& block, char** out) {
        // a null target selects the whole key
        return Rust::gfr_crypto_modify_key_password(
            ctx.GetChannel(), block,
            target_fpr.isEmpty() ? nullptr : target_fpr_utf8.constData(),
            FetchPasswordCallback, out);
      });
}
}  // namespace

auto ModifyKeyPassphraseRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                                 const DataObjectPtr& data_object) -> GFError {
  return modify_key_passphrase(ctx, key, {});
}

auto ModifySubkeyPassphraseRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                                    const SubkeyId& skey_fpr,
                                    const DataObjectPtr& data_object)
    -> GFError {
  if (skey_fpr.isEmpty()) {
    LOG_E() << "no subkey fingerprint given for key: " << key->Fingerprint();
    return RpgpError(GFErrorCode::kINVALID_ARGUMENT);
  }

  return modify_key_passphrase(ctx, key, skey_fpr);
}

auto DeleteSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                          int skey_idx) -> GFError {
  return EditSecretKeyRpgp(
      ctx, key->Fingerprint(), "Deleting a subkey",
      [&](const GFKeyMetadata& meta, const Rust::GfrBuffer& block, char** out) {
        if (skey_idx < 0 || skey_idx >= static_cast<int>(meta.subkeys.size())) {
          LOG_E() << "illegal subkey index:" << skey_idx
                  << "for key:" << meta.fpr;
          return Rust::GfrStatus::ErrorInvalidInput;
        }
        return Rust::gfr_crypto_delete_subkey(
            block, meta.subkeys[skey_idx].fpr.toUtf8().constData(), out);
      });
}

auto RevokeSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                          int subkey_index, int reason_code,
                          const QString& reason_text) -> GFError {
  const auto reason_text_utf8 = reason_text.toUtf8();

  auto mapped_reason_code = Rust::GfrRevocationCode::NoReason;
  switch (reason_code) {
    case 1:
      mapped_reason_code = Rust::GfrRevocationCode::Compromised;
      break;
    case 2:
      mapped_reason_code = Rust::GfrRevocationCode::Superseded;
      break;
    case 3:
      mapped_reason_code = Rust::GfrRevocationCode::Retired;
      break;
    default:
      break;
  }

  return EditSecretKeyRpgp(
      ctx, key->Fingerprint(), "Revoking a subkey",
      [&](const GFKeyMetadata& meta, const Rust::GfrBuffer& block, char** out) {
        if (subkey_index < 0 ||
            subkey_index >= static_cast<int>(meta.subkeys.size())) {
          LOG_E() << "illegal subkey index:" << subkey_index
                  << "for key:" << meta.fpr;
          return Rust::GfrStatus::ErrorInvalidInput;
        }
        return Rust::gfr_crypto_revoke_subkey(
            ctx.GetChannel(), block,
            meta.subkeys[subkey_index].fpr.toUtf8().constData(),
            mapped_reason_code, reason_text_utf8.constData(),
            FetchPasswordCallback, out);
      });
}

auto GenerateRevCertRpgpImpl(OpenPGPContext& ctx_, const GpgKeyPtr& key,
                             const QString& output_path, int reason_code,
                             const QString& reason_text) -> bool {
  auto key_db = ctx_.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return false;
  }

  auto meta = key_db->GetKeyMetadata(key->Fingerprint());
  if (!meta) {
    LOG_E() << "key metadata not found in database for key: "
            << key->Fingerprint();
    return false;
  }

  auto key_block_data = key_db->GetKeyBlocks(meta->fpr);
  if (!key_block_data) {
    LOG_E() << "key block data not found in database for key with fpr: "
            << meta->fpr;
    return false;
  }

  if (key_block_data->secret_key.Empty()) {
    LOG_E() << "secret key block is empty for key with fpr: " << meta->fpr;
    return false;
  }

  auto key_block_utf8 = key_block_data->secret_key;
  if (key_block_utf8 == nullptr) {
    LOG_E() << "key block data is null for key with fpr: " << meta->fpr;
    return false;
  }

  auto reason_text_utf8 = reason_text.toUtf8();

  int mapped_reason_code = 0;
  switch (reason_code) {
    case 1:
      mapped_reason_code =
          static_cast<int>(Rust::GfrRevocationCode::Compromised);
      break;
    case 2:
      mapped_reason_code =
          static_cast<int>(Rust::GfrRevocationCode::Superseded);
      break;
    case 3:
      mapped_reason_code = static_cast<int>(Rust::GfrRevocationCode::Retired);
      break;
    default:
      mapped_reason_code = 0;
      break;
  }

  auto key_fpr_utf8 = key->Fingerprint().toUtf8();
  char* out_secret_block = nullptr;

  Rust::GfrBuffer key_block_buffer = {
      reinterpret_cast<const uint8_t*>(key_block_utf8.Data()),
      key_block_utf8.Size()};

  auto err = Rust::gfr_crypto_generate_key_rev_cert(
      ctx_.GetChannel(), key_block_buffer,
      static_cast<Rust::GfrRevocationCode>(mapped_reason_code),
      reason_text_utf8.constData(), FetchPasswordCallback, &out_secret_block);

  if (err != Rust::GfrStatus::Success) {
    // Taken, not only logged: left behind, it was reported as the detail of
    // whatever failed next on this thread.
    LOG_E() << "generating a revocation certificate failed:"
            << TakeEngineErrorDetail(err, "generate revocation certificate");
    return false;
  }

  if (out_secret_block == nullptr) {
    LOG_E() << "gfr_crypto_generate_key_rev_cert returned null secret block";
    return false;
  }

  auto out_block_str = QString::fromUtf8(out_secret_block);
  Rust::gfr_crypto_free_string(out_secret_block);
  return WriteFileGFBuffer(output_path, GFBuffer(out_block_str));
}

auto SetExpireRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                       const SubkeyId& skey_fpr,
                       const std::optional<QDateTime>& expires) -> GFError {
  // A null target selects the primary key. The GnuPG impl treats an empty
  // fingerprint or one matching the primary the same way, so mirror that here.
  const auto target_is_primary =
      skey_fpr.isEmpty() || skey_fpr == key->Fingerprint();
  const auto target_fpr_utf8 = skey_fpr.toUtf8();

  // 0 means "never expires"; otherwise pass the absolute Unix time. Unlike the
  // GnuPG impl (which uses seconds-from-now), the rPGP engine takes an absolute
  // epoch and derives the duration from the key's creation time internally.
  const auto expires_epoch =
      expires.has_value() ? static_cast<uint64_t>(expires->toSecsSinceEpoch())
                          : 0;

  return EditSecretKeyRpgp(
      ctx, key->Fingerprint(), "Changing the expiration",
      [&](const GFKeyMetadata&, const Rust::GfrBuffer& block, char** out) {
        return Rust::gfr_crypto_update_key_expiration(
            ctx.GetChannel(), block,
            target_is_primary ? nullptr : target_fpr_utf8.constData(),
            expires_epoch, FetchPasswordCallback, out);
      });
}

auto GetEcdhKdfParamsRpgpImpl(const GFBuffer& public_key_block,
                              const QString& subkey_fpr)
    -> std::tuple<GFError, QString> {
  if (public_key_block.Empty() || subkey_fpr.isEmpty()) {
    return {RpgpError(GFErrorCode::kINVALID_ARGUMENT), {}};
  }

  Rust::GfrBuffer block_buffer = {
      reinterpret_cast<const uint8_t*>(public_key_block.Data()),
      public_key_block.Size()};

  char* out_hex = nullptr;
  auto err = Rust::gfr_crypto_get_ecdh_kdf_params(
      block_buffer, subkey_fpr.toUtf8().constData(), &out_hex);

  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_get_ecdh_kdf_params error, code: "
            << static_cast<int>(err);
    // the caller only distinguishes "not an ECDH key we can handle" from
    // "malformed request"; collapse the Rust status into that.
    if (err == Rust::GfrStatus::ErrorInvalidInput) {
      return {RpgpError(GFErrorCode::kINVALID_ARGUMENT, err), {}};
    }
    return {RpgpError(GFErrorCode::kWRONG_KEY_USAGE, err), {}};
  }

  if (out_hex == nullptr) {
    LOG_E() << "gfr_crypto_get_ecdh_kdf_params returned null";
    return {RpgpError(GFErrorCode::kGENERAL), {}};
  }

  auto hex = QString::fromUtf8(out_hex);
  Rust::gfr_crypto_free_string(out_hex);
  return {GFError{}, hex};
}
}  // namespace GpgFrontend