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

#include "core/function/AppSecureKeyManager.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Singleton operator for storing and retrieving encrypted data objects.
 *
 * Provides key-by-name and reference-based access to data objects persisted on
 * disk. JSON and raw binary variants are both supported. All objects are
 * encrypted with a per-object key derived from the application's secure key
 * material via libsodium (XChaCha20-Poly1305).
 */
class GF_CORE_EXPORT DataObjectOperator
    : public SingletonFunctionObject<DataObjectOperator> {
 public:
  /**
   * @brief Construct and initialise the operator, loading key material from
   * GlobalSettingStation.
   *
   * @param channel singleton channel identifier
   */
  explicit DataObjectOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Encrypt and store a JSON document on disk under the given key.
   *
   * @param key logical name used to derive the storage reference
   * @param value JSON document to encrypt and store
   * @return hex-encoded reference string to the stored object, or empty on
   * failure
   */
  auto StoreDataObj(const QString &key, const QJsonDocument &value) -> QString;

  /**
   * @brief Decrypt and retrieve a JSON data object by logical key name.
   *
   * @param key logical name identifying the stored object
   * @return decrypted JSON document, or empty if not found or decryption fails
   */
  auto GetDataObject(const QString &key) -> std::optional<QJsonDocument>;

  /**
   * @brief Decrypt and retrieve a JSON data object by its hex-encoded
   * reference.
   *
   * @param ref hex-encoded 64-character reference string
   * @return decrypted JSON document, or empty if not found or decryption fails
   */
  auto GetDataObjectByRef(const QString &ref) -> std::optional<QJsonDocument>;

  /**
   * @brief Encrypt and store raw binary data on disk under the given key.
   *
   * @param key logical name used to derive the storage reference
   * @param value binary data to encrypt and store
   * @return hex-encoded reference string to the stored object, or empty on
   * failure
   */
  auto StoreSecDataObj(const QString &key, const GFBuffer &value) -> QString;

  /**
   * @brief Decrypt and retrieve a raw binary data object by logical key name.
   *
   * @param key logical name identifying the stored object
   * @return decrypted data buffer, or empty if not found or decryption fails
   */
  auto GetSecDataObject(const QString &key) -> GFBufferOrNone;

  /**
   * @brief Decrypt and retrieve a raw binary data object by its hex-encoded
   * reference.
   *
   * @param ref hex-encoded 64-character reference string
   * @return decrypted data buffer, or empty if not found or decryption fails
   */
  auto GetSecDataObjectByRef(const QString &ref) -> GFBufferOrNone;

  /**
   * @brief Delete the stored data object backing the given logical key name.
   *
   * Removing the in-memory value alone is not enough: callers that clear a
   * cached secret must also drop it from disk, otherwise it is reloaded on the
   * next start.
   *
   * @param key logical name identifying the stored object
   * @return true if the object was deleted or did not exist, false on I/O error
   */
  auto RemoveDataObj(const QString &key) -> bool;

 private:
  GlobalSettingStation &gss_ =
      GlobalSettingStation::GetInstance();  ///< Storage paths
  AppSecureKeyManager &key_mgr_ =
      AppSecureKeyManager::GetInstance();  ///< Owner of all key material
  GFBuffer l_key_;                         ///< Legacy key
  GFBuffer key_;                           ///< Active key
  GFBuffer key_id_;                        ///< Active key Id

  /**
   * @brief Derive the binary storage reference for the given object name.
   *
   * Computes HMAC-SHA256 of @p obj_name using the legacy key. If @p obj_name
   * is empty, a cryptographically random reference is generated instead.
   *
   * @param obj_name logical object name, or empty to generate a random
   * reference
   * @return binary reference (HMAC-SHA256 digest)
   */
  auto get_object_ref(const QString &obj_name) -> GFBuffer;

  /**
   * @brief Read and decrypt a binary data object from disk using its reference.
   *
   * @param ref binary reference identifying the stored object
   * @return decrypted data buffer, or empty on failure
   */
  auto read_decr_object(const GFBuffer &ref) -> GFBufferOrNone;

  /**
   * @brief Encrypt a value and write it to disk at the path derived from the
   * reference.
   *
   * @param ref binary reference identifying the storage location
   * @param value binary data to encrypt and store
   * @return hex-encoded reference string, or empty on failure
   */
  auto write_encr_object(const GFBuffer &ref, const GFBuffer &value) -> QString;

  /**
   * @brief Read, decrypt, and parse a JSON data object from disk using its
   * reference.
   *
   * @param ref binary reference identifying the stored object
   * @return parsed JSON document, or empty on failure
   */
  auto read_decr_json_object(const GFBuffer &ref)
      -> std::optional<QJsonDocument>;
};

}  // namespace GpgFrontend
