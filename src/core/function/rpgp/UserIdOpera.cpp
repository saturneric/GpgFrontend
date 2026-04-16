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

#include "UserIdOpera.h"

#include "core/GpgCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/rpgp/KeyImportExport.h"
#include "core/function/rpgp/RustEngineCallback.h"

namespace GpgFrontend {

auto AddUIDRpgpImpl(GpgContext& ctx, const GpgKeyPtr& key, const QString& uid)
    -> bool {
  LOG_D() << "Adding UID: " << uid << " to key: " << key->Fingerprint()
          << " using RPGP backend";

  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "Key database is not initialized";
    return false;
  }

  auto gf_key = key_db->GetKeyByIdentifier(key->Fingerprint());
  if (!gf_key) {
    LOG_E() << "Key not found in database: " << key->Fingerprint();
    return false;
  }

  if (!gf_key->metadata.has_secret) {
    LOG_E() << "Cannot add UID to a public key";
    return false;
  }

  auto secret_key_block = gf_key->blocks.secret_key.toUtf8();
  if (secret_key_block.isEmpty()) {
    LOG_E() << "Secret key block is empty for key: " << key->Fingerprint();
    return false;
  }

  auto uid_utf8 = uid.toUtf8();

  char* out_block = nullptr;
  auto status = Rust::gfr_crypto_add_user_id(
      ctx.GetChannel(), secret_key_block.constData(), uid_utf8.constData(),
      FetchPasswordCallback, FreeCallback, &out_block);

  LOG_D() << "Rust function gfr_crypto_add_user_id returned status: "
          << static_cast<int>(status);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Failed to add UID: " << uid << " to key: " << key->Fingerprint()
            << ", status: " << static_cast<int>(status);
    return false;
  }

  if (out_block == nullptr) {
    LOG_E() << "Output block is null after adding UID: " << uid
            << " to key: " << key->Fingerprint();
    return false;
  }

  auto out_block_str = QString::fromUtf8(out_block);
  Rust::gfr_crypto_free_string(out_block);

  LOG_D() << "Successfully added UID: " << uid << " to key: " << key;

  auto info = ImportKeyRpgpImpl(ctx, GFBuffer(out_block_str));
  if (info == nullptr || info->imported_keys.empty()) {
    LOG_E() << "Failed to import updated key block after adding UID: " << uid
            << " to key: " << key->Fingerprint();
    return false;
  }

  return true;
}

auto DeleteUIDRpgpImpl(GpgContext& ctx, const GpgKeyPtr& key, int uid_index)
    -> bool {
  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "Key database is not initialized";
    return false;
  }

  auto gf_key = key_db->GetKeyByIdentifier(key->Fingerprint());
  if (!gf_key) {
    LOG_E() << "Key not found in database: " << key->Fingerprint();
    return false;
  }

  if (!gf_key->metadata.has_secret) {
    LOG_E() << "Cannot delete UID from a public key";
    return false;
  }

  auto uids = gf_key->metadata.user_ids;
  if (uid_index <= 0 || uid_index > static_cast<int>(uids.size())) {
    LOG_E() << "Invalid UID index: " << uid_index;
    return false;
  }

  auto uid = uids[uid_index - 1].ToString();
  auto uid_utf8 = uid.toUtf8();

  auto secret_key_block = gf_key->blocks.secret_key.toUtf8();
  if (secret_key_block.isEmpty()) {
    LOG_E() << "Secret key block is empty for key: " << key->Fingerprint();
    return false;
  }

  char* out_block = nullptr;
  auto status = Rust::gfr_crypto_delete_user_id(
      secret_key_block.constData(), uid_utf8.constData(), &out_block);

  LOG_D() << "Rust function gfr_crypto_delete_user_id returned status: "
          << static_cast<int>(status);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Failed to delete UID: " << uid
            << " from key: " << key->Fingerprint()
            << ", status: " << static_cast<int>(status);
    return false;
  }

  if (out_block == nullptr) {
    LOG_E() << "Output block is null after deleting UID: " << uid
            << " from key: " << key->Fingerprint();
    return false;
  }

  auto out_block_str = QString::fromUtf8(out_block);
  Rust::gfr_crypto_free_string(out_block);

  LOG_D() << "Successfully deleted UID: " << uid << " from key: " << key;

  auto info = ImportKeyRpgpImpl(ctx, GFBuffer(out_block_str));
  if (info == nullptr || info->imported_keys.empty()) {
    LOG_E() << "Failed to import updated key block after deleting UID: " << uid
            << " from key: " << key->Fingerprint();
    return false;
  }

  return true;
}

}  // namespace GpgFrontend
