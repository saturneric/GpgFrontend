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

#include "MessageCryptoOperation.h"

#include "core/GFCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/function/rpgp/ResultHandler.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto EncryptRpgpImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& keys,
                     const GFBuffer& in_buffer, bool ascii,
                     const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  auto recipients = Convert2GpgKeyList(ctx_.GetChannel(), keys);

  // 1. Vector to hold the actual memory of the UTF-8 strings
  QContainer<GFBuffer> key_blocks_utf8 =
      GetPublicKeysByKeyIdsForEncryption(*key_db, recipients);
  if (key_blocks_utf8.empty()) {
    LOG_E() << "No valid recipients found for encryption.";
    return GPG_ERR_GENERAL;  // Or appropriate error code
  }

  // 2. Vector to hold the pointers to pass to Rust FFI
  std::vector<const char*> recipient_cstrs;
  recipient_cstrs.reserve(key_blocks_utf8.size());
  for (const auto& ba : key_blocks_utf8) {
    recipient_cstrs.push_back(ba.Data());
  }

  std::string name;
  Rust::GfrEncryptResultC encrypt_result;
  memset(&encrypt_result, 0, sizeof(encrypt_result));

  // Call Rust FFI. Ensure in_buffer is a null-terminated C-string if Rust
  // expects it.
  auto status = Rust::gfr_crypto_encrypt_data(
      name.c_str(), reinterpret_cast<const uint8_t*>(in_buffer.Data()),
      in_buffer.Size(), recipient_cstrs.data(), recipient_cstrs.size(), ascii,
      &encrypt_result);

  auto [gf_err, result] =
      HandleEncryptResult(in_buffer, status, encrypt_result);
  Rust::gfr_crypto_free_encrypt_result(&encrypt_result);

  data_object->Swap({GpgEncryptResult(result), result.data});
  return gf_err;
}

auto DecryptRpgpImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                     const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  Rust::GfrDecryptResultC decrypt_result;
  memset(&decrypt_result, 0, sizeof(decrypt_result));

  auto err = Rust::gfr_crypto_decrypt_data(
      ctx_.GetChannel(), reinterpret_cast<const uint8_t*>(in_buffer.Data()),
      in_buffer.Size(), FetchSecretKeyCallback, FetchPasswordCallback,
      key_db.data(), &decrypt_result);

  auto [gf_err, result] =
      HandleDecryptResult(*key_db, in_buffer, err, decrypt_result);
  Rust::gfr_crypto_free_decrypt_result(&decrypt_result);

  data_object->Swap({
      GpgDecryptResult(result),
      result.data,
  });
  return gf_err;
}

namespace {
auto ExportKeyBlockForSigning(GFKeyDatabase& key_db,
                              const GpgAbstractKeyPtrList& keys)
    -> std::tuple<GpgError, QContainer<GFBuffer>> {
  QContainer<GFBuffer> skey_utf8_list;
  // Fetch key blocks and safely store memory
  for (const auto& signer : keys) {
    auto blocks = key_db.GetKeyBlocks(signer->Fingerprint());
    if (!blocks || blocks->secret_key.Empty()) {
      LOG_E() << "Failed to find secret key block for FPR: "
              << signer->Fingerprint();
      return {GPG_ERR_NO_SECKEY, {}};
    }

    auto skey_data = blocks->secret_key;
    auto data = GFBuffer();

    // If the signer is a GPG key, check if there's a marked subkey and use it
    // for signing if available and valid.
    if (signer->KeyType() == GpgAbstractKeyType::kGPG_KEY) {
      auto key = GetGpgKeyByGpgAbstractKey(signer.data());

      for (const auto& sub : key.SubKeys()) {
        if (sub.IsMarked()) {
          LOG_D() << "Using marked subkey with fpr: " << sub.Fingerprint()
                  << " for signing instead of primary key with fpr: "
                  << signer->Fingerprint();
          auto prefix = sub.Fingerprint().toUtf8() + "!\n";
          data.Combine({GFBuffer(prefix), skey_data});
          break;
        }

        LOG_D() << "No marked subkey found for key with fpr: "
                << signer->Fingerprint()
                << ", using primary key block for signing.";
        data = skey_data;
      }
    }

    skey_utf8_list.push_back(data);
  }

  return {GPG_ERR_NO_ERROR, skey_utf8_list};
}
}  // namespace

auto SignRpgpImpl(OpenPGPContext& ctx, const GpgAbstractKeyPtrList& signers,
                  const GFBuffer& in_buffer, GpgSignMode mode, bool ascii,
                  const DataObjectPtr& data_object) -> GpgError {
  if (signers.isEmpty()) {
    return GPG_ERR_INV_ARG;
  }

  auto key_db = ctx.KeyDatabase();
  if (!key_db) return GPG_ERR_GENERAL;

  auto [err, skey_utf8_list] = ExportKeyBlockForSigning(*key_db, signers);
  if (err != GPG_ERR_NO_ERROR) {
    return err;
  }

  // Extract C-string pointers
  std::vector<const char*> c_skeys;
  c_skeys.reserve(skey_utf8_list.size());
  for (const auto& i : skey_utf8_list) {
    c_skeys.push_back(i.Data());
  }

  QByteArray name_utf8;
  Rust::GfrSignMode rs_mode;

  if (mode == GPGME_SIG_MODE_DETACH) {
    rs_mode = Rust::GfrSignMode::Detached;
  } else if (mode == GPGME_SIG_MODE_CLEAR) {
    rs_mode = Rust::GfrSignMode::ClearText;
  } else {
    rs_mode = Rust::GfrSignMode::Inline;
  }

  Rust::GfrSignResultC sign_result;
  memset(&sign_result, 0, sizeof(sign_result));

  auto status = Rust::gfr_crypto_sign_data(
      ctx.GetChannel(), name_utf8.constData(),
      reinterpret_cast<const uint8_t*>(in_buffer.Data()), in_buffer.Size(),
      c_skeys.data(), c_skeys.size(), FetchPasswordCallback, rs_mode, ascii,
      &sign_result);

  auto [gf_err, result] = HandleSignResult(in_buffer, status, sign_result);
  Rust::gfr_crypto_free_sign_result(&sign_result);

  data_object->Swap({
      GpgSignResult(result),
      result.data,
  });

  return gf_err;
}

auto VerifyRpgpImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                    const GFBuffer& sig_buffer,
                    const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  Rust::GfrVerifyResultC verify_result;
  memset(&verify_result, 0, sizeof(verify_result));

  auto status = Rust::gfr_crypto_verify_data(
      reinterpret_cast<const uint8_t*>(in_buffer.Data()), in_buffer.Size(),
      reinterpret_cast<const uint8_t*>(sig_buffer.Data()), sig_buffer.Size(),
      FetchPublicKeyCallback, key_db.data(),
      sig_buffer.Empty() ? Rust::GfrSignMode::ClearText
                         : Rust::GfrSignMode::Detached,
      &verify_result);

  auto [gf_err, result] = HandleVerifyResult(in_buffer, status, verify_result);
  Rust::gfr_crypto_free_verify_result(&verify_result);

  LOG_D() << "Verification result: "
          << (result.is_verified ? "VALID" : "INVALID")
          << ", Signatures found: " << result.signatures.size();

  data_object->Swap({
      GpgVerifyResult{result},
      GFBuffer{},
  });
  return gf_err;
}

auto EncryptSignRpgpImpl(OpenPGPContext& ctx, const GpgAbstractKeyPtrList& keys,
                         const GpgAbstractKeyPtrList& signers,
                         const GFBuffer& in_buffer, bool ascii,
                         const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  if (signers.isEmpty() || keys.isEmpty()) return GPG_ERR_INV_ARG;

  auto recipients = Convert2GpgKeyList(ctx.GetChannel(), keys);

  QContainer<GFBuffer> key_blocks_utf8 =
      GetPublicKeysByKeyIdsForEncryption(*key_db, recipients);
  if (key_blocks_utf8.empty()) {
    LOG_E() << "No valid recipients found for encryption.";
    return GPG_ERR_GENERAL;  // Or appropriate error code
  }

  std::vector<const char*> recipient_cstrs;
  recipient_cstrs.reserve(key_blocks_utf8.size());
  for (const auto& ba : key_blocks_utf8) {
    recipient_cstrs.push_back(ba.Data());
  }

  auto skey_utf8_list = GetSecretKeysByKeyIdForSigning(*key_db, signers);

  // Extract C-string pointers
  std::vector<const char*> c_skeys;
  c_skeys.reserve(skey_utf8_list.size());
  for (const auto& i : skey_utf8_list) {
    c_skeys.push_back(i.Data());
  }

  QString name;
  Rust::GfrEncryptAndSignResultC encrypt_sign_result;
  memset(&encrypt_sign_result, 0, sizeof(encrypt_sign_result));

  auto err = Rust::gfr_crypto_encrypt_and_sign_data(
      ctx.GetChannel(), name.toUtf8().constData(),
      reinterpret_cast<const uint8_t*>(in_buffer.Data()), in_buffer.Size(),
      recipient_cstrs.data(), recipient_cstrs.size(), c_skeys.data(),
      c_skeys.size(), FetchPasswordCallback, ascii, &encrypt_sign_result);

  auto [gf_err, encrypt_result] =
      HandleEncryptResult(in_buffer, err,
                          Rust::GfrEncryptResultC{
                              .data = encrypt_sign_result.data,
                              .data_len = encrypt_sign_result.data_len,
                              .meta = encrypt_sign_result.encrypt_meta,
                          });

  auto [gf_err_2, sign_result] = HandleSignResult(
      in_buffer, err,
      Rust::GfrSignResultC{.data = encrypt_sign_result.data,
                           .data_len = encrypt_sign_result.data_len,
                           .meta = encrypt_sign_result.sign_meta});
  Rust::gfr_crypto_free_encrypt_and_sign_result(&encrypt_sign_result);

  data_object->Swap({
      GpgEncryptResult(encrypt_result),
      GpgSignResult(sign_result),
      encrypt_result.data,
  });

  return GPG_ERR_NO_ERROR;
}

auto DecryptVerifyRpgpImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                           const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  Rust::GfrDecryptAndVerifyResultC decrypt_verify_result;
  memset(&decrypt_verify_result, 0, sizeof(decrypt_verify_result));

  auto err = Rust::gfr_crypto_decrypt_and_verify_data(
      ctx_.GetChannel(), reinterpret_cast<const uint8_t*>(in_buffer.Data()),
      in_buffer.Size(), FetchSecretKeyCallback, FetchPasswordCallback,
      FetchPublicKeyCallback, key_db.data(), &decrypt_verify_result);

  auto [gf_err, decrypt_result] =
      HandleDecryptResult(*key_db, in_buffer, err,
                          Rust::GfrDecryptResultC{
                              .data = decrypt_verify_result.data,
                              .data_len = decrypt_verify_result.data_len,
                              .meta = decrypt_verify_result.decrypt_meta,
                          });

  auto [gf_err_2, verify_result] = HandleVerifyResult(
      in_buffer, err,
      Rust::GfrVerifyResultC{.meta = decrypt_verify_result.verify_meta});

  Rust::gfr_crypto_free_decrypt_and_verify_result(&decrypt_verify_result);

  data_object->Swap({
      GpgDecryptResult(decrypt_result),
      GpgVerifyResult(verify_result),
      decrypt_result.data,
  });

  return (gf_err != GPG_ERR_NO_ERROR) ? gf_err : gf_err_2;
}

auto EncryptSymmetricRpgpImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                              bool ascii, const DataObjectPtr& data_object)
    -> GpgError {
  std::string name;
  Rust::GfrEncryptResultC encrypt_result;
  memset(&encrypt_result, 0, sizeof(encrypt_result));

  auto status = Rust::gfr_crypto_encrypt_data_symmetric(
      ctx_.GetChannel(), name.c_str(),
      reinterpret_cast<const uint8_t*>(in_buffer.Data()), in_buffer.Size(),
      ascii, FetchPasswordCallback, &encrypt_result);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Rust FFI symmetric encryption failed.";
    return GPG_ERR_GENERAL;
  }

  GFEncryptResult result = GfrEncryptResultC2GFEncryptResult(encrypt_result);
  Rust::gfr_crypto_free_encrypt_result(&encrypt_result);

  data_object->Swap({
      GpgEncryptResult(result),
      result.data,
  });
  return GPG_ERR_NO_ERROR;
}

}  // namespace GpgFrontend