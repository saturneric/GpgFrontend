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

#include "core/profile/ProfileLock.h"
#include "core/profile/ProfileMigration.h"
#include "core/profile/ProfileSession.h"

namespace GpgFrontend {

/**
 * @brief Why the profile could not be opened.
 *
 * Every one of these stops startup. They are a closed set rather than free text
 * so the application layer can decide how to say each one, and so a test can
 * assert which one happened without matching on a sentence.
 */
enum class ProfileLoadFailure : std::uint8_t {
  kSELECTION_INVALID,  ///< the command line named something unusable
  kALREADY_OPEN,       ///< another process holds this profile
  kLOCK_UNAVAILABLE,   ///< the lock file could not be created
  kMOUNT_FAILED,       ///< the storage could not be made usable
  /// Nothing was available that this profile's storage policy would accept.
  /// Not a failure of the machine or the package: the user asked not to open it
  /// anywhere it would be left readable, and nowhere qualified.
  kSTORAGE_UNAVAILABLE,
  kSTORAGE_FULL,  ///< the storage ran out of room part way through unpacking
  kPACKAGE_TOO_LARGE,    ///< the file is larger than this build can open
  kNOT_A_PACKAGE,        ///< the file named is not a .gfp
  kPACKAGE_TAMPERED,     ///< header and sealed manifest disagree
  kPACKAGE_MALFORMED,    ///< the payload is not a profile tree
  kTOO_NEW,              ///< written by a newer build; must not be touched
  kUPGRADE_REFUSED,      ///< cannot be upgraded by this build
  kUPGRADE_FAILED,       ///< a migration rung failed part way
  kCANCELLED,            ///< the user declined a prompt
  kKEY_READ_FAILED,      ///< a key file exists but could not be read
  kKEY_DECRYPT_FAILED,   ///< a key file was read but would not decrypt
  kKEY_WRITE_FAILED,     ///< a new key could not be persisted
  kKEY_GENERATE_FAILED,  ///< no usable random source produced a key
  kKEY_LOCKED_OUT,       ///< the secret that opens the key is unrecoverable
  kKEY_IO_FAILED,        ///< the key file could not be read or rewritten
};

/**
 * @brief Something worth telling the user that does not stop startup.
 */
enum class ProfileLoadNotice : std::uint8_t {
  /// Keychain protection was asked for and is not available here; the
  /// preference has been turned off and the key is left unprotected.
  kKEYCHAIN_UNAVAILABLE,

  /// A PIN is configured but the key file on disk is plaintext — an
  /// interrupted transition. The setting has been turned off and the key left
  /// exactly as it is.
  kPIN_SET_BUT_KEY_PLAINTEXT,

  /// The key was discarded at the user's request. Everything it encrypted is
  /// now permanently unreadable.
  kKEY_WAS_RESET,
};

/**
 * @brief A failure, with everything needed to describe it.
 */
struct GF_CORE_EXPORT ProfileLoadError {
  ProfileLoadFailure failure = ProfileLoadFailure::kMOUNT_FAILED;

  /// What it is about: the profile root, or the package path.
  QString subject;

  /// The cause, in the words of whatever produced it.
  QString detail;

  /// The version or process responsible, when there is one.
  QString actor;
};

/**
 * @brief Why the loader is offering to discard the key.
 */
enum class ProfileKeyResetReason : std::uint8_t {
  kKEYCHAIN_SECRET_LOST,  ///< the credential store will not give it back
  kPIN_FORGOTTEN,         ///< the user asked, having run out of guesses
};

/**
 * @brief What the loader needs a PIN for.
 */
struct GF_CORE_EXPORT AppKeyPinRequest {
  /// True when there is no key file yet, so the PIN being asked for is one the
  /// user is choosing rather than recalling.
  bool creating = false;

  /// How many PINs have already been tried and rejected this start.
  int failures = 0;

  QString key_path;
};

/**
 * @brief The answer to AppKeyPinRequest.
 *
 * The empty-PIN case is deliberately not overloaded to mean "quit": a reset key
 * file legitimately continues with no PIN, and a cancel must not.
 */
struct GF_CORE_EXPORT AppKeyPinAnswer {
  enum class Action : std::uint8_t {
    kQuit,      ///< the user backed out; startup stops
    kUsePin,    ///< try this PIN
    kResetKey,  ///< discard the key, already confirmed with the user
  };

  Action action = Action::kQuit;
  GFBuffer pin;
};

/**
 * @brief Everything the loader needs a human for.
 *
 * gf_core does not link QtWidgets, and it should not: the load sequence is a
 * decision procedure, and which dialog renders a decision is not part of it.
 * Implement this in the application layer, or with a scripted fake in a test —
 * which is the whole point, since startup is otherwise only exercisable by
 * starting the application.
 */
class GF_CORE_EXPORT ProfileLoaderDelegate {
 public:
  ProfileLoaderDelegate() = default;
  virtual ~ProfileLoaderDelegate() = default;

  ProfileLoaderDelegate(const ProfileLoaderDelegate &) = delete;
  auto operator=(const ProfileLoaderDelegate &)
      -> ProfileLoaderDelegate & = delete;
  ProfileLoaderDelegate(ProfileLoaderDelegate &&) = delete;
  auto operator=(ProfileLoaderDelegate &&) -> ProfileLoaderDelegate & = delete;

  /**
   * @brief Ask for the passphrase that opens a package.
   *
   * @param package the file being opened
   * @param retry true when a previous answer did not open it
   * @return the passphrase, or nothing to give up
   */
  virtual auto AskPackagePassphrase(const QString &package, bool retry)
      -> std::optional<GFBuffer> = 0;

  /**
   * @brief Ask for the PIN that opens the profile's key file.
   *
   * @param request what is being asked and why
   * @return what to do
   */
  virtual auto AskAppKeyPin(const AppKeyPinRequest &request)
      -> AppKeyPinAnswer = 0;

  /**
   * @brief Ask whether to break another process's claim on this profile.
   *
   * Destructive by nature: if the holder is in fact alive, agreeing
   * reintroduces exactly the concurrent-write window the lock exists to
   * prevent.
   *
   * @param held who holds it, as far as the lock file knows
   * @return true to force the lock open
   */
  virtual auto ConfirmForceUnlock(const ProfileLockResult &held) -> bool = 0;

  /**
   * @brief Ask whether to discard the key that cannot be opened.
   *
   * Everything it encrypted becomes permanently unreadable, so a delegate that
   * cannot ask a human must answer false.
   *
   * @param reason why the key cannot be opened
   * @return true to reset
   */
  virtual auto ConfirmKeyReset(ProfileKeyResetReason reason) -> bool = 0;

  /**
   * @brief Report the failure that is about to stop startup.
   *
   * @param error what went wrong
   */
  virtual void Report(const ProfileLoadError &error) = 0;

  /**
   * @brief Mention something the user should know but need not act on.
   *
   * @param notice what happened
   * @param detail backend name, path, or cause
   */
  virtual void Note(ProfileLoadNotice notice, const QString &detail) = 0;
};

/**
 * @brief Opens a profile and produces the session that runs it.
 *
 * The single entry point into the profile system. It owns the whole sequence —
 * the lock, extraction, the compatibility gate, the migrations, the profile's
 * identity, the at-rest protection ladder, the secret, and the key set — so
 * that the order those happen in is stated once, here, rather than
 * reconstructed from the order of calls in main().
 *
 * Two phases, because the application's startup genuinely has two:
 *
 *  - @ref Mount makes the storage reachable and publishes the session. After
 *    it, settings and paths are live, which is what the log sink and
 *    `--environment` need — and none of them needs a key.
 *  - @ref Open resolves the secret and loads the key set. After it,
 *    ProfileSession::Keys() answers.
 *
 * Anything that needs a human goes through the delegate. Nothing here shows a
 * dialog, reads argv, or decides how a failure should be phrased.
 */
class GF_CORE_EXPORT ProfileLoader {
 public:
  /**
   * @brief Prepare to load a profile.
   *
   * @param profile what to open
   * @param delegate who to ask; must outlive the loader
   */
  ProfileLoader(QSharedPointer<Profile> profile,
                ProfileLoaderDelegate *delegate);

  ~ProfileLoader() = default;

  ProfileLoader(const ProfileLoader &) = delete;
  auto operator=(const ProfileLoader &) -> ProfileLoader & = delete;
  ProfileLoader(ProfileLoader &&) = delete;
  auto operator=(ProfileLoader &&) -> ProfileLoader & = delete;

  /**
   * @brief Take the lock, make the storage reachable, publish the session.
   *
   * The lock comes first, before anything writes — including before a package
   * is extracted. Two processes on one root corrupt data_objs/, where every
   * write is a whole-file read-modify-write with no locking of its own, let the
   * garbage collector quarantine objects the other process is writing, and
   * leave the GnuPG home directory unprotected, since SQLite protects only
   * itself.
   *
   * @param schema_version the layout version this build speaks
   * @return false when the application must stop; the delegate has been told
   * why
   */
  auto Mount(int schema_version) -> bool;

  /**
   * @brief Run the compatibility gate and the migrations, then open the keys.
   *
   * @param schema_version the layout version this build speaks
   * @param rotation_requested whether the security level asks for a rotating
   * key; the profile still has the last word through AllowsKeyRotation()
   * @return false when the application must stop
   */
  auto Open(int schema_version, bool rotation_requested) -> bool;

  /**
   * @brief The credential-store account of the profile now running.
   *
   * Resolves DeriveAppKeyWrapAccount() against the live session and the uuid in
   * its marker, so every caller names the same entry without each having to
   * reassemble the rule.
   *
   * @return the account name, or empty when it cannot be derived
   */
  static auto CurrentWrapAccount() -> QString;

  /**
   * @brief Read the resolved protection from the "GFAppKeyProtection" property.
   *
   * @return the mode in effect for this process, or kNONE when unset
   */
  static auto AppKeyProtectionFromApp() -> AppKeyProtection;

  /**
   * @brief Write the protection in effect into the "GFAppKeyProtection"
   * property.
   *
   * Startup can end up with a protection the settings asked for but the key
   * file does not have -- an unavailable credential store, a reset key, a PIN
   * setting over a plaintext file. The stored setting is turned back off in
   * each of those cases, and the property has to follow it: everything that
   * asks what is in effect this run reads the property, so one left saying
   * "keychain" over an unprotected key file shows the user a protection they do
   * not have.
   *
   * @param protection the mode actually in effect for this process
   */
  static void SetAppKeyProtectionInApp(AppKeyProtection protection);

  /**
   * @brief Refuse the credential store for any profile that leaves this
   * machine.
   *
   * A profile inside a `.gfp` package, or on removable media, is written
   * expressly to be carried somewhere else — very possibly to a different
   * operating system, where "the system credential store" is not the same thing
   * and often is not anything at all. A key sealed with one machine's store
   * simply cannot be opened there, so the request is downgraded rather than
   * honoured and the profile is left openable.
   *
   * That leaves exactly two protections for a profile that travels: a PIN,
   * which travels with it, or none at all. Inside a package both are
   * additionally covered by the package's own passphrase.
   *
   * @param resolved mode chosen by the settings layers
   * @param keychain_allowed what the profile says, i.e.
   * Profile::AllowsSystemKeychain()
   * @return the mode that may actually be used
   */
  static auto ApplyProfilePortabilityRule(AppKeyProtection resolved,
                                          bool keychain_allowed)
      -> AppKeyProtection;

  /**
   * @brief Resolve the protection across its layers and the two settings keys
   * it replaced.
   *
   * Pure, taking every layer as a QVariant rather than reading QSettings, so
   * the whole compatibility ladder can be tested without restarting the
   * process. An invalid QVariant means "this layer has no value", matching
   * ResolveLayeredValue() and QSettings::value() for a missing key.
   *
   * The secure_level rungs are what keep a profile written before the split
   * starting: at level 3 its key file is sealed with a PIN, so it has to keep
   * resolving to kPIN until the user chooses otherwise.
   *
   * @param env_protection pinned AppKeyProtection
   * @param env_secure_level pinned SecureLevel
   * @param env_os_secret_store pinned OSSecretStore
   * @param user_protection user advanced/app_key_protection
   * @param user_secure_level user advanced/secure_level
   * @param user_os_secret_store user advanced/os_secret_store
   * @return the winning mode, before the portability rule is applied
   */
  static auto ResolveAppKeyProtection(const QVariant &env_protection,
                                      const QVariant &env_secure_level,
                                      const QVariant &env_os_secret_store,
                                      const QVariant &user_protection,
                                      const QVariant &user_secure_level,
                                      const QVariant &user_os_secret_store)
      -> AppKeyProtection;

 private:
  /// Acquire the lock, asking the delegate whether to force it open.
  auto acquire_lock() -> bool;

  /// Mount a packaged profile, prompting for its passphrase until it opens.
  /// Returns the failure rather than reporting it, so the caller can give the
  /// storage back before a dialog stops to wait for a human.
  auto mount_package(int schema_version) -> std::optional<ProfileLoadError>;

  /// Refuse a profile this build must not touch, and run one stage of the
  /// ladder.
  auto run_migrations(int schema_version, ProfileMigrationStage stage) -> bool;

  /// Give the profile a uuid before anything derives an identity from it.
  void ensure_identity();

  /// Record that this build opened the profile, once it really has.
  void stamp_marker();

  /// Resolve the secret that opens the key file, prompting when it is a PIN.
  auto resolve_secret(AppKeyProtection protection, GFBuffer &pin,
                      GFBuffer &wrap) -> bool;

  /// Discard the key material and drop the protection back to none.
  auto reset_key_storage() -> bool;

  /// Record that the key file is unprotected this run, in the stored setting
  /// and in the property that says what is in effect.
  void turn_protection_off();

  QSharedPointer<Profile> profile_;
  ProfileLoaderDelegate *delegate_;
  QSharedPointer<ProfileSession> session_;
};

}  // namespace GpgFrontend
