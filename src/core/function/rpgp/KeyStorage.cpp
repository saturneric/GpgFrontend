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
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyGetter.h"

namespace GpgFrontend {

namespace {
auto ParseGfrMetadata(const Rust::GfrKeyMetadataC& gfr_meta) -> GFKey {
  GFKey key;
  GFKeyMetadata& meta = key.metadata;
  meta.fpr = QString::fromUtf8(gfr_meta.fpr);
  meta.key_id = QString::fromUtf8(gfr_meta.key_id);
  meta.user_id = QString::fromUtf8(gfr_meta.user_id);
  meta.created_at = static_cast<qint64>(gfr_meta.created_at);
  meta.has_secret = gfr_meta.has_secret;
  meta.algo = static_cast<int>(gfr_meta.algo);

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

    sub_meta.can_sign = subkey_meta.can_sign;
    sub_meta.can_encrypt = subkey_meta.can_encrypt;
    sub_meta.can_auth = subkey_meta.can_auth;
    sub_meta.can_certify = subkey_meta.can_certify;

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
            << ", user_id: " << key.metadata.user_id
            << ", created_at: " << key.metadata.created_at
            << ", has_secret: " << key.metadata.has_secret
            << "subkeys: " << key.metadata.subkeys.size();
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
  auto key_db = GpgContext::GetInstance(channel).KeyDatabase();
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

  GpgKeyGetter::GetInstance(channel).FlushKeyCache();
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

}  // namespace GpgFrontend