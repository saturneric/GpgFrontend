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

#include "KeyImportExport.h"

#include "core/GFCoreRust.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {

namespace {
auto MergeGFKeyRpgpImpl(const QContainer<GFKey>& gf_keys, bool secret)
    -> std::optional<GFKey> {
  if (gf_keys.empty()) {
    LOG_E() << "no gf key provided for merging";
    return std::nullopt;
  }

  QContainer<GFBuffer> blocks_to_merge;
  bool has_any_secret = false;

  for (const auto& gf_key : gf_keys) {
    if (gf_key.metadata.fpr.isEmpty()) continue;

    GFBuffer block;
    if (!gf_key.blocks.secret_key.Empty()) {
      block = gf_key.blocks.secret_key;
      has_any_secret = true;
    } else if (!gf_key.blocks.public_key.Empty()) {
      block = gf_key.blocks.public_key;
    }

    if (!block.Empty()) {
      blocks_to_merge.push_back(block);
    }
  }

  if (secret && !has_any_secret) {
    LOG_E() << "secret merge requested but no secret blocks provided";
    return std::nullopt;
  }

  if (blocks_to_merge.empty()) {
    return std::nullopt;
  }

  GFBuffer current_merged_block = blocks_to_merge.front();

  for (size_t i = 1; i < static_cast<size_t>(blocks_to_merge.size()); ++i) {
    char* out_sec = nullptr;
    char* out_pub = nullptr;

    auto err = Rust::gfr_crypto_merge_key_blocks(
        current_merged_block.Data(),  // base
        blocks_to_merge[i].Data(),    // incoming
        &out_sec, &out_pub);

    if (err != Rust::GfrStatus::Success) {
      LOG_E() << "Merge failed at index " << i
              << " code: " << static_cast<int>(err);
      if (out_sec != nullptr) Rust::gfr_crypto_free_string(out_sec);
      if (out_pub != nullptr) Rust::gfr_crypto_free_string(out_pub);
      return std::nullopt;
    }

    GFBuffer res_armored;
    if (secret && (out_sec != nullptr) && strlen(out_sec) > 0) {
      res_armored = GFBuffer(out_sec);
    } else if ((out_pub != nullptr) && strlen(out_pub) > 0) {
      res_armored = GFBuffer(out_pub);
    }

    if (out_sec != nullptr) Rust::gfr_crypto_free_string(out_sec);
    if (out_pub != nullptr) Rust::gfr_crypto_free_string(out_pub);

    if (res_armored.Empty()) {
      LOG_E() << "Merge resulted in empty string at index " << i;
      return std::nullopt;
    }

    current_merged_block = res_armored;
  }

  auto final_gf_keys = GetGFKeysFromKeyBlock(GFBuffer(current_merged_block));
  if (final_gf_keys.empty()) {
    LOG_E() << "failed to parse final merged key block";
    return std::nullopt;
  }

  return final_gf_keys.front();
}
}  // namespace

auto ImportKeyRpgpImpl(OpenPGPContext& ctx, const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  if (in_buffer.Empty()) return {};

  auto key_db = ctx.KeyDatabase();

  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return {};
  }

  auto info = QSharedPointer<GpgImportInformation>::create();
  auto pd_gf_keys = GetGFKeysFromKeyBlock(in_buffer);
  if (pd_gf_keys.empty()) {
    LOG_E() << "failed to extract any keys from the provided key block";
    return {};
  }

  for (const auto& pd_gf_key : pd_gf_keys) {
    std::optional<GFKey> final_gf_key;
    auto db_gf_key = key_db->GetKeyByIdentifier(pd_gf_key.metadata.fpr);
    // should do merge if the key already exists
    if (db_gf_key) {
      LOG_I() << "key with fpr: " << pd_gf_key.metadata.fpr
              << " already exists in database, merging with the imported key";
      final_gf_key = MergeGFKeyRpgpImpl(
          {pd_gf_key, *db_gf_key},
          pd_gf_key.metadata.has_secret || db_gf_key->metadata.has_secret);
    } else {
      final_gf_key = pd_gf_key;
    }

    if (!final_gf_key) {
      LOG_E() << "failed to merge key with fpr: " << pd_gf_key.metadata.fpr;
      continue;
    }

    if (CreateOrUpdateGFKeyInDatabase(ctx.GetChannel(), *final_gf_key)) {
      info->imported_keys.push_back(
          {final_gf_key->metadata.fpr.toUpper(), GPG_ERR_NO_ERROR});
      info->imported += 1;
      info->considered += 1;
    } else {
      LOG_E() << "failed to import key with fpr: "
              << final_gf_key->metadata.fpr;
    }
  }

  return info;
}

auto ExportKeysRpgpImpl(OpenPGPContext& ctx, const GpgAbstractKeyPtrList& keys,
                        bool secret, bool ascii, bool shortest, bool ssh_mode)
    -> std::tuple<GpgError, GFBuffer> {
  auto key_db = ctx.KeyDatabase();

  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return {GPG_ERR_GENERAL, {}};
  }

  QStringList key_ids;
  for (const auto& key : keys) {
    if (!key) continue;
    key_ids.push_back(key->Fingerprint());
  }

  auto key_blocks = GetArmoredKeyBlocksForKeys(*key_db, key_ids, secret);
  if (key_blocks.empty()) {
    LOG_E() << "no valid key blocks found for export";
    return {GPG_ERR_NO_DATA, {}};
  }

  char* out_armored_string = nullptr;

  std::vector<char*> key_block_ptrs;
  for (const auto& block : key_blocks) {
    key_block_ptrs.push_back(const_cast<char*>(block.Data()));
  }

  auto err =
      Rust::gfr_export_merged_keys(key_block_ptrs.data(), key_block_ptrs.size(),
                                   secret, &out_armored_string);
  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_export_merged_keys error, code: " << static_cast<int>(err);
    return {GPG_ERR_GENERAL, {}};
  }

  auto qs_armored = QString::fromUtf8(out_armored_string);
  Rust::gfr_crypto_free_string(out_armored_string);

  return {GPG_ERR_NO_ERROR, GFBuffer(qs_armored)};
}

auto ExportKeysAsyncRpgpImpl(OpenPGPContext& ctx,
                             const GpgAbstractKeyPtrList& keys, bool secret,
                             bool ascii, bool shortest, bool ssh_mode,
                             const DataObjectPtr& data_object) -> GpgError {
  auto [err, buffer] =
      ExportKeysRpgpImpl(ctx, keys, secret, ascii, shortest, ssh_mode);
  data_object->Swap({buffer});
  return err;
}

auto ExportAllKeysRpgpImpl(OpenPGPContext& ctx,
                           const GpgAbstractKeyPtrList& keys, bool secret,
                           bool ascii, const DataObjectPtr& data_object)
    -> GpgError {
  if (keys.empty()) return GPG_ERR_CANCELED;

  auto [err, buffer] =
      ExportKeysRpgpImpl(ctx, keys, false, ascii, false, false);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return err;

  if (secret) {
    auto [sec_err, sec_buffer] =
        ExportKeysRpgpImpl(ctx, keys, true, ascii, false, false);
    if (gpgme_err_code(sec_err) != GPG_ERR_NO_ERROR) return sec_err;

    buffer.Append(sec_buffer);
  }

  data_object->Swap({buffer});
  return err;
}

auto ImportRevCertRpgpImpl(OpenPGPContext& ctx, const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  if (in_buffer.Empty()) return {};

  LOG_D() << "importing revocation certificate with buffer size: "
          << in_buffer.Size();

  auto in_buffer_utf8 = in_buffer.ConvertToQString().toUtf8();
  LOG_D() << "importing revocation certificate with UTF-8 size: "
          << in_buffer_utf8.size();

  char* out_fpr = nullptr;
  auto status = Rust::gfr_crypto_extract_rev_cert_target_fpr(
      in_buffer_utf8.data(), &out_fpr);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_extract_rev_cert_target_fpr failed with status: "
            << static_cast<int>(status);
    return {};
  }

  auto qs_fpr = QString::fromUtf8(out_fpr).toUpper();
  Rust::gfr_crypto_free_string(out_fpr);

  if (qs_fpr.isEmpty()) {
    LOG_E() << "extracted empty fingerprint from revocation certificate";
    return {};
  }

  LOG_D() << "extracted fingerprint from revocation certificate: " << qs_fpr;

  auto key_db = ctx.KeyDatabase();
  if (!key_db) {
    LOG_E() << "key database is not initialized";
    return {};
  }

  auto db_gf_key = key_db->GetKeyByIdentifier(qs_fpr);
  if (!db_gf_key) {
    LOG_E()
        << "no key found in database matching the fingerprint extracted from "
        << "revocation certificate. fpr: " << qs_fpr;
    return {};
  }

  GFBuffer key_block_utf8;
  if (db_gf_key->metadata.has_secret) {
    if (db_gf_key->blocks.secret_key.Empty()) {
      LOG_E()
          << "key with matching fingerprint does not have a secret key block";
      return {};
    }
    key_block_utf8 = db_gf_key->blocks.secret_key;
  } else {
    if (db_gf_key->blocks.public_key.Empty()) {
      LOG_E()
          << "key with matching fingerprint does not have a public key block";
      return {};
    }
    key_block_utf8 = db_gf_key->blocks.public_key;
  }

  char* out_sec = nullptr;
  char* out_pub = nullptr;
  status = Rust::gfr_crypto_import_rev_cert(
      key_block_utf8.Data(), in_buffer_utf8.data(), &out_sec, &out_pub);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_import_rev_cert failed with status: "
            << static_cast<int>(status);
    return {};
  }

  auto qs_sec_armored = QString::fromUtf8(out_sec);
  Rust::gfr_crypto_free_string(out_sec);
  auto qs_pub_armored = QString::fromUtf8(out_pub);
  Rust::gfr_crypto_free_string(out_pub);

  if (qs_sec_armored.isEmpty() && qs_pub_armored.isEmpty()) {
    LOG_E() << "both imported public and secret key blocks are empty after "
               "importing revocation certificate";
    return {};
  }

  if (db_gf_key->metadata.has_secret && qs_sec_armored.isEmpty()) {
    LOG_E() << "imported revocation certificate did not return a secret key "
               "block for a key that has secret key in database";
    return {};
  }

  LOG_D() << "imported revocation certificate, got secret block size: "
          << qs_sec_armored.size()
          << ", public block size: " << qs_pub_armored.size();

  return ImportKeyRpgpImpl(ctx, qs_sec_armored.isEmpty()
                                    ? GFBuffer(qs_pub_armored)
                                    : GFBuffer(qs_sec_armored));
}

}  // namespace GpgFrontend