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

auto ImportKeyRpgpImpl(GpgContext& ctx, const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  if (in_buffer.Empty()) return {};

  auto key_db = ctx.KeyDatabase();

  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return {};
  }

  auto info = QSharedPointer<GpgImportInformation>::create();
  auto gf_keys = GetGFKeysFromKeyBlock(in_buffer);
  if (gf_keys.empty()) {
    LOG_E() << "failed to extract any keys from the provided key block";
    return {};
  }

  for (const auto& gf_key : gf_keys) {
    if (CreateOrUpdateGFKeyInDatabase(ctx.GetChannel(), gf_key)) {
      info->imported_keys.push_back(
          {gf_key.metadata.fpr.toUpper(), GPG_ERR_NO_ERROR});
      info->imported += 1;
      info->considered += 1;
    } else {
      LOG_E() << "failed to import key with fpr: " << gf_key.metadata.fpr;
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