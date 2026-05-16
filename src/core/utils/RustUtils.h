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

#ifdef HAS_RUST_SUPPORT

#include "core/GpgCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

/**
 * @brief Return the version string of the embedded Rust crypto engine.
 * @return Rust engine version string
 */
auto GF_CORE_EXPORT RustEngineVersion() -> QString;

/**
 * @brief Convert an algorithm identifier string to the corresponding Rust
 * GfrKeyAlgo enum value.
 *
 * @param algo_id algorithm identifier string (e.g. "Ed25519")
 * @return corresponding Rust::GfrKeyAlgo enum value
 */
auto GF_CORE_EXPORT KeyAlgoId2GfrKeyAlgo(const QString& algo_id)
    -> Rust::GfrKeyAlgo;

/**
 * @brief Convert a Rust GfrKeyAlgo enum value to a human-readable algorithm
 * name.
 *
 * @param algo GfrKeyAlgo enum value
 * @return human-readable algorithm name string
 */
auto GF_CORE_EXPORT GfrKeyAlgo2KeyAlgoName(Rust::GfrKeyAlgo algo) -> QString;

/**
 * @brief Analyse an encrypted buffer to identify the intended recipients.
 *
 * Uses the Rust engine to parse recipient key IDs from @p in_buffer and
 * look them up in @p key_db.
 *
 * @param key_db key database to resolve recipient key IDs against
 * @param in_buffer encrypted data buffer to analyse
 * @return list of GFRecipient structures for each identified recipient
 */
auto GF_CORE_EXPORT SniffRecipients(GFKeyDatabase& key_db,
                                    const GFBuffer& in_buffer)
    -> QContainer<GFRecipient>;

/**
 * @brief Extract issuer key IDs from a signed data buffer.
 *
 * @param in_buffer signed or signed-and-encrypted data buffer
 * @return list of issuer key ID strings found in the buffer
 */
auto GF_CORE_EXPORT SniffIssuerKeyIds(const GFBuffer& in_buffer) -> QStringList;

/**
 * @brief Retrieve armored public key blocks needed to verify the given key IDs.
 *
 * @param key_db key database to look up keys in
 * @param key_ids list of key IDs to retrieve
 * @return list of armored public key blocks (one per found key)
 */
auto GF_CORE_EXPORT GetKeyBlocksForVerification(GFKeyDatabase& key_db,
                                                const QStringList& key_ids)
    -> QContainer<QByteArray>;

/**
 * @brief Retrieve armored key blocks for the given key IDs.
 *
 * @param key_db key database to look up keys in
 * @param key_ids list of key IDs to retrieve
 * @param secret if true, include secret key material; otherwise return only
 * public keys
 * @return list of armored key blocks (one per found key)
 */
auto GF_CORE_EXPORT GetArmoredKeyBlocksForKeys(GFKeyDatabase& key_db,
                                               const QStringList& key_ids,
                                               bool secret)
    -> QContainer<QByteArray>;

}  // namespace GpgFrontend

#endif  // HAS_RUST_SUPPORT
