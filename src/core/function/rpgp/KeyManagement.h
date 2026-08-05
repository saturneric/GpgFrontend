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

#include "core/function/openpgp/OpenPGPContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief
 *
 * @param ctx
 * @param keys
 */
auto DeleteKeysRpgpImpl(OpenPGPContext& ctx, const GpgAbstractKeyPtrList& keys)
    -> bool;

/**
 * @brief Re-protect the whole key -- primary and every subkey -- under one new
 * passphrase, matching what `gpg --passwd` does.
 *
 * @param ctx
 * @param key
 * @param data_object
 * @return GpgError
 */
auto ModifyKeyPassphraseRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                                 const DataObjectPtr& data_object) -> GpgError;

/**
 * @brief Re-protect a single subkey, leaving the primary and every other subkey
 * under their current passphrase.
 *
 * @param ctx
 * @param key
 * @param skey_fpr fingerprint of the subkey to re-protect
 * @param data_object
 * @return GpgError
 */
auto ModifySubkeyPassphraseRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                                    const SubkeyId& skey_fpr,
                                    const DataObjectPtr& data_object)
    -> GpgError;

/**
 * @brief
 *
 * @param ctx
 * @param key
 * @param skey_idx
 * @return GpgError
 */
auto DeleteSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                          int skey_idx) -> bool;

/**
 * @brief
 *
 * @param ctx
 * @param key
 * @param subkey_index
 * @param reason_code
 * @param reason_text
 * @return true
 * @return false
 */
auto RevokeSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                          int subkey_index, int reason_code,
                          const QString& reason_text) -> bool;

/**
 * @brief
 *
 * @param ctx
 * @param secret_key
 * @param reason_code
 * @param reason_text
 * @return true
 * @return false
 */
auto GenerateRevCertRpgpImpl(OpenPGPContext& ctx_, const GpgKeyPtr& secret_key,
                             const QString& output_path, int reason_code,
                             const QString& reason_text) -> bool;

/**
 * @brief Change the expiration of a primary key or a single subkey.
 *
 * An empty @p skey_fpr (or one equal to the primary key fingerprint) targets
 * the primary key; any other fingerprint targets that subkey. A `std::nullopt`
 * @p expires clears the expiration ("never expires").
 *
 * @param ctx
 * @param key
 * @param skey_fpr fingerprint of the subkey to change, or empty for the primary
 * @param expires absolute expiration time, or nullopt for "never expires"
 * @return GpgError
 */
auto SetExpireRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                       const SubkeyId& skey_fpr,
                       const std::optional<QDateTime>& expires) -> GpgError;

/**
 * @brief Extract the OpenPGP ECDH KDF parameters of the (sub)key @p subkey_fpr
 * from an armored public key block, as the hexified octet-string
 * `03 01 <hash> <cipher>` (the value gpg-agent's `KEYTOCARD <ecdh>` argument
 * expects when moving an ECDH encryption subkey onto a smart card).
 *
 * This only inspects public key material and is engine-independent -- the block
 * may originate from either engine. gpgme does not expose these bytes, so the
 * rPGP parser is used regardless of the key's home engine.
 *
 * @param public_key_block armored public key block containing the (sub)key
 * @param subkey_fpr fingerprint of the ECDH (sub)key
 * @return {GPG_ERR_NO_ERROR, hex} on success; a non-zero GpgError otherwise
 */
auto GF_CORE_EXPORT GetEcdhKdfParamsRpgpImpl(const GFBuffer& public_key_block,
                                             const QString& subkey_fpr)
    -> std::tuple<GpgError, QString>;
}  // namespace GpgFrontend