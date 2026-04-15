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

#include "FileCryptoOpera.h"

#include "core/GpgCoreRust.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {

auto EncryptFileRpgpImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& keys,
                         const QString& in_path, bool ascii,
                         const QString& out_path,
                         const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  // 1. Vector to hold the actual memory of the UTF-8 strings
  QContainer<QByteArray> key_blocks_utf8 =
      GetPublicKeysByKeyIdsForEncryption(*key_db, keys);
  if (key_blocks_utf8.empty()) {
    LOG_E() << "No valid recipients found for encryption.";
    return GPG_ERR_GENERAL;  // Or appropriate error code
  }

  // 2. Vector to hold the pointers to pass to Rust FFI
  std::vector<const char*> recipient_cstrs;
  recipient_cstrs.reserve(key_blocks_utf8.size());
  for (const auto& ba : key_blocks_utf8) {
    recipient_cstrs.push_back(ba.constData());
  }

  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();
  Rust::GfrEncryptResultC encrypt_result;

  // Call Rust FFI. Ensure in_buffer is a null-terminated C-string if Rust
  // expects it.
  auto status = Rust::gfr_crypto_encrypt_file(
      in_file_path_utf8.constData(), out_file_path_utf8.constData(),
      recipient_cstrs.data(), recipient_cstrs.size(), ascii, &encrypt_result);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Rust FFI encryption failed.";
    return GPG_ERR_GENERAL;
  }

  GFEncryptResult result = GfrEncryptResultC2GFEncryptResult(encrypt_result);
  Rust::gfr_crypto_free_encrypt_result(&encrypt_result);

  data_object->Swap({GpgEncryptResult(result)});
  return GPG_ERR_NO_ERROR;
}

auto DecryptFileRpgpImpl(GpgContext& ctx_, const QString& in_path,
                         const QString& out_path,
                         const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  Rust::GfrDecryptResultC decrypt_result;
  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();

  auto err = Rust::gfr_crypto_decrypt_file(
      ctx_.GetChannel(), in_file_path_utf8.constData(),
      out_file_path_utf8.constData(), false, FetchSecretKeyCallback,
      FetchPasswordCallback, FreeCallback, key_db.data(), &decrypt_result);

  if (err != Rust::GfrStatus::Success) {
    if (err == Rust::GfrStatus::ErrorDecryptionFailed) {
      data_object->Swap({GpgDecryptResult{}, GFBuffer()});
      return GPG_ERR_BAD_PASSPHRASE;
    }

    LOG_E() << "Rust FFI decryption failed."
            << " Status: " << static_cast<int>(err);
    return GPG_ERR_GENERAL;
  }

  GFDecryptResult result = GfrDecryptResultC2GFDecryptResult(decrypt_result);
  Rust::gfr_crypto_free_decrypt_result(&decrypt_result);

  data_object->Swap({
      GpgDecryptResult(result),
  });

  return GPG_ERR_NO_ERROR;
}

auto SignFileRpgpImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& keys,
                      const QString& in_path, bool ascii,
                      const QString& out_path, const DataObjectPtr& data_object)
    -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  // For signing, we typically use the first key in the list
  if (keys.empty()) {
    LOG_E() << "No signing key provided.";
    return GPG_ERR_GENERAL;
  }

  auto key_block_utf8 = GetSecretKeysByKeyIdForSigning(*key_db, keys);
  if (key_block_utf8.isEmpty()) {
    LOG_E() << "Failed to retrieve secret key for signing.";
    return GPG_ERR_GENERAL;
  }

  std::vector<const char*> key_block_cstrs;
  for (const auto& ba : key_block_utf8) {
    key_block_cstrs.push_back(ba.constData());
  }

  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();
  Rust::GfrSignResultC sign_result;

  auto status = Rust::gfr_crypto_sign_file(
      ctx_.GetChannel(), in_file_path_utf8.constData(),
      out_file_path_utf8.constData(), key_block_cstrs.data(),
      key_block_cstrs.size(), FetchPasswordCallback, FreeCallback,
      Rust::GfrSignMode::Detached, ascii, &sign_result);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Rust FFI signing failed.";
    return GPG_ERR_GENERAL;
  }

  GFSignResult result = GfrSignResultC2GFSignResult(sign_result);
  Rust::gfr_crypto_free_sign_result(&sign_result);

  data_object->Swap({GpgSignResult(result)});
  return GPG_ERR_NO_ERROR;
}

auto VerifyFileRpgpImpl(GpgContext& ctx_, const QString& data_path,
                        const QString& sign_path,
                        const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  Rust::GfrVerifyResultC verify_result;
  auto data_file_path_utf8 = data_path.toUtf8();
  auto sign_file_path_utf8 = sign_path.toUtf8();

  Rust::GfrStatus err;
  if (sign_path.isEmpty()) {
    err = Rust::gfr_crypto_verify_file(
        data_file_path_utf8.constData(), sign_file_path_utf8.constData(),
        nullptr, FetchPublicKeyCallback, FreeCallback, key_db.data(),
        Rust::GfrSignMode::Inline, &verify_result);
  } else {
    err = Rust::gfr_crypto_verify_file(
        data_file_path_utf8.constData(), sign_file_path_utf8.constData(),
        nullptr, FetchPublicKeyCallback, FreeCallback, key_db.data(),
        Rust::GfrSignMode::Detached, &verify_result);
  }

  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "Rust FFI verification failed."
            << " Status: " << static_cast<int>(err);
    return GPG_ERR_GENERAL;
  }

  GFVerifyResult result = GfrVerifyResultC2GFVerifyResult(verify_result);
  Rust::gfr_crypto_free_verify_result(&verify_result);

  data_object->Swap({GpgVerifyResult(result)});
  return GPG_ERR_NO_ERROR;
}

auto EncryptSignFileRpgpImpl(GpgContext& ctx,
                             const GpgAbstractKeyPtrList& enc_keys,
                             const GpgAbstractKeyPtrList& sign_keys,
                             const QString& in_path, bool ascii,
                             const QString& out_path,
                             const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  if (sign_keys.isEmpty() || enc_keys.isEmpty()) return GPG_ERR_INV_ARG;

  // 1. Vector to hold the actual memory of the UTF-8 strings
  QContainer<QByteArray> key_blocks_utf8;

  // 2. Vector to hold the pointers to pass to Rust FFI
  std::vector<const char*> recipient_cstrs;

  for (const auto& key : enc_keys) {
    auto key_block = key_db->GetKeyBlocks(key->Fingerprint());
    if (!key_block || key_block->public_key.isEmpty()) {
      LOG_W() << "No valid public key block found for key with fpr: "
              << key->Fingerprint();
      continue;
    }

    // Keep the QByteArray alive by pushing it to the vector
    key_blocks_utf8.push_back(key_block->public_key.toUtf8());
  }

  if (key_blocks_utf8.empty()) {
    LOG_E() << "No valid recipients found for encryption.";
    return GPG_ERR_GENERAL;  // Or appropriate error code
  }

  // Pre-allocate space for performance
  recipient_cstrs.reserve(key_blocks_utf8.size());

  // Safely extract pointers from the valid memory blocks
  for (const auto& ba : key_blocks_utf8) {
    recipient_cstrs.push_back(ba.constData());
  }

  auto skey_utf8_list = GetSecretKeysByKeyIdForSigning(*key_db, sign_keys);
  std::vector<const char*> c_skeys;

  // Fetch key blocks and safely store memory

  // Extract C-string pointers
  c_skeys.reserve(skey_utf8_list.size());
  for (const auto& i : skey_utf8_list) {
    c_skeys.push_back(i.constData());
  }

  QString name;
  Rust::GfrEncryptAndSignResultC encrypt_sign_result;

  auto err = Rust::gfr_crypto_encrypt_and_sign_file(
      ctx.GetChannel(), in_path.toUtf8().constData(),
      out_path.toUtf8().constData(), recipient_cstrs.data(),
      recipient_cstrs.size(), c_skeys.data(), c_skeys.size(),
      FetchPasswordCallback, FreeCallback, ascii, &encrypt_sign_result);

  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "Rust FFI encrypt_and_sign failed with status: "
            << static_cast<int>(err);
    return GPG_ERR_GENERAL;
  }

  GFEncryptAndSignResult result =
      GfrEncryptAndSignResultC2GFEncryptAndSignResult(encrypt_sign_result);
  Rust::gfr_crypto_free_encrypt_and_sign_result(&encrypt_sign_result);

  data_object->Swap({
      GpgEncryptResult(result.encrypt_result),
      GpgSignResult(result.sign_result),
  });

  return GPG_ERR_NO_ERROR;
}

auto DecryptVerifyFileRpgpImpl(GpgContext& ctx, const QString& in_path,
                               const QString& out_path,
                               const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  Rust::GfrDecryptAndVerifyResultC decrypt_verify_result;

  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();

  auto err = Rust::gfr_crypto_decrypt_and_verify_file(
      ctx.GetChannel(), in_file_path_utf8.constData(),
      out_file_path_utf8.constData(), false, FetchSecretKeyCallback,
      FetchPasswordCallback, FetchPublicKeyCallback, FreeCallback,
      key_db.data(), &decrypt_verify_result);

  if (err != Rust::GfrStatus::Success) {
    if (err == Rust::GfrStatus::ErrorDecryptionFailed) {
      data_object->Swap({GpgDecryptResult{}, GpgVerifyResult{}});
      return GPG_ERR_BAD_PASSPHRASE;
    }

    LOG_E() << "Rust FFI decryption failed."
            << " Status: " << static_cast<int>(err);
    return GPG_ERR_GENERAL;
  }

  GFDecryptAndVerifyResult result =
      GfrDecryptAndVerifyResultC2GFDecryptAndVerifyResult(
          decrypt_verify_result);
  Rust::gfr_crypto_free_decrypt_and_verify_result(&decrypt_verify_result);

  // For simplicity, we can treat the verify result as part of the decrypt
  // result since they are closely related in this combined operation.
  data_object->Swap({
      GpgDecryptResult(result.decrypt_result),
      // We can add a placeholder for the verify result if needed
      GpgVerifyResult(result.verify_result),
  });

  return GPG_ERR_NO_ERROR;
}

}  // namespace GpgFrontend