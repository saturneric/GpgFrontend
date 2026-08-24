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

#include <functional>

#include "core/function/SystemSecretStore.h"
#include "core/model/GFBuffer.h"
#include "core/profile/Profile.h"

namespace GpgFrontend {

/**
 * @brief Outcome of ProfileSecureKeyManager::Load().
 *
 * The manager never shows UI of its own — gf_core does not link QtWidgets — so
 * every failure is reported here and rendered by the application layer.
 */
enum class ProfileKeyLoadStatus : std::uint8_t {
  kOK,               ///< key set loaded or created successfully
  kREAD_FAILED,      ///< a key file exists but could not be read
  kDECRYPT_FAILED,   ///< a key file was read but would not decrypt
  kWRITE_FAILED,     ///< a newly generated key could not be persisted
  kGENERATE_FAILED,  ///< no usable random source produced a key
};

/**
 * @brief Result of loading a profile's key set.
 */
struct GF_CORE_EXPORT ProfileKeyLoadResult {
  ProfileKeyLoadStatus status = ProfileKeyLoadStatus::kOK;

  /// Path, cause, or other context worth showing the user and logging.
  QString detail;

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfileKeyLoadStatus::kOK;
  }
};

/**
 * @brief How many keys are in play, and why.
 */
enum class ProfileKeyMode : std::uint8_t {
  /**
   * @brief One active key. The classical case.
   *
   * A profile written before the PIN became a pure wrap secret also has its
   * old, PIN-derived id registered, so objects filed under it stay readable —
   * that is an alias for the same material, not a second key.
   */
  kSINGLE,

  /**
   * @brief An active key that changes on a schedule, plus the keys that open
   * what earlier periods wrote.
   *
   * New objects are encrypted with the current period's key; every key still
   * needed to read older material is listed alongside it. Rotation is derived
   * from the profile's own key rather than from whatever protects it at rest,
   * so setting, changing or clearing a PIN never orphans a rotated key.
   */
  kROTATING,
};

/**
 * @brief Which credential-store entry protects one profile's key file.
 *
 * There used to be exactly one account name for the whole application, which
 * works only while there is exactly one profile. With several, they all reach
 * for the same entry and whichever opens last overwrites the secret the others
 * depend on.
 *
 * A profile id alone is not enough of a namespace either. Ids are short and a
 * profile can be deleted and recreated under the same one, so the name binds
 * the id to the root it lives in *and* to a uuid minted once at creation, which
 * is what makes a recreated profile a genuinely different identity.
 *
 * The installed root keeps the original account verbatim. Its entry already
 * exists on every current installation, and renaming it would lock every one of
 * those users out of their own data objects.
 *
 * @param kind which shape of profile this is
 * @param profile_id the profile id
 * @param canonical_root absolute, canonicalised profile root
 * @param profile_uuid the uuid from profile.json; may be empty on a first run,
 * in which case only the root distinguishes the entry
 * @return the account name to use with SystemSecretStore
 */
auto GF_CORE_EXPORT DeriveAppKeyWrapAccount(ProfileKind kind,
                                            const QString& profile_id,
                                            const QString& canonical_root,
                                            const QString& profile_uuid)
    -> QString;

/**
 * @brief How the application key file is protected at rest.
 *
 * All three are pure at-rest backends: none of them takes part in deriving a
 * key identity, so switching between them never changes a key ID and never
 * orphans a stored data object.
 */
enum class AppKeyProtection : std::uint8_t {
  kNONE,      ///< key file is stored as plaintext
  kKEYCHAIN,  ///< encrypted with a random secret in the system credential store
  kPIN,       ///< encrypted with a PIN the user types at startup
};

/**
 * @brief Outcome of ProfileSecureKeyManager::ChangeProtection().
 */
enum class AppKeyProtectionStatus : std::uint8_t {
  kOK,                 ///< the file now carries the requested protection
  kUNCHANGED,          ///< the requested protection was already in effect
  kBAD_PIN,            ///< kPIN was requested without a usable PIN
  kSTORE_UNAVAILABLE,  ///< kKEYCHAIN was requested but no store could be used
  kSEAL_FAILED,        ///< the key could not be encrypted for its new form
  kIO_FAILED,          ///< the key file could not be rewritten
};

/**
 * @brief Result of ProfileSecureKeyManager::ChangeProtection().
 */
struct GF_CORE_EXPORT AppKeyProtectionResult {
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
 * a marker should leave the key unprotected, not demand a PIN nobody ever set.
 *
 * @param s spelling from the profile marker or the settings store
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
 * @brief Outcome of reconciling the at-rest protection of the key file.
 */
enum class AppKeyWrapStatus : std::uint8_t {
  kNOT_WRAPPED,        ///< key file is plaintext and should stay that way
  kWRAPPED,            ///< key file is encrypted and the secret was resolved
  kJUST_ENABLED,       ///< key file was just encrypted for the first time
  kJUST_DISABLED,      ///< key file was just decrypted back to plaintext
  kSTORE_UNAVAILABLE,  ///< protection was requested but no store could be used
  kLOCKED_OUT,  ///< key file is encrypted but the secret is unrecoverable
  kIO_FAILED,   ///< the key file could not be read or rewritten
};

/**
 * @brief Result of ProfileSecureKeyManager::ResolveWrapSecret().
 */
struct GF_CORE_EXPORT AppKeyWrapResult {
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
 * @brief The key set of one profile session.
 *
 * This is the single owner of the key material that protects everything
 * DataObjectOperator persists, and it belongs to the session rather than to the
 * process: a profile's keys have no meaning outside the profile they came from,
 * and one process only ever runs one profile.
 *
 * There is exactly one secret in play at rest: the **wrap secret**, which
 * encrypts the key file and nothing else. It comes from one of the three
 * backends in AppKeyProtection — nothing, the system credential store, or a PIN
 * the user types at startup — and which one is in use is invisible to
 * everything above. Deciding *which* is the loader's job; this stores and opens
 * the files.
 *
 * Identity is derived from the plaintext key alone. Keeping it independent of
 * the wrap secret is what makes the protection switchable at all: the key ID is
 * stored as a prefix on every object DataObjectOperator persists, so deriving
 * it from the at-rest protection would orphan every stored object each time
 * that protection changed. The PIN used to feed both, which is exactly why it
 * could not be turned on from the UI; RegisterKeyIds() keeps the objects such a
 * profile already wrote readable.
 */
/**
 * @brief Where a re-sealed application key is written, and what to call there.
 *
 * A path is not enough. A driver may hold the secure area in memory, where
 * there is no file to write and nothing to name -- a legitimate answer rather
 * than a failure -- so a re-protection is handed somewhere to put the key
 * instead of a place on a filesystem to put it.
 */
struct GF_CORE_EXPORT AppKeySink {
  /// Replaces the stored key in one step. False when it could not.
  std::function<bool(const GFBuffer&)> write;

  /// Where the key is, in words fit for a log line or an error detail.
  QString location;
};

/**
 * @brief A sink that rewrites the key file at @p path.
 *
 * For a key addressed as a file in its own right rather than through a
 * session's storage.
 *
 * @param path the key file to replace
 * @return a sink writing there, atomically
 */
auto GF_CORE_EXPORT AppKeySinkForFile(const QString& path) -> AppKeySink;

class GF_CORE_EXPORT ProfileSecureKeyManager {
 public:
  /**
   * @brief Construct the key set against one profile's storage.
   *
   * @param accessor the session's storage driver
   */
  explicit ProfileSecureKeyManager(QSharedPointer<ProfileAccessor> accessor);

  ~ProfileSecureKeyManager() = default;

  ProfileSecureKeyManager(const ProfileSecureKeyManager&) = delete;
  auto operator=(const ProfileSecureKeyManager&)
      -> ProfileSecureKeyManager& = delete;
  ProfileSecureKeyManager(ProfileSecureKeyManager&&) = delete;
  auto operator=(ProfileSecureKeyManager&&)
      -> ProfileSecureKeyManager& = delete;

  /**
   * @brief Load the key set from storage, creating it when absent.
   *
   * Must run before DataObjectOperator is constructed, since that caches the
   * active and root keys at construction time.
   *
   * @param pin identity PIN; empty for everything written since the split
   * @param wrap secret used only to encrypt the key file at rest; empty when
   * the key is stored unprotected
   * @param rotating whether this session rotates its active key, which the
   * profile has to allow and the security level has to ask for
   * @return outcome, with a detail string on failure
   */
  auto Load(const GFBuffer& pin, const GFBuffer& wrap, bool rotating)
      -> ProfileKeyLoadResult;

  /**
   * @brief Whether one key is in play or a rotating set.
   *
   * @return the mode this session loaded in
   */
  [[nodiscard]] auto Mode() const -> ProfileKeyMode { return mode_; }

  /**
   * @brief The key new objects are encrypted with.
   *
   * @return key material for the active key
   */
  [[nodiscard]] auto ActiveKey() const -> GFBuffer;

  /**
   * @brief The ID of the active key.
   *
   * @return binary key ID
   */
  [[nodiscard]] auto ActiveKeyId() const -> GFBuffer;

  /**
   * @brief The profile's own key, from which everything else is derived.
   *
   * @return key material
   */
  [[nodiscard]] auto RootKey() const -> GFBuffer;

  /**
   * @brief Look up a key by its ID.
   *
   * In kROTATING mode this is how an object written in an earlier period is
   * still opened: the keys that decrypt old material are listed here alongside
   * the one that encrypts new material.
   *
   * @param id binary key ID
   * @return key material, or an empty buffer when the ID is unknown
   */
  [[nodiscard]] auto KeyById(const GFBuffer& id) const -> GFBuffer;

  /**
   * @brief The path of the profile's key file.
   *
   * @return absolute path to secure/app.key
   */
  [[nodiscard]] auto KeyPath() const -> QString;

  /**
   * @brief Where the key is, in words fit for a log line or an error.
   *
   * KeyPath() is empty when the driver holds the secure area in memory, which
   * is a legitimate answer rather than a failure. Everything that only wants to
   * *say* where the key is should ask for this instead, so a report never comes
   * out with a blank where a location should be.
   *
   * @return the key file's path; where there is none, that the key is held in
   * memory, or failing that the storage's name
   */
  [[nodiscard]] auto KeyLocationForMessage() const -> QString;

  /**
   * @brief Delete every on-disk key file, for a destructive reset to default.
   *
   * Removes app.key along with any rotated <keyId>.key files derived from it,
   * since those are keyed to the key being discarded and would only be orphaned
   * by the reset. A fresh key is regenerated on the next Load().
   *
   * This is the only reversal of a forgotten PIN or an unrecoverable keychain
   * secret: everything the old key encrypted becomes permanently unreadable, so
   * the callers gate it behind an explicit, confirmed user choice. The store
   * entry and the protection preference are cleared by the caller, which owns
   * both; this handles only the files under @p key_dir.
   *
   * Static and taking the secure directory explicitly rather than reading a
   * session, so a test can drive it against a temporary directory without
   * disturbing the running process's own key.
   *
   * @param key_dir directory holding the key files, i.e. KeyDir()
   * @return false if app.key existed but could not be removed; true otherwise
   */
  [[nodiscard]] static auto ResetKeyStorage(const QString& key_dir) -> bool;

  /**
   * @brief Derive the identity of a key.
   *
   * HMAC-SHA256 over @p key using @p pin as the HMAC key, falling back to a
   * fixed label when @p pin is empty. Static so tests can assert that an ID is
   * stable across changes to at-rest protection.
   *
   * Everything written from now on passes an empty @p pin. A non-empty one
   * reproduces the ID a pre-split profile filed its objects under, back when
   * the PIN was part of the identity; see RegisterKeyIds().
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
   * directly, without a key file or a live session.
   *
   * @param[in,out] keys registry to populate
   * @param pin PIN a pre-split profile derived its IDs from; may be empty
   * @param key key material
   * @return the stable ID, which is the one new objects are written under
   */
  static auto RegisterKeyIds(QMap<GFBuffer, GFBuffer>& keys,
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
   * Takes its dependencies explicitly rather than reading a session so that
   * tests can drive every path with a temporary directory and a fake store.
   *
   * @param key_path path of the key file
   * @param store credential store to use, or nullptr when none is installed
   * @param intent_enabled whether the user asked for OS-backed protection
   * @param account credential-store account to use
   * @return the resolved secret and what, if anything, was changed
   */
  static auto ResolveWrapSecret(
      const QString& key_path, SystemSecretStore* store, bool intent_enabled,
      const QString& account = QString::fromLatin1(kAppKeyWrapAccount))
      -> AppKeyWrapResult;

  /**
   * @brief Re-seal the key file under a different at-rest protection.
   *
   * The plaintext key is supplied by the caller because it is already resident
   * (RootKey()), so a change never has to open the old container and never
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
   * @param sink where the re-sealed key goes: KeySink() for a live session,
   * AppKeySinkForFile() for a key file addressed in its own right
   * @param store credential store to use, or nullptr when none is installed
   * @param plain_key plaintext key material
   * @param from protection currently in effect
   * @param to protection requested
   * @param new_pin PIN to seal with when @p to is kPIN; ignored otherwise
   * @param account credential-store account to use
   * @return what happened, with a detail string on failure
   */
  static auto ChangeProtection(
      const AppKeySink& sink, SystemSecretStore* store,
      const GFBuffer& plain_key, AppKeyProtection from, AppKeyProtection to,
      const GFBuffer& new_pin,
      const QString& account = QString::fromLatin1(kAppKeyWrapAccount))
      -> AppKeyProtectionResult;

  /**
   * @brief A sink that writes this session's key through its own storage.
   *
   * The reason ChangeProtection() takes a sink at all. A driver may hold the
   * secure area in memory, where KeyPath() is empty and rightly so -- and a
   * change that wrote by path would either fail outright or, handed a path,
   * write the key onto the disk the session arranged to keep it off.
   *
   * Keeps a reference to this manager's storage, so it must not outlive it.
   *
   * @return a sink addressing this session's application key
   */
  [[nodiscard]] auto KeySink() const -> AppKeySink;

  /**
   * @brief The stored application key, in whatever form it is sealed.
   *
   * Read through the storage rather than from a path, for the same reason
   * KeySink() writes through it. Verifying a PIN is the caller this exists for:
   * a session holding the area in memory has no file to read back, and reading
   * an empty path fails in a way that is indistinguishable from a wrong PIN.
   *
   * @return the sealed key, or nothing when there is none to read
   */
  [[nodiscard]] auto ReadStoredKey() const -> GFBufferOrNone;

  /**
   * @brief Derive the rotating key for one rotation period.
   *
   * HMAC-SHA256 over the period's salt, keyed by the profile's key. Nothing
   * about the at-rest protection takes part, which is what lets a PIN be set,
   * changed or cleared without orphaning a rotated key.
   *
   * The period is a parameter rather than read from the clock so the schedule
   * can be asserted without waiting a week, and so the derivation is pure.
   *
   * @param app_key the profile's key
   * @param period rotation period index, seconds-since-epoch / period length
   * @return key material, or an empty buffer on failure
   */
  static auto DeriveRotatedKey(const GFBuffer& app_key, qint64 period)
      -> GFBuffer;

  /**
   * @brief Encrypt the key for storage.
   *
   * Sealing and unsealing must pick the same key derivation, so both live here
   * rather than at each call site. A PIN is low entropy and gets Argon2id; the
   * credential store's secret is 32 random bytes and gets the much cheaper
   * BLAKE2b derivation, which would otherwise cost ~100ms on every start for
   * no gain. At most one of @p pin and @p wrap is ever set.
   *
   * @param pin identity PIN, set only when a PIN protects the file
   * @param wrap credential store secret, set only when OS protection is on
   * @param plain key material to protect
   * @return the bytes to write, which are @p plain itself when neither secret
   * is set, or empty on failure
   */
  static auto SealKey(const GFBuffer& pin, const GFBuffer& wrap,
                      const GFBuffer& plain) -> GFBufferOrNone;

  /**
   * @brief Recover the key from its stored form. Inverse of SealKey().
   *
   * @param pin identity PIN, set only when a PIN protects the file
   * @param wrap credential store secret, set only when OS protection is on
   * @param stored bytes read from the key file
   * @return the key material, or empty when it does not decrypt
   */
  static auto UnsealKey(const GFBuffer& pin, const GFBuffer& wrap,
                        const GFBuffer& stored) -> GFBufferOrNone;

 private:
  /**
   * @brief Generate a fresh key and persist it.
   *
   * @param pin identity PIN
   * @param wrap wrap secret; when non-empty the file is written encrypted
   * @param[out] status failure detail when the returned buffer is empty
   * @return the plaintext key material, or an empty buffer on failure
   */
  auto new_root_key(const GFBuffer& pin, const GFBuffer& wrap,
                    ProfileKeyLoadResult& status) -> GFBuffer;

  /**
   * @brief Load or create the profile's key and register it as active.
   *
   * @param pin identity PIN
   * @param wrap wrap secret
   * @return outcome
   */
  auto init_root_key(const GFBuffer& pin, const GFBuffer& wrap)
      -> ProfileKeyLoadResult;

  /**
   * @brief Derive and persist this period's rotating key.
   *
   * Derived from the profile's key rather than from a PIN, so that rotation is
   * independent of how — or whether — the key file is protected at rest.
   * Setting, changing or clearing a PIN must never orphan a rotated key.
   *
   * Also sets the active key ID, since in this mode new objects are written
   * under the rotated key rather than the root one.
   *
   * @param app_key the profile's key to derive from
   * @return key material, or an empty buffer on failure
   */
  auto fetch_time_related_key(const GFBuffer& app_key) -> GFBuffer;

  QSharedPointer<ProfileAccessor> accessor_;  ///< where the key files live
  QMap<GFBuffer, GFBuffer> keys_;             ///< key ID to key material
  GFBuffer active_key_id_;                    ///< ID used for new objects
  GFBuffer root_key_id_;                      ///< ID of the profile's own key
  ProfileKeyMode mode_ = ProfileKeyMode::kSINGLE;
};

}  // namespace GpgFrontend
