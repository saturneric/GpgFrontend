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

#include "core/function/openpgp/OpenPGPContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton for key lifecycle management operations.
 *
 * Covers deletion, expiry, passphrase changes, revocation certificate
 * generation, subkey management, trust assignment, and key certification.
 */
class GF_CORE_EXPORT KeyManagementOperation
    : public SingletonFunctionObject<KeyManagementOperation> {
 public:
  /**
   * @brief Construct the operation handler for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit KeyManagementOperation(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Delete a list of keys from the keyring.
   *
   * @param keys list of keys to delete (both public and secret material)
   */
  void DeleteKeys(const GpgAbstractKeyPtrList& keys);

  /**
   * @brief Delete a single key from the keyring.
   *
   * @param key key to delete
   */
  void DeleteKey(const GpgAbstractKeyPtr& key);

  /**
   * @brief Set or clear the expiry date of a primary key or subkey.
   *
   * @param key primary key whose expiry is to be set
   * @param skey_fpr fingerprint of the subkey to update, or empty for the
   * primary key
   * @param expires new expiry date, or empty optional to remove the expiry
   * @return GpgError; GPG_ERR_NO_ERROR on success
   */
  auto SetExpire(const GpgKeyPtr& key, const SubkeyId& skey_fpr,
                 const std::optional<QDateTime>& expires) -> GpgError;

  /**
   * @brief Generate a revocation certificate for the given key and write it to
   * a file.
   *
   * @param key key for which to generate the certificate
   * @param output_path destination file path for the revocation certificate
   * @param reason_code revocation reason code (0=no reason, 1=compromised,
   * 2=superseded, 3=no longer used)
   * @param reason_text human-readable reason description
   */
  void GenerateRevokeCert(const GpgKeyPtr& key, const QString& output_path,
                          int reason_code, const QString& reason_text);

  /**
   * @brief Change the passphrase protecting the given key (async).
   *
   * @param key key whose passphrase should be changed
   * @param cb callback invoked on completion
   */
  void ModifyPassword(const GpgKeyPtr& key, const GpgOperationCallback& cb);

  /**
   * @brief Add an Additional Decryption Subkey (ADSK) to the given key (async).
   *
   * @param key primary key to add the ADSK to
   * @param adsk subkey to add as an ADSK
   * @param cb callback invoked on completion
   */
  void AddADSK(const GpgKeyPtr& key, const GpgSubKey& adsk,
               const GpgOperationCallback& cb);

  /**
   * @brief Add an Additional Decryption Subkey (ADSK) to the given key (sync).
   *
   * @param key primary key to add the ADSK to
   * @param adsk subkey to add as an ADSK
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto AddADSKSync(const GpgKeyPtr& key, const GpgSubKey& adsk)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Certify (sign) a key's user IDs with the given signer keys.
   *
   * @param key key to certify
   * @param keys list of signer keys to certify with
   * @param uid user ID string to certify (empty means all user IDs)
   * @param expires optional expiry for the certification signature
   * @return true on success, false on failure
   */
  auto SignKey(const GpgKeyPtr& key, const GpgAbstractKeyPtrList& keys,
               const QString& uid, const std::optional<QDateTime>& expires)
      -> bool;

  /**
   * @brief Revoke specific certifications (key signatures) on the given key.
   *
   * @param key key whose certifications are to be revoked
   * @param signature_id list of signature IDs to revoke
   * @return true if the revocations were applied successfully
   */
  auto RevKeySignature(const GpgKeyPtr& key, const SignIdArgsList& signature_id)
      -> bool;

  /**
   * @brief Set the owner-trust level for the given key.
   *
   * @param key key whose trust level is to be updated
   * @param trust_level gpgme trust level integer (e.g. GPGME_VALIDITY_FULL)
   * @return true on success
   */
  auto SetOwnerTrustLevel(const GpgAbstractKeyPtr& key, int trust_level)
      -> bool;

  /**
   * @brief Delete a subkey from the given primary key.
   *
   * @param key primary key containing the subkey
   * @param skey_idx zero-based index of the subkey to delete in the subkey list
   * @return true on success
   */
  auto DeleteSubkey(const GpgKeyPtr& key, int skey_idx) -> bool;

  /**
   * @brief Revoke a subkey of the given primary key.
   *
   * @param key primary key containing the subkey
   * @param skey_idx zero-based index of the subkey to revoke
   * @param reason_code revocation reason code
   * @param reason_text human-readable reason description
   * @return true on success
   */
  auto RevokeSubkey(const GpgKeyPtr& key, int skey_idx, int reason_code,
                    const QString& reason_text) -> bool;

 private:
  // OpenPGP context for this channel.
  OpenPGPContext& ctx_ =
      OpenPGPContext::GetInstance(SingletonFunctionObject::GetChannel());
};
}  // namespace GpgFrontend
