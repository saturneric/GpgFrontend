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
#include "core/function/rpgp/ResultHandler.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/AsyncUtils.h"
#include "utils/GpgUtils.h"

namespace GpgFrontend {

auto EncryptFileRpgpImpl(OpenPGPContext& ctx_,
                         const GpgAbstractKeyPtrList& keys,
                         const QString& in_path, bool ascii,
                         const QString& out_path,
                         const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  auto recipients = Convert2GpgKeyList(ctx_.GetChannel(), keys);

  // 1. Vector to hold the actual memory of the UTF-8 strings
  auto key_blocks_utf8 =
      GetPublicKeysByKeyIdsForEncryption(*key_db, recipients);
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

  Rust::GfrStatus status;
  Rust::GfrEncryptResultC encrypt_result;
  memset(&encrypt_result, 0, sizeof(encrypt_result));

  auto is_directory = QFileInfo(in_path).isDir();
  if (is_directory) {
    // For directory encryption, we need to create a tar archive in memory first
    // and then pass it to the Rust FFI. The Rust FFI will handle the encryption
    // of the tar archive.
    status = Rust::gfr_crypto_encrypt_directory(
        in_file_path_utf8.constData(), out_file_path_utf8.constData(),
        recipient_cstrs.data(), recipient_cstrs.size(), ascii, &encrypt_result);
  } else {
    // Call Rust FFI. Ensure in_buffer is a null-terminated C-string if Rust
    // expects it.
    status = Rust::gfr_crypto_encrypt_file(
        in_file_path_utf8.constData(), out_file_path_utf8.constData(),
        recipient_cstrs.data(), recipient_cstrs.size(), ascii, &encrypt_result);
  }

  auto [gf_err, result] = HandleEncryptResult({}, status, encrypt_result);
  Rust::gfr_crypto_free_encrypt_result(&encrypt_result);

  data_object->Swap({GpgEncryptResult(result)});
  return gf_err;
}

auto EncryptDirRpgpImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& keys,
                        const QString& in_path, bool ascii,
                        const QString& out_path, const GpgOperationCallback& cb)
    -> GpgError {
  RunGpgOperaAsync(ctx_.GetChannel(),
                   [ctx_ptr = &ctx_, keys, in_path, ascii,
                    out_path](const DataObjectPtr& data_object) -> GpgError {
                     return EncryptFileRpgpImpl(*ctx_ptr, keys, in_path, ascii,
                                                out_path, data_object);
                   },
                   cb, "rpgp_op_encrypt", {{OpenPGPEngine::kRPGP, "0.1.0"}});
  return GPG_ERR_NO_ERROR;  // The actual result will be delivered via callback
}

namespace {
auto DecryptFileGeneralRpgpImpl(OpenPGPContext& ctx_, bool is_archive,
                                const QString& in_path, const QString& out_path,
                                const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();

  Rust::GfrDecryptResultC decrypt_result;
  memset(&decrypt_result, 0, sizeof(decrypt_result));
  Rust::GfrStatus err;

  if (is_archive) {
    err = Rust::gfr_crypto_decrypt_archive(
        ctx_.GetChannel(), in_file_path_utf8.constData(),
        out_file_path_utf8.constData(), false, FetchSecretKeyCallback,
        FetchPasswordCallback, FreeCallback, key_db.data(), &decrypt_result);
  } else {
    err = Rust::gfr_crypto_decrypt_file(
        ctx_.GetChannel(), in_file_path_utf8.constData(),
        out_file_path_utf8.constData(), false, FetchSecretKeyCallback,
        FetchPasswordCallback, FreeCallback, key_db.data(), &decrypt_result);
  }

  auto [gf_err, result] =
      HandleDecryptResult(*key_db, GFBuffer{}, err, decrypt_result);
  Rust::gfr_crypto_free_decrypt_result(&decrypt_result);

  data_object->Swap({GpgDecryptResult(result)});
  return gf_err;
}
}  // namespace

auto DecryptFileRpgpImpl(OpenPGPContext& ctx_, const QString& in_path,
                         const QString& out_path,
                         const DataObjectPtr& data_object) -> GpgError {
  return DecryptFileGeneralRpgpImpl(ctx_, false, in_path, out_path,
                                    data_object);
}

auto DecryptArchiveRpgpImpl(OpenPGPContext& ctx_, const QString& in_path,
                            const QString& out_path,
                            const GpgOperationCallback& cb) -> GpgError {
  RunGpgOperaAsync(ctx_.GetChannel(),
                   [ctx_ptr = &ctx_, in_path,
                    out_path](const DataObjectPtr& data_object) -> GpgError {
                     return DecryptFileGeneralRpgpImpl(*ctx_ptr, true, in_path,
                                                       out_path, data_object);
                   },
                   cb, "rpgp_op_decrypt_archive",
                   {{OpenPGPEngine::kRPGP, "0.1.0"}});
  return GPG_ERR_NO_ERROR;  // The actual result will be delivered via callback
}

auto SignFileRpgpImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& keys,
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
  memset(&sign_result, 0, sizeof(sign_result));

  auto status = Rust::gfr_crypto_sign_file(
      ctx_.GetChannel(), in_file_path_utf8.constData(),
      out_file_path_utf8.constData(), key_block_cstrs.data(),
      key_block_cstrs.size(), FetchPasswordCallback, FreeCallback,
      Rust::GfrSignMode::Detached, ascii, &sign_result);

  auto [gf_err, result] = HandleSignResult({}, status, sign_result);
  Rust::gfr_crypto_free_sign_result(&sign_result);

  data_object->Swap({GpgSignResult(result)});
  return gf_err;
}

auto VerifyFileRpgpImpl(OpenPGPContext& ctx_, const QString& data_path,
                        const QString& sign_path,
                        const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  auto data_file_path_utf8 = data_path.toUtf8();
  auto sign_file_path_utf8 = sign_path.toUtf8();

  Rust::GfrVerifyResultC verify_result;
  memset(&verify_result, 0, sizeof(verify_result));

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

  auto [gf_err, result] = HandleVerifyResult(GFBuffer{}, err, verify_result);
  Rust::gfr_crypto_free_verify_result(&verify_result);

  LOG_D() << "Verification result: "
          << (result.is_verified ? "VALID" : "INVALID")
          << ", Signatures found: " << result.signatures.size();

  data_object->Swap({GpgVerifyResult{result}});
  return gf_err;
}

auto EncryptSignFileRpgpImpl(OpenPGPContext& ctx,
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

  auto recipients = Convert2GpgKeyList(ctx.GetChannel(), enc_keys);

  QContainer<QByteArray> key_blocks_utf8 =
      GetPublicKeysByKeyIdsForEncryption(*key_db, recipients);
  if (key_blocks_utf8.empty()) {
    LOG_E() << "No valid recipients found for encryption.";
    return GPG_ERR_GENERAL;  // Or appropriate error code
  }

  std::vector<const char*> recipient_cstrs;
  recipient_cstrs.reserve(key_blocks_utf8.size());
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

  Rust::GfrStatus err;
  QString name;
  Rust::GfrEncryptAndSignResultC encrypt_sign_result;

  bool is_directory = QFileInfo(in_path).isDir();
  if (is_directory) {
    err = Rust::gfr_crypto_encrypt_and_sign_directory(
        ctx.GetChannel(), in_path.toUtf8().constData(),
        out_path.toUtf8().constData(), recipient_cstrs.data(),
        recipient_cstrs.size(), c_skeys.data(), c_skeys.size(),
        FetchPasswordCallback, FreeCallback, ascii, &encrypt_sign_result);
  } else {
    err = Rust::gfr_crypto_encrypt_and_sign_file(
        ctx.GetChannel(), in_path.toUtf8().constData(),
        out_path.toUtf8().constData(), recipient_cstrs.data(),
        recipient_cstrs.size(), c_skeys.data(), c_skeys.size(),
        FetchPasswordCallback, FreeCallback, ascii, &encrypt_sign_result);
  }

  auto [gf_err, encrypt_result] =
      HandleEncryptResult({}, err,
                          Rust::GfrEncryptResultC{
                              .data = encrypt_sign_result.data,
                              .data_len = encrypt_sign_result.data_len,
                              .meta = encrypt_sign_result.encrypt_meta,
                          });

  auto [gf_err_2, sign_result] = HandleSignResult(
      {}, err,
      Rust::GfrSignResultC{.data = encrypt_sign_result.data,
                           .data_len = encrypt_sign_result.data_len,
                           .meta = encrypt_sign_result.sign_meta});
  Rust::gfr_crypto_free_encrypt_and_sign_result(&encrypt_sign_result);

  data_object->Swap({
      GpgEncryptResult(encrypt_result),
      GpgSignResult(sign_result),
  });

  return (gf_err != GPG_ERR_NO_ERROR) ? gf_err : gf_err_2;
}

auto EncryptSignDirRpgpImpl(OpenPGPContext& ctx,
                            const GpgAbstractKeyPtrList& enc_keys,
                            const GpgAbstractKeyPtrList& sign_keys,
                            const QString& in_path, bool ascii,
                            const QString& out_path,
                            const GpgOperationCallback& cb) -> GpgError {
  RunGpgOperaAsync(
      ctx.GetChannel(),
      [ctx_ptr = &ctx, enc_keys, sign_keys, in_path, ascii,
       out_path](const DataObjectPtr& data_object) -> GpgError {
        return EncryptSignFileRpgpImpl(*ctx_ptr, enc_keys, sign_keys, in_path,
                                       ascii, out_path, data_object);
      },
      cb, "rpgp_op_encrypt_sign_dir", {{OpenPGPEngine::kRPGP, "0.1.0"}});
  return GPG_ERR_NO_ERROR;  // The actual result will be delivered via callback
}

namespace {
auto DecryptVerifyFileGeneralRpgpImpl(OpenPGPContext& ctx, bool is_archive,
                                      const QString& in_path,
                                      const QString& out_path,
                                      const DataObjectPtr& data_object)
    -> GpgError {
  auto key_db = ctx.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();

  Rust::GfrStatus err;
  Rust::GfrDecryptAndVerifyResultC decrypt_verify_result;
  memset(&decrypt_verify_result, 0, sizeof(decrypt_verify_result));

  if (is_archive) {
    err = Rust::gfr_crypto_decrypt_and_verify_archive(
        ctx.GetChannel(), in_file_path_utf8.constData(),
        out_file_path_utf8.constData(), false, FetchSecretKeyCallback,
        FetchPasswordCallback, FetchPublicKeyCallback, FreeCallback,
        key_db.data(), &decrypt_verify_result);
  } else {
    err = Rust::gfr_crypto_decrypt_and_verify_file(
        ctx.GetChannel(), in_file_path_utf8.constData(),
        out_file_path_utf8.constData(), false, FetchSecretKeyCallback,
        FetchPasswordCallback, FetchPublicKeyCallback, FreeCallback,
        key_db.data(), &decrypt_verify_result);
  }

  auto [gf_err, decrypt_result] =
      HandleDecryptResult(*key_db, GFBuffer{}, err,
                          Rust::GfrDecryptResultC{
                              .data = decrypt_verify_result.data,
                              .data_len = decrypt_verify_result.data_len,
                              .meta = decrypt_verify_result.decrypt_meta,
                          });

  auto [gf_err_2, verify_result] = HandleVerifyResult(
      GFBuffer{}, err,
      Rust::GfrVerifyResultC{.meta = decrypt_verify_result.verify_meta});

  Rust::gfr_crypto_free_decrypt_and_verify_result(&decrypt_verify_result);

  data_object->Swap(
      {GpgDecryptResult(decrypt_result), GpgVerifyResult(verify_result)});

  return (gf_err != GPG_ERR_NO_ERROR) ? gf_err : gf_err_2;
}
}  // namespace

auto DecryptVerifyFileRpgpImpl(OpenPGPContext& ctx, const QString& in_path,
                               const QString& out_path,
                               const DataObjectPtr& data_object) -> GpgError {
  return DecryptVerifyFileGeneralRpgpImpl(ctx, false, in_path, out_path,
                                          data_object);
}

auto DecryptVerifyArchiveRpgpImpl(OpenPGPContext& ctx, const QString& in_path,
                                  const QString& out_path,
                                  const GpgOperationCallback& cb) -> GpgError {
  RunGpgOperaAsync(ctx.GetChannel(),
                   [ctx_ptr = &ctx, in_path,
                    out_path](const DataObjectPtr& data_object) -> GpgError {
                     return DecryptVerifyFileGeneralRpgpImpl(
                         *ctx_ptr, true, in_path, out_path, data_object);
                   },
                   cb, "rpgp_op_decrypt_archive",
                   {{OpenPGPEngine::kRPGP, "0.1.0"}});
  return GPG_ERR_NO_ERROR;  // The actual result will be delivered via callback
}

auto EncryptSymmetricFileRpgpImpl(OpenPGPContext& ctx_, const QString& in_path,
                                  bool ascii, const QString& out_path,
                                  const DataObjectPtr& data_object)
    -> GpgError {
  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();

  Rust::GfrStatus status;
  Rust::GfrEncryptResultC encrypt_result;
  memset(&encrypt_result, 0, sizeof(encrypt_result));

  auto is_directory = QFileInfo(in_path).isDir();
  if (is_directory) {
    // For directory encryption, we need to create a tar archive in memory
    // first and then pass it to the Rust FFI. The Rust FFI will handle the
    // encryption of the tar archive.
    status = Rust::gfr_crypto_encrypt_directory_symmetric(
        ctx_.GetChannel(), in_file_path_utf8.constData(),
        out_file_path_utf8.constData(), ascii, FetchPasswordCallback,
        FreeCallback, &encrypt_result);
  } else {
    // Call Rust FFI. Ensure in_buffer is a null-terminated C-string if Rust
    // expects it.
    status = Rust::gfr_crypto_encrypt_file_symmetric(
        ctx_.GetChannel(), in_file_path_utf8.constData(),
        out_file_path_utf8.constData(), ascii, FetchPasswordCallback,
        FreeCallback, &encrypt_result);
  }

  auto [gf_err, result] = HandleEncryptResult({}, status, encrypt_result);
  Rust::gfr_crypto_free_encrypt_result(&encrypt_result);

  data_object->Swap({GpgEncryptResult(result)});
  return gf_err;
}

auto EncryptSymmetricDirRpgpImpl(OpenPGPContext& ctx, const QString& in_path,
                                 bool ascii, const QString& out_path,
                                 const GpgOperationCallback& cb) -> GpgError {
  RunGpgOperaAsync(ctx.GetChannel(),
                   [ctx_ptr = &ctx, in_path, ascii,
                    out_path](const DataObjectPtr& data_object) -> GpgError {
                     return EncryptSymmetricFileRpgpImpl(
                         *ctx_ptr, in_path, ascii, out_path, data_object);
                   },
                   cb, "rpgp_op_encrypt_symmetric_dir",
                   {{OpenPGPEngine::kRPGP, "0.1.0"}});

  return GPG_ERR_NO_ERROR;  // The actual result will be delivered via callback
}

}  // namespace GpgFrontend