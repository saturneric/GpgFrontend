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

#include "KeyStorage.h"

#include "core/GpgCoreRust.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/OpenPGPContext.h"

namespace GpgFrontend {

namespace {
auto ParseGfrMetadata(const Rust::GfrKeyMetadataC& gfr_meta) -> GFKey {
  GFKey key;
  GFKeyMetadata& meta = key.metadata;
  meta.fpr = QString::fromUtf8(gfr_meta.fpr);
  meta.key_id = QString::fromUtf8(gfr_meta.key_id);
  meta.created_at = static_cast<qint64>(gfr_meta.created_at);
  meta.has_secret = gfr_meta.has_secret;
  meta.is_revoked = gfr_meta.is_revoked;
  // GpgFrontend itself will be responsible for handling disabled keys
  meta.is_disabled = false;
  meta.algo = static_cast<int>(gfr_meta.algo);
  meta.key_length = static_cast<unsigned int>(gfr_meta.key_length);

  for (size_t i = 0; i < gfr_meta.user_id_count; ++i) {
    const auto& user_id = gfr_meta.user_ids[i];
    auto uid = GFUserId(QString::fromUtf8(user_id.user_id));

    uid.is_primary = user_id.is_primary;
    uid.is_revoked = user_id.is_revoked;
    meta.user_ids.push_back(uid);
  }

  meta.can_sign = gfr_meta.can_sign;
  meta.can_encrypt = gfr_meta.can_encrypt;
  meta.can_auth = gfr_meta.can_auth;
  meta.can_certify = gfr_meta.can_certify;

  for (size_t i = 0; i < gfr_meta.subkey_count; ++i) {
    const auto& subkey_meta = gfr_meta.subkeys[i];
    GFSubKeyMetadata sub_meta;
    sub_meta.fpr = QString::fromUtf8(subkey_meta.fpr);
    sub_meta.key_id = QString::fromUtf8(subkey_meta.key_id);
    sub_meta.created_at = static_cast<qint64>(subkey_meta.created_at);
    sub_meta.has_secret = subkey_meta.has_secret;
    sub_meta.algo = static_cast<int>(subkey_meta.algo);
    sub_meta.key_length = static_cast<unsigned int>(subkey_meta.key_length);
    sub_meta.can_sign = subkey_meta.can_sign;
    sub_meta.can_encrypt = subkey_meta.can_encrypt;
    sub_meta.can_auth = subkey_meta.can_auth;
    sub_meta.can_certify = subkey_meta.can_certify;
    sub_meta.is_revoked = subkey_meta.is_revoked;

    LOG_D() << "Parsed subkey metadata, fpr: " << sub_meta.fpr
            << ", key_id: " << sub_meta.key_id
            << ", created_at: " << sub_meta.created_at
            << ", has_secret: " << sub_meta.has_secret
            << ", algo: " << sub_meta.algo
            << ", key_length: " << sub_meta.key_length
            << ", can_sign: " << sub_meta.can_sign
            << ", can_encrypt: " << sub_meta.can_encrypt
            << ", can_auth: " << sub_meta.can_auth
            << ", can_certify: " << sub_meta.can_certify
            << ", is_revoked: " << sub_meta.is_revoked;

    meta.subkeys.push_back(sub_meta);
  }

  GFKeyBlocks& blocks = key.blocks;
  if (meta.has_secret) {
    blocks.secret_key = QString::fromUtf8(gfr_meta.secret_key_block);
  }
  blocks.public_key = QString::fromUtf8(gfr_meta.public_key_block);

  return key;
}
}  // namespace

auto GetGFKeysFromKeyBlock(const GFBuffer& buffer) -> QContainer<GFKey> {
  size_t out_meta_count = 0;
  Rust::GfrKeyMetadataC* out_meta_array = nullptr;
  auto key_block_data = buffer.ConvertToQByteArray();

  auto err = Rust::gfr_crypto_extract_metadata(
      key_block_data.data(), &out_meta_array, &out_meta_count);
  if (err != Rust::GfrStatus::Success) {
    LOG_E() << "gfr_crypto_extract_metadata error, code: "
            << static_cast<int>(err);
    return {};
  }

  QContainer<GFKey> keys;

  for (size_t i = 0; i < out_meta_count; ++i) {
    const auto& out_metadata = out_meta_array[i];
    auto key = ParseGfrMetadata(out_metadata);
    LOG_D() << "extracted key metadata, fpr: " << key.metadata.fpr
            << ", key_id: " << key.metadata.key_id
            << ", user_ids: " << key.metadata.user_ids.size()
            << ", created_at: " << key.metadata.created_at
            << ", has_secret: " << key.metadata.has_secret
            << ", is_revoked: " << key.metadata.is_revoked
            << ", algo: " << key.metadata.algo
            << ", key_length: " << key.metadata.key_length
            << ", can_sign: " << key.metadata.can_sign
            << ", can_encrypt: " << key.metadata.can_encrypt
            << ", can_auth: " << key.metadata.can_auth
            << ", can_certify: " << key.metadata.can_certify
            << ", subkeys: " << key.metadata.subkeys.size();
    keys.push_back(key);
  }

  Rust::gfr_free_metadata_array(out_meta_array, out_meta_count);

  for (auto& key : keys) {
    auto meta = key.metadata;

    if (meta.has_secret && key.blocks.public_key.isEmpty()) {
      char* public_key = nullptr;
      auto err = Rust::gfr_crypto_extract_public_key(key_block_data.data(),
                                                     &public_key);

      if (err != Rust::GfrStatus::Success) {
        LOG_E() << "gfr_crypto_extract_public_key error, code: "
                << static_cast<int>(err);
        return {};
      }

      key.blocks.public_key = QString::fromUtf8(public_key);
      Rust::gfr_crypto_free_string(public_key);

    } else if (!meta.has_secret && key.blocks.public_key.isEmpty()) {
      key.blocks.public_key = key_block_data;
    }
  }

  return keys;
}

auto CreateOrUpdateGFKeyInDatabase(int channel, const GFKey& key) -> bool {
  auto key_db = OpenPGPContext::GetInstance(channel).KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return false;
  }

  if (key.metadata.fpr.isEmpty()) {
    LOG_E() << "key metadata must contain a valid fingerprint";
    return false;
  }

  if (key.metadata.key_id.isEmpty()) {
    LOG_E() << "key metadata must contain a valid key ID";
    return false;
  }

  if (key.blocks.public_key.isEmpty()) {
    LOG_E() << "key blocks must contain a valid public key block";
    return false;
  }

  if (key.metadata.algo <= 0) {
    LOG_E() << "key metadata must contain a valid algorithm identifier, algo: "
            << key.metadata.algo;
    return false;
  }

  auto succ = key_db->SaveKey(key.metadata, key.blocks);
  if (!succ) {
    LOG_E() << "failed to save key with fpr: " << key.metadata.fpr;
    return false;
  }

  GpgKeyRepository::GetInstance(channel).FlushKeyCache();
  return succ;
}

auto GetKeyByKeyIdsForDecryption(GFKeyDatabase& key_db,
                                 const QStringList& key_ids)
    -> std::optional<GFKey> {
  // Variables to store our target key for decryption
  std::optional<GFKey> result;
  bool found_usable_secret = false;

  // 2. Iterate through all sniffed recipient IDs to find a USABLE secret key
  for (const auto& key_id : key_ids) {
    // Fetch the full metadata tree (Primary + Subkeys)
    auto meta_opt = key_db.GetKeyMetadata(key_id);
    if (!meta_opt) continue;

    // Check if the recipient ID matches the primary key itself
    // (Rare for encryption, but possible with older RSA keys)
    if (meta_opt->key_id.toUpper() == key_id.toUpper() ||
        meta_opt->fpr.toUpper() == key_id.toUpper()) {
      if (meta_opt->has_secret) {
        found_usable_secret = true;
      }
    } else {
      // Check if the recipient ID matches a subkey, and IF THAT SUBKEY HAS A
      // SECRET
      for (const auto& subkey : meta_opt->subkeys) {
        if (subkey.key_id.toUpper() == key_id.toUpper() ||
            subkey.fpr.toUpper() == key_id.toUpper()) {
          if (subkey.has_secret) {
            found_usable_secret = true;
          } else {
            LOG_W() << "Subkey " << key_id
                    << " matched, but its secret is stripped/offline.";
          }
          break;  // Stop searching subkeys for this specific recipient_id
        }
      }
    }

    // If we found a usable secret key, fetch the actual key block and stop
    // searching
    if (found_usable_secret) {
      auto key = key_db.GetKeyByIdentifier(meta_opt->fpr);
      if (key && !key->blocks.secret_key.isEmpty()) {
        result = *key;
        break;
      }
      // Fallback in case DB is inconsistent
      found_usable_secret = false;
    }
  }

  // 3. Handle the result of our search
  if (!found_usable_secret) {
    LOG_E() << "No USABLE secret key found in local database to decrypt this "
               "message. "
            << "Keys might be offline or on a smartcard.";
    return {};
  }

  return result;
}

auto GetPublicKeysByKeyIdsForEncryption(GFKeyDatabase& key_db,
                                        const GpgAbstractKeyPtrList& keys)
    -> QContainer<QByteArray> {
  QContainer<QByteArray> result;

  for (const auto& key : keys) {
    auto key_block = key_db.GetKeyBlocks(key->Fingerprint());
    if (!key_block || key_block->public_key.isEmpty()) {
      LOG_W() << "No valid public key block found for key with fpr: "
              << key->Fingerprint();
      continue;
    }

    // Keep the QByteArray alive by pushing it to the vector
    result.push_back(key_block->public_key.toUtf8());
  }

  return result;
}

auto GetSecretKeysByKeyIdForSigning(GFKeyDatabase& key_db,
                                    const GpgAbstractKeyPtrList& key)
    -> QContainer<QByteArray> {
  QContainer<QByteArray> result;

  for (const auto& k : key) {
    auto key_block = key_db.GetKeyBlocks(k->Fingerprint());
    if (!key_block || key_block->secret_key.isEmpty()) {
      LOG_W() << "No valid secret key block found for key with fpr: "
              << k->Fingerprint();
      continue;
    }

    // Keep the QByteArray alive by pushing it to the vector
    result.push_back(key_block->secret_key.toUtf8());
  }

  return result;
}

auto RefreshKeyMetaInDatabase(GFKeyDatabase& key_db, const QString& key_id)
    -> bool {
  auto key = key_db.GetKeyByIdentifier(key_id);
  if (!key) {
    LOG_E() << "cannot find key with id: " << key_id
            << " in database to refresh";
    return false;
  }

  auto disable_and_err = [&key_db, key, key_id]() -> bool {
    if (!key_db.DisableKey(key->metadata.fpr)) {
      LOG_E() << "failed to disable key with id: " << key_id;
    }
    return false;
  };

  auto key_blocks = key->blocks;
  auto key_block = key_blocks.secret_key.isEmpty() ? key_blocks.public_key
                                                   : key_blocks.secret_key;
  if (key_block.isEmpty()) {
    LOG_E() << "no valid key block found for key with id: " << key_id
            << " to refresh metadata";
    return disable_and_err();
  }

  auto key_block_utf8 = key_block.toUtf8();
  auto keys = GetGFKeysFromKeyBlock(GFBuffer(key_block_utf8));
  if (keys.empty()) {
    LOG_E() << "cannot parse key blocks for key with id: " << key_id
            << " to refresh metadata";
    return disable_and_err();
  }

  const auto& final_key = keys.front();

  if (final_key.metadata.fpr.toUpper() != key->metadata.fpr.toUpper()) {
    LOG_E()
        << "fingerprint mismatch when refreshing key metadata for key with id: "
        << key_id;
    return disable_and_err();
  }

  if (!key_db.SaveKey(final_key.metadata, final_key.blocks, false)) {
    LOG_E() << "failed to refresh key metadata for key with id: " << key_id;
    return disable_and_err();
  }

  LOG_D() << "successfully refreshed key metadata for key with id: " << key_id;
  return true;
}

auto FlushKeyCacheRpgpImpl(
    OpenPGPContext& ctx, const QSharedPointer<GpgKeyPtrList>& keys_cache,
    const QSharedPointer<QMap<QString, GpgAbstractKeyPtr>>& keys_search_cache)
    -> bool {
  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return false;
  }

  auto key_meta_list = key_db->GetMetadataList();

  for (const auto& meta : key_meta_list) {
    auto key = QSharedPointer<GpgKey>::create(meta);
    if (key == nullptr) {
      LOG_W() << "cannot get key with fpr: " << meta.fpr;
      continue;
    }

    keys_cache->push_back(key);
    keys_search_cache->insert(key->Fingerprint(), key);
    keys_search_cache->insert(key->ID(), key);

    for (const auto& s_key : key->SubKeys()) {
      if (s_key.ID() == key->ID()) continue;

      // don't add adsk key or it will cause bugs
      if (s_key.IsADSK()) continue;

      // subkeys should be weaker than primary key
      if (keys_search_cache->contains(s_key.ID())) continue;

      auto p_s_key = SecureCreateSharedObject<GpgSubKey>(s_key);
      keys_search_cache->insert(s_key.ID(), p_s_key);
      keys_search_cache->insert(s_key.Fingerprint(), p_s_key);
    }
  }

  return true;
}

auto GetKeyPtrRpgpImpl(OpenPGPContext& ctx, const QString& key_id, bool secret)
    -> GpgKeyPtr {
  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "cannot get key database for channel: " << ctx.GetChannel();
    return nullptr;
  }

  std::optional<GFKeyMetadata> meta_list;
  if (key_id.endsWith("!")) {
    auto real_key_id = key_id.left(key_id.length() - 1);
    meta_list = key_db->GetKeyMetadata(real_key_id);

    if (!meta_list.has_value()) {
      LOG_W() << "cannot get key metadata for key id: " << real_key_id;
      return nullptr;
    }

    // mark the subkey metadata as marked if the key_id is a subkey id
    for (auto& sub : meta_list->subkeys) {
      if ((sub.fpr == sub.key_id || sub.fpr.contains(real_key_id))) {
        LOG_D() << "mark subkey with key id: " << real_key_id << " as marked";
        sub.marked = true;
        break;
      }
    }

  } else {
    meta_list = key_db->GetKeyMetadata(key_id);
  }

  if (!meta_list.has_value()) {
    LOG_W() << "cannot get key metadata for key id: " << key_id;
    return nullptr;
  }

  if (secret && !meta_list->has_secret) {
    LOG_W() << "key with fpr: " << key_id
            << " does not have secret key, but requested to get secret key";
    return nullptr;
  }

  return SecureCreateSharedObject<GpgKey>(meta_list.value());
}

auto FlushKeyDatabaseRpgpImpl(OpenPGPContext& ctx) -> bool {
  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return false;
  }

  auto key_meta_list = key_db->GetMetadataList();
  for (const auto& meta : key_meta_list) {
    if (!RefreshKeyMetaInDatabase(*key_db, meta.fpr)) {
      LOG_W() << "refresh key meta in database failed, fpr: " << meta.fpr;
    }
  }

  return true;
}

}  // namespace GpgFrontend