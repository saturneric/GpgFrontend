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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend {

/**
 * @brief Outcome of AppSecureKeyManager::Initialize().
 *
 * The manager never shows UI of its own — gf_core does not link QtWidgets — so
 * every failure is reported here and rendered by the application layer.
 */
enum class AppSecureKeyStatus {
  kOk,              ///< key set loaded or created successfully
  kReadFailed,      ///< a key file exists but could not be read
  kDecryptFailed,   ///< a key file was read but would not decrypt
  kWriteFailed,     ///< a newly generated key could not be persisted
  kGenerateFailed,  ///< no usable random source produced a key
};

/**
 * @brief Result of loading the application secure key set.
 */
struct AppSecureKeyInitResult {
  AppSecureKeyStatus status = AppSecureKeyStatus::kOk;

  /// Path, cause, or other context worth showing the user and logging.
  QString detail;

  [[nodiscard]] auto Ok() const -> bool {
    return status == AppSecureKeyStatus::kOk;
  }
};

/**
 * @brief Singleton owning every aspect of the application secure key.
 *
 * This is the single owner of the key material that protects everything
 * DataObjectOperator persists. It resolves the secure directory paths,
 * generates and loads the key files, derives key identities, and keeps the
 * in-memory registry mapping key ID to key material.
 *
 * Two distinct secrets flow through the private helpers and must not be
 * confused:
 *
 * - the **identity pin** feeds CalculateKeyId(), and therefore determines the
 *   ID that DataObjectOperator stores as a prefix on every object. It is
 *   non-empty only in high security mode, where the user types it at startup.
 * - the **wrap secret** encrypts the key file at rest and nothing else.
 *
 * Mixing them would change every key ID whenever the at-rest protection
 * changed, orphaning all previously stored data objects.
 */
class GF_CORE_EXPORT AppSecureKeyManager
    : public SingletonFunctionObject<AppSecureKeyManager> {
 public:
  /**
   * @brief Construct the manager.
   *
   * @param channel singleton channel identifier
   */
  explicit AppSecureKeyManager(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Load the key set from disk, creating it when absent, and register
   * every key in the in-memory registry.
   *
   * Must run before DataObjectOperator is constructed, since that caches the
   * active and legacy keys at construction time.
   *
   * @param pin identity PIN; empty below high security mode
   * @param wrap secret used only to encrypt the key file at rest; empty when
   * the key is stored unprotected
   * @return outcome, with a detail string on failure
   */
  auto Initialize(const GFBuffer& pin, const GFBuffer& wrap = {})
      -> AppSecureKeyInitResult;

  /**
   * @brief Return the active key, used to encrypt newly written objects.
   *
   * @return key material for the active key
   */
  [[nodiscard]] auto GetActiveKey() const -> GFBuffer;

  /**
   * @brief Return the ID of the active key.
   *
   * @return binary key ID
   */
  [[nodiscard]] auto GetActiveKeyId() const -> GFBuffer;

  /**
   * @brief Return the legacy key.
   *
   * @return key material for the legacy key
   */
  [[nodiscard]] auto GetLegacyKey() const -> GFBuffer;

  /**
   * @brief Look up a key by its ID.
   *
   * @param id binary key ID
   * @return key material, or an empty buffer when the ID is unknown
   */
  [[nodiscard]] auto GetKey(const GFBuffer& id) const -> GFBuffer;

  /**
   * @brief Return the directory holding the key files.
   *
   * @return absolute path to the secure directory
   */
  [[nodiscard]] auto GetKeyDir() const -> QString;

  /**
   * @brief Return the path of the legacy key file.
   *
   * @return absolute path to secure/app.key
   */
  [[nodiscard]] auto GetLegacyKeyPath() const -> QString;

  /**
   * @brief Derive the identity of a key.
   *
   * HMAC-SHA256 over @p key using @p pin as the HMAC key, falling back to a
   * fixed label when @p pin is empty. Static so tests can assert that an ID is
   * stable across changes to at-rest protection.
   *
   * @param pin identity PIN, may be empty
   * @param key key material
   * @return binary key ID
   */
  static auto CalculateKeyId(const GFBuffer& pin, const GFBuffer& key)
      -> GFBuffer;

 private:
  /**
   * @brief Generate a fresh legacy key and persist it.
   *
   * @param wrap wrap secret; when non-empty the file is written encrypted
   * @param[out] status failure detail when the returned buffer is empty
   * @return the plaintext key material, or an empty buffer on failure
   */
  auto new_legacy_key(const GFBuffer& wrap, AppSecureKeyInitResult& status)
      -> GFBuffer;

  /**
   * @brief Load or create the legacy key and register it as active.
   *
   * @param pin identity PIN
   * @param wrap wrap secret
   * @return outcome
   */
  auto init_legacy_key(const GFBuffer& pin, const GFBuffer& wrap)
      -> AppSecureKeyInitResult;

  /**
   * @brief Derive and persist the weekly rotating key. High security mode only.
   *
   * @param pin identity PIN
   * @return key material, or an empty buffer on failure
   */
  auto fetch_time_related_key(const GFBuffer& pin) -> GFBuffer;

  QMap<GFBuffer, GFBuffer> keys_;  ///< key ID to key material
  GFBuffer active_key_id_;         ///< ID of the key used for new objects
  GFBuffer legacy_key_id_;         ///< ID of the legacy key
};

}  // namespace GpgFrontend
