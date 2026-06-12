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

#include "core/GFCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/openpgp/KeyGroupRepository.h"
#include "core/function/rpgp/KeyImportExport.h"
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

  for (const auto& key : keys) {
    LOG_D() << "Deleting key: " << key->ID();

    if (key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP) {
      KeyGroupRepository::GetInstance(ctx.GetChannel()).Remove(key->ID());
      continue;
    }

    if (!key_db->GetKeyMetadata(key->ID())) {
      LOG_E() << "key not found in database: " << key->ID();
      return false;
    }

    if (!key_db->DeleteKey(key->ID())) {
      LOG_E() << "failed to delete key from database: " << key->ID();
    }
  }
  return true;
}

auto ModifyKeyPassphraseRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                                 const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return GPG_ERR_GENERAL;
  }

  auto meta = key_db->GetKeyMetadata(key->Fingerprint());
  if (!meta) {
    LOG_E() << "key metadata not found in database for key: "
            << key->Fingerprint();
    return GPG_ERR_GENERAL;
  }

  auto key_block_data = key_db->GetKeyBlocks(meta->fpr);
  if (!key_block_data) {
    LOG_E() << "key block data not found in database for key with fpr: "
            << meta->fpr;
    return GPG_ERR_GENERAL;
  }

  if (key_block_data->secret_key.Empty()) {
    LOG_E() << "secret key block is empty for key with fpr: " << meta->fpr;
    return GPG_ERR_GENERAL;
  }

  auto key_block_utf8 = key_block_data->secret_key;
  if (key_block_utf8 == nullptr) {
    LOG_E() << "key block data is null for key with fpr: " << meta->fpr;
    return GPG_ERR_GENERAL;
  }

  auto key_fpr_utf8 = key->Fingerprint().toUtf8();
  char* out_secret_block = nullptr;

  auto err = Rust::gfr_crypto_modify_key_password(
      ctx.GetChannel(), key_block_utf8.Data(), key_fpr_utf8.constData(),
      FetchPasswordCallback, &out_secret_block);

  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_modify_key_password error, code: "
            << static_cast<int>(err);
    return GPG_ERR_GENERAL;
  }

  if (out_secret_block == nullptr) {
    LOG_E() << "gfr_crypto_modify_key_password returned null secret block";
    return GPG_ERR_GENERAL;
  }

  auto out_block_str = QString::fromUtf8(out_secret_block);
  Rust::gfr_crypto_free_string(out_secret_block);

  auto info = ImportKeyRpgpImpl(ctx, GFBuffer(out_block_str));
  if (info == nullptr || info->imported_keys.empty()) {
    LOG_E() << "Failed to import updated key block after modifying key "
               "password for key with fpr: "
            << key->Fingerprint();
    return GPG_ERR_GENERAL;
  }

  return GPG_ERR_NO_ERROR;
}

auto DeleteSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                          int skey_idx) -> bool {
  auto key_db = ctx.KeyDatabase();
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

  if (skey_idx < 0 || skey_idx >= static_cast<int>(meta->subkeys.size())) {
    LOG_E() << "illegal subkey index: " << skey_idx
            << " for key with fpr: " << meta->fpr;
    return false;
  }

  auto target_subkey_fpr = meta->subkeys[skey_idx].fpr;

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
  if (key_block_utf8.Empty()) {
    LOG_E() << "key block data is empty for key with fpr: " << meta->fpr;
    return false;
  }

  auto key_fpr_utf8 = key->Fingerprint().toUtf8();
  char* out_secret_block = nullptr;

  auto err = Rust::gfr_crypto_delete_subkey(
      key_block_utf8.Data(), target_subkey_fpr.toUtf8().constData(),
      &out_secret_block);

  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_delete_subkey error, code: "
            << static_cast<int>(err);
    return false;
  }

  if (out_secret_block == nullptr) {
    LOG_E() << "gfr_crypto_delete_subkey returned null secret block";
    return false;
  }

  auto out_block_str = QString::fromUtf8(out_secret_block);
  Rust::gfr_crypto_free_string(out_secret_block);

  auto info = ImportKeyRpgpImpl(ctx, GFBuffer(out_block_str));
  if (info == nullptr || info->imported_keys.empty()) {
    LOG_E() << "Failed to import updated key block after deleting subkey for "
               "key with fpr: "
            << key->Fingerprint();
    return false;
  }

  return true;
}

auto RevokeSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                          int subkey_index, int reason_code,
                          const QString& reason_text) -> bool {
  auto key_db = ctx.KeyDatabase();
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

  if (subkey_index < 0 ||
      subkey_index >= static_cast<int>(meta->subkeys.size())) {
    LOG_E() << "illegal subkey index: " << subkey_index
            << " for key with fpr: " << meta->fpr;
    return false;
  }

  auto target_subkey_fpr = meta->subkeys[subkey_index].fpr;

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
  if (key_block_utf8.Empty()) {
    LOG_E() << "key block data is empty for key with fpr: " << meta->fpr;
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

  auto err = Rust::gfr_crypto_revoke_subkey(
      ctx.GetChannel(), key_block_utf8.Data(),
      target_subkey_fpr.toUtf8().constData(),
      static_cast<Rust::GfrRevocationCode>(mapped_reason_code),
      reason_text_utf8.constData(), FetchPasswordCallback, &out_secret_block);

  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_revoke_subkey error, code: "
            << static_cast<int>(err);
    return false;
  }

  if (out_secret_block == nullptr) {
    LOG_E() << "gfr_crypto_revoke_subkey returned null secret block";
    return false;
  }

  auto out_block_str = QString::fromUtf8(out_secret_block);
  Rust::gfr_crypto_free_string(out_secret_block);

  auto info = ImportKeyRpgpImpl(ctx, GFBuffer(out_block_str));
  if (info == nullptr || info->imported_keys.empty()) {
    LOG_E() << "Failed to import updated key block after revoking subkey for "
               "key with fpr: "
            << key->Fingerprint();
    return false;
  }

  return true;
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

  auto err = Rust::gfr_crypto_generate_key_rev_cert(
      ctx_.GetChannel(), key_block_utf8.Data(),
      static_cast<Rust::GfrRevocationCode>(mapped_reason_code),
      reason_text_utf8.constData(), FetchPasswordCallback, &out_secret_block);

  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_generate_key_rev_cert error, code: "
            << static_cast<int>(err);
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
}  // namespace GpgFrontend