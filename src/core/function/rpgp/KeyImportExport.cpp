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

#include "core/GpgCoreRust.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {

auto MergeGFKeyRpgpImpl(const QContainer<GFKey>& gf_keys, bool secret)
    -> std::optional<GFKey> {
  if (gf_keys.empty()) {
    LOG_E() << "no gf key provided for merging";
    return std::nullopt;
  }

  QContainer<QByteArray> armored_key_blocks;
  for (const auto& gf_key : gf_keys) {
    if (gf_key.metadata.fpr.isEmpty()) {
      LOG_E() << "skipping key with empty fingerprint";
      continue;
    }

    if (secret) {
      if (!gf_key.metadata.has_secret) {
        LOG_E() << "skipping key without secret key block: "
                << gf_key.metadata.fpr;
        continue;
      }

      if (gf_key.blocks.secret_key.isEmpty()) {
        LOG_E() << "skipping key with empty secret key block: "
                << gf_key.metadata.fpr;
        continue;
      }

      armored_key_blocks.push_back(gf_key.blocks.secret_key.toUtf8());
      continue;
    }

    if (gf_key.blocks.public_key.isEmpty()) {
      LOG_E() << "skipping key with empty public key block: "
              << gf_key.metadata.fpr;
      continue;
    }
    armored_key_blocks.push_back(gf_key.blocks.public_key.toUtf8());
  }

  if (armored_key_blocks.empty()) {
    LOG_E() << "no valid key blocks found for merging";
    return std::nullopt;
  }

  if (armored_key_blocks.size() == 1) {
    auto pd_gf_keys = GetGFKeysFromKeyBlock(GFBuffer(armored_key_blocks[0]));
    if (pd_gf_keys.empty()) {
      LOG_E() << "failed to extract any keys from the provided key block";
      return std::nullopt;
    }

    if (pd_gf_keys.size() > 1) {
      LOG_W() << "multiple keys extracted from single key block, returning the "
                 "first one. count: "
              << pd_gf_keys.size();
    }

    return pd_gf_keys.front();
  }

  std::vector<char*> key_block_ptrs;
  for (const auto& block : armored_key_blocks) {
    key_block_ptrs.push_back(const_cast<char*>(block.data()));
  }

  char* out_pub = nullptr;
  char* out_sec = nullptr;

  for (size_t idx = 1; idx < static_cast<size_t>(armored_key_blocks.size());
       ++idx) {
    auto* base_block_ptrs = key_block_ptrs[idx - 1];
    auto* incoming_block_ptr = key_block_ptrs[idx];
    auto err = Rust::gfr_crypto_merge_key_blocks(
        base_block_ptrs, incoming_block_ptr, &out_sec, &out_pub);

    if (err != Rust::GfrStatus::Success) {
      LOG_E() << "gfr_export_merged_keys error, code: "
              << static_cast<int>(err);
      return std::nullopt;
    }

    auto qs_armored = QString::fromUtf8(out_pub);
    Rust::gfr_crypto_free_string(out_pub);

    auto qs_sec_armored = QString::fromUtf8(out_sec);
    Rust::gfr_crypto_free_string(out_sec);

    if (secret && !qs_sec_armored.isEmpty()) {
      armored_key_blocks[idx] = qs_sec_armored.toUtf8();
    } else if (!secret && !qs_armored.isEmpty()) {
      armored_key_blocks[idx] = qs_armored.toUtf8();
    } else {
      LOG_E() << "merged armored string is empty after merging block index "
              << idx;
      return std::nullopt;
    }

    LOG_D() << "merged armored string: " << qs_armored;
  }

  auto final_gf_keys =
      GetGFKeysFromKeyBlock(GFBuffer(armored_key_blocks.back()));
  if (final_gf_keys.empty()) {
    LOG_E() << "failed to extract any keys from the merged key block";
    return std::nullopt;
  }

  if (final_gf_keys.size() > 1) {
    LOG_W() << "multiple keys extracted from merged key block, returning the "
               "first one. count: "
            << final_gf_keys.size();
  }

  return final_gf_keys.front();
}

auto ImportKeyRpgpImpl(GpgContext& ctx, const GFBuffer& in_buffer)
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
      final_gf_key = MergeGFKeyRpgpImpl({*db_gf_key, pd_gf_key},
                                        pd_gf_key.metadata.has_secret);
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

auto ExportKeysRpgpImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                        bool secret) -> std::tuple<GpgError, GFBuffer> {
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
    key_block_ptrs.push_back(const_cast<char*>(block.data()));
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

  LOG_D() << "exported armored string: " << qs_armored;

  return {GPG_ERR_NO_ERROR, GFBuffer(qs_armored)};
}

}  // namespace GpgFrontend