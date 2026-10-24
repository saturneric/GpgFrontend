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

#pragma once

#include <optional>

#include "core/function/GFKeyDatabase.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/typedef/GFTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Extracts keys from a key block
 *
 * @param buffer
 * @return std::optional<GFKeyMetadata>
 */
auto GetGFKeysFromKeyBlock(const GFBuffer& buffer) -> QList<GFKey>;

/**
 * @brief Creates or updates a GFKey in the database based on the provided
 * metadata and key blocks. If a key with the same fingerprint already exists,
 * it will be updated with the new metadata and key blocks. If no such key
 * exists, a new entry will be created in the database.
 *
 * @param key_db
 * @param key
 * @return true
 * @return false
 */
auto CreateOrUpdateGFKeyInDatabase(int channel, const GFKey& key) -> bool;

/**
 * @brief Replace the key(s) parsed from @p in_buffer in the database.
 *
 * Unlike ImportKeyRpgpImpl(), this does NOT merge with the existing key — the
 * parsed key block overwrites the stored entry verbatim. Use this for in-place
 * mutations (delete/revoke UID, set primary UID) where the new block already
 * represents the complete, desired final state and a merge would resurrect
 * removed data.
 *
 * The edit was computed from @p before. If the stored key no longer holds
 * those blocks, another operation changed it in between, and overwriting it
 * would silently undo that change: nothing is saved and false is returned.
 * The check and the save run as one unit (GFKeyDatabase::RunExclusive()).
 *
 * @param channel OpenPGP context channel
 * @param in_buffer armored key block to store
 * @param before the stored blocks the edit started from
 * @return true if all parsed keys were saved successfully
 */
auto GF_CORE_EXPORT ReplaceKeyInDatabaseRpgp(int channel,
                                             const GFBuffer& in_buffer,
                                             const GFKeyBlocks& before) -> bool;

/**
 * @brief Save @p key to @p key_db after checking it is complete.
 *
 * CreateOrUpdateGFKeyInDatabase() without the key cache flush, for use
 * inside GFKeyDatabase::RunExclusive(): the flush must run outside it.
 *
 * @return true if the key was saved
 */
auto SaveGFKeyToDatabase(GFKeyDatabase& key_db, const GFKey& key) -> bool;

/**
 * @brief Get the Key By Key Ids For Decryption object
 *
 * @param key_db
 * @param key_ids
 * @return std::optional<GFKey>
 */
auto GetKeyByKeyIdsForDecryption(GFKeyDatabase& key_db,
                                 const QStringList& key_ids)
    -> std::optional<GFKey>;

/**
 * @brief The key block of @p key, pinned to the subkey the user picked.
 *
 * When one of the key's subkeys is marked (picked with a "<fpr>!" id), the
 * block is prefixed with "<fpr>!\n" so the rPGP engine uses exactly that
 * subkey instead of auto-selecting one. Otherwise the block is returned as is.
 *
 * @param key key the block belongs to
 * @param key_block armored public or secret key block
 * @return GFBuffer
 */
auto PinMarkedSubkey(const GpgKey& key, const GFBuffer& key_block) -> GFBuffer;

/**
 * @brief PinMarkedSubkey() for an abstract key; only a primary key (not a
 * subkey or group) can carry a marked subkey.
 *
 * @param key key the block belongs to
 * @param key_block armored public or secret key block
 * @return GFBuffer
 */
auto PinMarkedSubkey(const GpgAbstractKey& key, const GFBuffer& key_block)
    -> GFBuffer;

/**
 * @brief Get the Public Keys By Key Ids For Encryption object
 *
 * @param key_db
 * @param key_ids
 * @return QList<GFBuffer>
 */
auto GetPublicKeysByKeyIdsForEncryption(GFKeyDatabase& key_db,
                                        const GpgKeyList& keys)
    -> QList<GFBuffer>;

/**
 * @brief Get the Secret Keys By Key Id For Signing object
 *
 * @param key_db
 * @param key
 * @return QList<GFBuffer>
 */
auto GetSecretKeysByKeyIdForSigning(GFKeyDatabase& key_db,
                                    const GpgAbstractKeyPtrList& key)
    -> QList<GFBuffer>;

/**
 * @brief
 *
 * @param key_db
 * @param key_id
 * @return true
 * @return false
 */
auto RefreshKeyMetaInDatabase(GFKeyDatabase& key_db, const QString& key_id)
    -> bool;

/**
 * @brief
 *
 * @param ctx
 * @param key_id
 * @param secret
 * @return GpgKeyPtr
 */
auto GetKeyPtrRpgpImpl(OpenPGPContext& ctx, const QString& key_id, bool secret)
    -> GpgKeyPtr;

/**
 * @brief Report the OpenPGP packet version of a key's primary key.
 *
 * The engine parses the key packet when importing, so the version is already
 * in the key metadata; nothing is exported or re-parsed here.
 *
 * @param ctx
 * @param fpr full fingerprint of the primary key
 * @return 4, 5 or 6; 0 if the key is unknown or its version was not recorded
 */
auto GetKeyVersionRpgpImpl(OpenPGPContext& ctx, const QString& fpr) -> int;

/**
 * @brief
 *
 * @param ctx
 * @param repo
 * @param keys_cache
 * @param keys_search_cache
 * @return true
 * @return false
 */
auto FlushKeyCacheRpgpImpl(
    OpenPGPContext& ctx, const QSharedPointer<GpgKeyPtrList>& keys_cache,
    const QSharedPointer<QMap<QString, GpgAbstractKeyPtr>>& keys_search_cache)
    -> bool;

/**
 * @brief
 *
 * @param ctx
 * @return true
 * @return false
 */
auto FlushKeyDatabaseRpgpImpl(OpenPGPContext& ctx) -> bool;
}  // namespace GpgFrontend