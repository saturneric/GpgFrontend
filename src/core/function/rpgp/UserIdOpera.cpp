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

#include "core/GFCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/rpgp/KeyImportExport.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

auto AddUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                    const QString& uid) -> bool {
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

  auto secret_key_block = gf_key->blocks.secret_key;
  if (secret_key_block.Empty()) {
    LOG_E() << "Secret key block is empty for key: " << key->Fingerprint();
    return false;
  }

  auto uid_utf8 = uid.toUtf8();

  char* out_block = nullptr;

  Rust::GfrBuffer key_block_buffer = {
      reinterpret_cast<const uint8_t*>(secret_key_block.Data()),
      secret_key_block.Size()};

  auto status = Rust::gfr_crypto_add_user_id(ctx.GetChannel(), key_block_buffer,
                                             uid_utf8.constData(),
                                             FetchPasswordCallback, &out_block);

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

auto DeleteUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                       const QString& uid) -> bool {
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
  if (std::none_of(uids.begin(), uids.end(), [uid](const GFUserId& u) -> bool {
        return u.ToString() == uid;
      })) {
    LOG_E() << "UID: " << uid << " not found in key: " << key->Fingerprint();
    return false;
  }

  auto uid_utf8 = uid.toUtf8();

  auto secret_key_block = gf_key->blocks.secret_key;
  if (secret_key_block.Empty()) {
    LOG_E() << "Secret key block is empty for key: " << key->Fingerprint();
    return false;
  }

  char* out_block = nullptr;

  Rust::GfrBuffer key_block_buffer = {
      reinterpret_cast<const uint8_t*>(secret_key_block.Data()),
      secret_key_block.Size()};

  auto status = Rust::gfr_crypto_delete_user_id(
      key_block_buffer, uid_utf8.constData(), &out_block);

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

  // Replace (do not merge), merging would re-add the deleted UID from the
  // existing database entry.
  if (!ReplaceKeyInDatabaseRpgp(ctx.GetChannel(), GFBuffer(out_block_str))) {
    LOG_E() << "Failed to store updated key block after deleting UID: " << uid
            << " from key: " << key->Fingerprint();
    return false;
  }

  return true;
}

auto SetPrimaryUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                           const QString& uid) -> bool {
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

  auto secret_key_block = gf_key->blocks.secret_key;
  if (secret_key_block.Empty()) {
    LOG_E() << "Secret key block is empty for key: " << key->Fingerprint();
    return false;
  }

  auto uid_utf8 = uid.toUtf8();

  char* out_block = nullptr;

  Rust::GfrBuffer key_block_buffer = {
      reinterpret_cast<const uint8_t*>(secret_key_block.Data()),
      secret_key_block.Size()};

  auto status = Rust::gfr_crypto_set_primary_user_id(
      ctx.GetChannel(), key_block_buffer, uid_utf8.constData(),
      FetchPasswordCallback, &out_block);

  LOG_D() << "Rust function gfr_crypto_set_primary_user_id returned status: "
          << static_cast<int>(status);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Failed to set primary UID: " << uid
            << " for key: " << key->Fingerprint()
            << ", status: " << static_cast<int>(status);
    return false;
  }

  if (out_block == nullptr) {
    LOG_E() << "Output block is null after setting primary UID: " << uid
            << " for key: " << key->Fingerprint();
    return false;
  }

  auto out_block_str = QString::fromUtf8(out_block);
  Rust::gfr_crypto_free_string(out_block);

  LOG_D() << "Successfully set primary UID: " << uid << " for key: " << key;

  // Replace (do not merge) — merging would keep the previous primary UID's
  // self-signature and defeat the change.
  if (!ReplaceKeyInDatabaseRpgp(ctx.GetChannel(), GFBuffer(out_block_str))) {
    LOG_E() << "Failed to store updated key block after setting primary UID: "
            << uid << " for key: " << key->Fingerprint();
    return false;
  }

  return true;
}

auto RevokeUIDRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                       const QString& uid, int reason_code,
                       const QString& reason_text) -> bool {
  LOG_D() << "Revoking UID: " << uid << " from key: " << key->Fingerprint()
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
    LOG_E() << "Cannot revoke UID from a public key";
    return false;
  }

  auto secret_key_block = gf_key->blocks.secret_key;
  if (secret_key_block.Empty()) {
    LOG_E() << "Secret key block is empty for key: " << key->Fingerprint();
    return false;
  }

  auto uid_utf8 = uid.toUtf8();
  auto reason_text_utf8 = reason_text.toUtf8();

  int mapped_reason_code = 0;
  switch (reason_code) {
    case 4:
      mapped_reason_code = 32;
      break;
    default:
      mapped_reason_code = 0;
      break;
  }

  char* out_block = nullptr;

  Rust::GfrBuffer key_block_buffer = {
      reinterpret_cast<const uint8_t*>(secret_key_block.Data()),
      secret_key_block.Size()};

  auto status = Rust::gfr_crypto_revoke_user_id(
      ctx.GetChannel(), key_block_buffer, uid_utf8.constData(),
      static_cast<Rust::GfrRevocationCode>(mapped_reason_code),
      reason_text_utf8.constData(), FetchPasswordCallback, &out_block);

  LOG_D() << "Rust function gfr_crypto_revoke_user_id returned status: "
          << static_cast<int>(status);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Failed to revoke UID: " << uid
            << " for key: " << key->Fingerprint()
            << ", status: " << static_cast<int>(status);
    return false;
  }

  if (out_block == nullptr) {
    LOG_E() << "Output block is null after revoking UID: " << uid
            << " for key: " << key->Fingerprint();
    return false;
  }

  auto out_block_str = QString::fromUtf8(out_block);
  Rust::gfr_crypto_free_string(out_block);

  LOG_D() << "Successfully revoked UID: " << uid
          << " for key: " << key->Fingerprint();

  // Replace (do not merge) so the revocation self-signature is stored as the
  // authoritative state of the key.
  if (!ReplaceKeyInDatabaseRpgp(ctx.GetChannel(), GFBuffer(out_block_str))) {
    LOG_E() << "Failed to store updated key block after revoking UID: " << uid
            << " for key: " << key->Fingerprint();
    return false;
  }

  return true;
}

}  // namespace GpgFrontend
