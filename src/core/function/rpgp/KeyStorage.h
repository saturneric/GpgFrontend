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

#include "core/function/GFKeyDatabase.h"
#include "core/typedef/GFTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Extracts keys from a key block
 *
 * @param buffer
 * @return std::optional<GFKeyMetadata>
 */
auto GetGFKeysFromKeyBlock(const GFBuffer& buffer) -> QContainer<GFKey>;

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
 * @brief Get the Public Keys By Key Ids For Encryption object
 *
 * @param key_db
 * @param key_ids
 * @return QContainer<QByteArray>
 */
auto GetPublicKeysByKeyIdsForEncryption(GFKeyDatabase& key_db,
                                        const GpgAbstractKeyPtrList& keys)
    -> QContainer<QByteArray>;

}  // namespace GpgFrontend