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
  kOK,               ///< key set loaded or created successfully
  kREAD_FAILED,      ///< a key file exists but could not be read
  kDECRYPT_FAILED,   ///< a key file was read but would not decrypt
  kWRITE_FAILED,     ///< a newly generated key could not be persisted
  kGENERATE_FAILED,  ///< no usable random source produced a key
};

/**
 * @brief Result of loading the application secure key set.
 */
struct AppSecureKeyInitResult {
  AppSecureKeyStatus status = AppSecureKeyStatus::kOK;

  /// Path, cause, or other context worth showing the user and logging.
  QString detail;

  [[nodiscard]] auto Ok() const -> bool {
    return status == AppSecureKeyStatus::kOK;
  }
};

class SystemSecretStore;

/**
 * @brief How the application key file is protected at rest.
 *
 * All three are pure at-rest backends: none of them takes part in deriving a
 * key identity, so switching between them never changes a key ID and never
 * orphans a stored data object.
 */
enum class AppKeyProtection {
  kNONE,      ///< key file is stored as plaintext
  kKEYCHAIN,  ///< encrypted with a random secret in the system credential store
  kPIN,       ///< encrypted with a PIN the user types at startup
};

/**
 * @brief Outcome of AppSecureKeyManager::ChangeProtection().
 */
enum class AppKeyProtectionStatus {
  kOK,                 ///< the file now carries the requested protection
  kUNCHANGED,          ///< the requested protection was already in effect
  kBAD_PIN,            ///< kPIN was requested without a usable PIN
  kSTORE_UNAVAILABLE,  ///< kKEYCHAIN was requested but no store could be used
  kSEAL_FAILED,        ///< the key could not be encrypted for its new form
  kIO_FAILED,          ///< the key file could not be rewritten
};

/**
 * @brief Result of AppSecureKeyManager::ChangeProtection().
 */
struct AppKeyProtectionResult {
  AppKeyProtectionStatus status = AppKeyProtectionStatus::kOK;

  /// Backend name, path, or cause, for the log and any dialog.
  QString detail;

  [[nodiscard]] auto Ok() const -> bool {
    return status == AppKeyProtectionStatus::kOK ||
           status == AppKeyProtectionStatus::kUNCHANGED;
  }
};

/**
 * @brief Parse the stored spelling of a protection mode.
 *
 * Case-insensitive, and anything unrecognised reads as kNONE: a typo in
 * ENV.ini should leave the key unprotected, not demand a PIN nobody ever set.
 *
 * @param s spelling from ENV.ini or the settings store
 * @return the parsed mode
 */
auto GF_CORE_EXPORT AppKeyProtectionFromString(const QString& s)
    -> AppKeyProtection;

/**
 * @brief Canonical spelling of a protection mode, as stored.
 *
 * @param p mode to spell
 * @return the canonical lowercase token
 */
auto GF_CORE_EXPORT AppKeyProtectionToString(AppKeyProtection p) -> QString;

/**
 * @brief Read the resolved protection from the "GFAppKeyProtection" property.
 *
 * @return the mode in effect for this process, or kNONE when unset
 */
auto GF_CORE_EXPORT AppKeyProtectionFromApp() -> AppKeyProtection;

/**
 * @brief Apply the installation-mode rule to an already-resolved protection.
 *
 * A portable installation allows only kNONE and kPIN. A portable directory
 * exists to be carried to another computer, and a key wrapped with one
 * machine's credential store cannot be opened anywhere else, so a keychain
 * request is downgraded rather than honoured — even when ENV.ini asked for it,
 * since ENV.ini cannot know where the directory will be plugged in. A PIN
 * travels with the directory and is left alone.
 *
 * @param resolved mode chosen by the settings layers
 * @param portable whether this is a portable installation
 * @return the mode that may actually be used
 */
auto GF_CORE_EXPORT ApplyPortableModeRule(AppKeyProtection resolved,
                                          bool portable) -> AppKeyProtection;

/**
 * @brief Resolve the protection across its layers and the two settings keys it
 * replaced.
 *
 * Pure, taking every layer as a QVariant rather than reading QSettings, so the
 * whole compatibility ladder can be tested without restarting the process. An
 * invalid QVariant means "this layer has no value", matching
 * ResolveLayeredValue() and QSettings::value() for a missing key.
 *
 * The secure_level rungs are what keep a profile written before the split
 * starting: at level 3 its key file is sealed with a PIN, so it has to keep
 * resolving to kPIN until the user chooses otherwise.
 *
 * @param env_protection ENV.ini AppKeyProtection
 * @param env_secure_level ENV.ini SecureLevel
 * @param env_os_secret_store ENV.ini OSSecretStore
 * @param user_protection user advanced/app_key_protection
 * @param user_secure_level user advanced/secure_level
 * @param user_os_secret_store user advanced/os_secret_store
 * @return the winning mode, before the installation-mode rule is applied
 */
auto GF_CORE_EXPORT ResolveAppKeyProtection(
    const QVariant& env_protection, const QVariant& env_secure_level,
    const QVariant& env_os_secret_store, const QVariant& user_protection,
    const QVariant& user_secure_level, const QVariant& user_os_secret_store)
    -> AppKeyProtection;

/**
 * @brief Outcome of reconciling the at-rest protection of the key file.
 */
enum class AppKeyWrapStatus {
  kNOT_WRAPPED,        ///< key file is plaintext and should stay that way
  kWRAPPED,            ///< key file is encrypted and the secret was resolved
  kJUST_ENABLED,       ///< key file was just encrypted for the first time
  kJUST_DISABLED,      ///< key file was just decrypted back to plaintext
  kSTORE_UNAVAILABLE,  ///< protection was requested but no store could be used
  kLOCKED_OUT,  ///< key file is encrypted but the secret is unrecoverable
  kIO_FAILED,   ///< the key file could not be read or rewritten
};

/**
 * @brief Result of AppSecureKeyManager::ResolveWrapSecret().
 */
struct AppKeyWrapResult {
  AppKeyWrapStatus status = AppKeyWrapStatus::kNOT_WRAPPED;

  /// Secret protecting the key file; empty unless it is currently wrapped.
  GFBuffer secret;

  /// Backend name or cause, for the log and any dialog.
  QString detail;

  /// True when startup can proceed, whether or not protection was applied.
  [[nodiscard]] auto Usable() const -> bool {
    return status != AppKeyWrapStatus::kLOCKED_OUT &&
           status != AppKeyWrapStatus::kIO_FAILED;
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
 * There is exactly one secret in play: the **wrap secret**, which encrypts the
 * key file at rest and nothing else. It comes from one of the three backends in
 * AppKeyProtection — nothing, the system credential store, or a PIN the user
 * types at startup — and which one is in use is invisible to everything above.
 *
 * Identity is derived from the plaintext key alone. Keeping it independent of
 * the wrap secret is what makes the protection switchable at all: the key ID is
 * stored as a prefix on every object DataObjectOperator persists, so deriving
 * it from the at-rest protection would orphan every stored object each time
 * that protection changed. The PIN used to feed both, which is exactly why it
 * could not be turned on from the UI; RegisterLegacyKeyIds() keeps the objects
 * such a profile already wrote readable.
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
   * Everything written from now on passes an empty @p pin. A non-empty one
   * reproduces the ID a pre-split profile filed its objects under, back when
   * the PIN was part of the identity; see RegisterLegacyKeyIds().
   *
   * @param pin legacy identity PIN, empty for every current caller
   * @param key key material
   * @return binary key ID
   */
  static auto CalculateKeyId(const GFBuffer& pin, const GFBuffer& key)
      -> GFBuffer;

  /**
   * @brief Register a key under every ID it may be filed under.
   *
   * A profile written before the PIN became a pure wrap secret prefixed its
   * stored objects with CalculateKeyId(pin, key). Everything from now on uses
   * the stable CalculateKeyId({}, key), so both are registered: the old
   * objects stay readable without a rewrite pass, at the cost of one extra map
   * entry, and new objects are written under an ID that no longer moves when
   * the at-rest protection changes.
   *
   * Static and taking the registry by reference so the rule can be asserted
   * directly, without a key file or a live singleton.
   *
   * @param[in,out] keys registry to populate
   * @param pin PIN a pre-split profile derived its IDs from; may be empty
   * @param key key material
   * @return the stable ID, which is the one new objects are written under
   */
  static auto RegisterLegacyKeyIds(QMap<GFBuffer, GFBuffer>& keys,
                                   const GFBuffer& pin, const GFBuffer& key)
      -> GFBuffer;

  /**
   * @brief Reconcile the requested at-rest protection with the key file.
   *
   * Whether the file is currently protected is read from the file itself,
   * which carries the encrypted-container magic; there is deliberately no
   * sidecar marker that could drift out of sync with it. Any transition is
   * performed here, ordered so that an interruption at any point leaves a
   * consistent state: the store entry is written and verified before the file
   * is touched, and removed only after the file no longer needs it.
   *
   * Takes its dependencies explicitly rather than reading the singleton so
   * that tests can drive every path with a temporary directory and a fake
   * store.
   *
   * @param key_path path of the key file
   * @param store credential store to use, or nullptr when none is installed
   * @param intent_enabled whether the user asked for OS-backed protection
   * @return the resolved secret and what, if anything, was changed
   */
  static auto ResolveWrapSecret(const QString& key_path,
                                SystemSecretStore* store, bool intent_enabled)
      -> AppKeyWrapResult;

  /**
   * @brief Re-seal the key file under a different at-rest protection.
   *
   * The plaintext key is supplied by the caller because it is already resident
   * (GetLegacyKey()), so a change never has to open the old container and never
   * depends on the old secret still being readable.
   *
   * Ordering follows ResolveWrapSecret(): the new secret is provisioned and
   * read back before anything depends on it, the new ciphertext is proven to
   * round-trip in memory before it replaces the only copy of the key, the file
   * is replaced in one atomic step, and the secret the old form depended on is
   * released only afterwards. An interruption at any point therefore leaves the
   * key openable by either the old or the new secret, never by neither.
   *
   * Re-sealing with a fresh PIN is a real transition rather than a no-op, which
   * is how a PIN is changed without passing through a plaintext file on disk.
   *
   * Takes its dependencies explicitly rather than reading the singleton so that
   * tests can drive every transition with a temporary directory and a fake
   * store.
   *
   * @param key_path path of the key file
   * @param store credential store to use, or nullptr when none is installed
   * @param plain_key plaintext key material
   * @param from protection currently in effect
   * @param to protection requested
   * @param new_pin PIN to seal with when @p to is kPIN; ignored otherwise
   * @return what happened, with a detail string on failure
   */
  static auto ChangeProtection(const QString& key_path,
                               SystemSecretStore* store,
                               const GFBuffer& plain_key, AppKeyProtection from,
                               AppKeyProtection to, const GFBuffer& new_pin)
      -> AppKeyProtectionResult;

  /**
   * @brief Derive the rotating key for one rotation period.
   *
   * HMAC-SHA256 over the period's salt, keyed by the application secure key.
   * Nothing about the at-rest protection takes part, which is what lets a PIN
   * be set, changed or cleared without orphaning a rotated key.
   *
   * The period is a parameter rather than read from the clock so the schedule
   * can be asserted without waiting a week, and so the derivation is pure.
   *
   * @param app_key application secure key
   * @param period rotation period index, seconds-since-epoch / period length
   * @return key material, or an empty buffer on failure
   */
  static auto DeriveRotatedKey(const GFBuffer& app_key, qint64 period)
      -> GFBuffer;

  /**
   * @brief Encrypt the key for storage on disk.
   *
   * Sealing and unsealing must pick the same key derivation, so both live here
   * rather than at each call site. A PIN is low entropy and gets Argon2id; the
   * credential store's secret is 32 random bytes and gets the much cheaper
   * BLAKE2b derivation, which would otherwise cost ~100ms on every start for
   * no gain. At most one of @p pin and @p wrap is ever set.
   *
   * @param pin identity PIN, set only in high security mode
   * @param wrap credential store secret, set only when OS protection is on
   * @param plain key material to protect
   * @return the bytes to write, which are @p plain itself when neither secret
   * is set, or empty on failure
   */
  static auto SealKey(const GFBuffer& pin, const GFBuffer& wrap,
                      const GFBuffer& plain) -> GFBufferOrNone;

  /**
   * @brief Recover the key from its on-disk form. Inverse of SealKey().
   *
   * @param pin identity PIN, set only in high security mode
   * @param wrap credential store secret, set only when OS protection is on
   * @param stored bytes read from the key file
   * @return the key material, or empty when it does not decrypt
   */
  static auto UnsealKey(const GFBuffer& pin, const GFBuffer& wrap,
                        const GFBuffer& stored) -> GFBufferOrNone;

 private:
  /**
   * @brief Generate a fresh legacy key and persist it.
   *
   * @param wrap wrap secret; when non-empty the file is written encrypted
   * @param[out] status failure detail when the returned buffer is empty
   * @return the plaintext key material, or an empty buffer on failure
   */
  auto new_legacy_key(const GFBuffer& pin, const GFBuffer& wrap,
                      AppSecureKeyInitResult& status) -> GFBuffer;

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
   * @brief Derive and persist the weekly rotating key. Secure level 3 only.
   *
   * Derived from the application secure key rather than from a PIN, so that
   * rotation is independent of how — or whether — the key file is protected at
   * rest. Setting, changing or clearing a PIN must never orphan a rotated key.
   *
   * Also sets the active key ID, since at this level new objects are written
   * under the rotated key rather than the legacy one.
   *
   * @param app_key application secure key to derive from
   * @return key material, or an empty buffer on failure
   */
  auto fetch_time_related_key(const GFBuffer& app_key) -> GFBuffer;

  QMap<GFBuffer, GFBuffer> keys_;  ///< key ID to key material
  GFBuffer active_key_id_;         ///< ID of the key used for new objects
  GFBuffer legacy_key_id_;         ///< ID of the legacy key
};

}  // namespace GpgFrontend
