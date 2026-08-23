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

#include "core/profile/ProfileAccessor.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfilePackage.h"

namespace GpgFrontend {

/**
 * @brief Which shape of profile a process is running against.
 *
 * The four shapes are the leaves of the class hierarchy below, and the enum
 * exists only where a class pointer cannot go: a stored token in profile.json,
 * a registry entry, a qApp property for the module SDK.
 *
 * The stored spellings are deliberately unchanged from the ones this
 * application has always written — see ProfileKindToString().
 */
enum class ProfileKind : std::uint8_t {
  kINSTALLED_ROOT,  ///< the OS user-data location; the original layout
  kPORTABLE_ROOT,  ///< the directory above the application, on a portable build
  kPERSIST,        ///< <root-profile>/profiles/<uuid>
  kPACKAGED,       ///< extracted from a .gfp, and temporary
};

/**
 * @brief Canonical stored spelling of a kind.
 *
 * These are a wire format: `classic`, `portable`, `named` and `package-linked`
 * are what every existing profile.json already contains, and renaming them
 * would make this build unable to recognise its own profiles. The C++ names
 * moved; the tokens did not.
 *
 * @param kind kind to spell
 * @return the canonical lowercase token
 */
auto GF_CORE_EXPORT ProfileKindToString(ProfileKind kind) -> QString;

/**
 * @brief Parse the stored spelling of a kind.
 *
 * Anything unrecognised reads as kINSTALLED_ROOT, which is the shape every
 * installation had before profiles existed.
 *
 * @param s stored token
 * @return the parsed kind
 */
auto GF_CORE_EXPORT ProfileKindFromString(const QString &s) -> ProfileKind;

/**
 * @brief Behavioural choices that travel with a profile.
 *
 * This is portable mode's other half. Portable mode was doing two unrelated
 * jobs at once — choosing a location and choosing a set of behaviours — and
 * only the first is a profile root. The second is this.
 */
struct GF_CORE_EXPORT ProfilePolicy {
  /**
   * @brief Never reach outside this root for keys, never bind to this machine.
   *
   * Makes the default key database `@profile/db` instead of whatever
   * `gpgconf --list-dirs homedir` reports, stores new database paths relative
   * to the profile, and downgrades credential-store protection of the app key.
   *
   * It deliberately does *not* change how the gpg binary is found: a portable
   * installation still runs a system or bundled gpg. "Do not ask gpgconf where
   * the keyring is" is not "do not use gpgconf".
   */
  bool self_contained = false;
};

/**
 * @brief Why a profile could not be made usable.
 */
enum class ProfileMountStatus : std::uint8_t {
  kOK,
  kNEEDS_PASSPHRASE,  ///< packaged and protected; ask, then mount again
  kBAD_PASSPHRASE,
  kNOT_A_PACKAGE,
  kTOO_NEW,    ///< a layout this build must not touch
  kTAMPERED,   ///< the package header and its sealed manifest disagree
  kMALFORMED,  ///< the payload is not a profile tree
  /// No storage was available that this profile's policy would accept. Not an
  /// I/O failure: nothing was wrong with the package or the machine, the user
  /// asked not to open it anywhere it would be left readable.
  kNO_PROTECTED_STORAGE,
  kNO_SPACE,   ///< the storage filled up part way through unpacking
  kTOO_LARGE,  ///< the file is larger than this build can open at all
  kIO_FAILED,
};

/**
 * @brief Outcome of Profile::Mount().
 */
struct GF_CORE_EXPORT ProfileMountResult {
  ProfileMountStatus status = ProfileMountStatus::kOK;
  QString detail;

  [[nodiscard]] auto Ok() const -> bool {
    return status == ProfileMountStatus::kOK;
  }
};

/**
 * @brief What Mount() is given by the loader.
 */
struct GF_CORE_EXPORT ProfileMountContext {
  /// The passphrase for a packaged profile; ignored by every other shape.
  GFBuffer passphrase;

  /// The layout version this build speaks, so a package from the future is
  /// refused before it is written anywhere.
  int schema_version = 0;
};

/**
 * @brief How a profile is given back.
 */
enum class ProfileUnmountMode : std::uint8_t {
  kNORMAL,  ///< an orderly shutdown
  kFORCED,  ///< the watchdog is about to _Exit; do the minimum, quickly
};

/**
 * @brief One profile: where the application's data is, and the rules that
 * follow from that.
 *
 * The hierarchy exists to hold the rules. Almost every question this answers
 * used to be a comparison against a kind enum somewhere else in the codebase —
 * in the settings station, in the key manager, in the launcher — and each one
 * was a place where a new shape of profile could be forgotten.
 *
 * A profile knows its root before it is mounted. That is what lets the loader
 * take the profile lock *before* anything writes, including before a package is
 * extracted: a packaged profile's session root is derived from the package's
 * own path rather than minted at random.
 */
class GF_CORE_EXPORT Profile {
 public:
  Profile() = default;
  virtual ~Profile() = default;

  Profile(const Profile &) = delete;
  auto operator=(const Profile &) -> Profile & = delete;
  Profile(Profile &&) = delete;
  auto operator=(Profile &&) -> Profile & = delete;

  [[nodiscard]] virtual auto Kind() const -> ProfileKind = 0;

  /// The identity that names this profile: a uuid, or "classic"/"portable".
  [[nodiscard]] virtual auto Id() const -> QString = 0;

  /// Absolute root of the AppData tree. Known before Mount().
  [[nodiscard]] virtual auto Root() const -> QString = 0;

  /// What to call it in the interface; the id when there is nothing better.
  [[nodiscard]] virtual auto DisplayName() const -> QString;

  [[nodiscard]] auto Policy() const -> const ProfilePolicy & { return policy_; }

  /**
   * @brief Take the policy the profile's own marker records.
   *
   * Virtual because a shape may force it: a portable root is self-contained by
   * construction, and a marker saying otherwise is a copy artefact.
   *
   * @param marker the profile's marker
   */
  virtual void ApplyMarkerPolicy(const ProfileMarker &marker);

  /**
   * @brief Make the AppData tree exist and be readable.
   *
   * For an unpackaged profile this only provisions directories. For a packaged
   * one it extracts the package into its session root, which is the whole
   * difference between the two branches of the hierarchy.
   *
   * Synchronous, and must not be called on the I/O task runner.
   *
   * @param ctx the passphrase, when there is one, and this build's layout
   * version
   * @return the outcome
   */
  virtual auto Mount(const ProfileMountContext &ctx) -> ProfileMountResult = 0;

  /**
   * @brief Give the storage back.
   *
   * A transient profile's tree is deleted here. Anything that has to happen
   * *before* the data goes away — writing a package back to where it came
   * from — is the application's, not this: it needs the resident application
   * key and the live settings, neither of which a profile owns.
   *
   * @param mode how much time there is
   */
  virtual void Unmount(ProfileUnmountMode mode);

  /**
   * @brief Whether the storage is disposable and must not outlive the process.
   *
   * @return true for a packaged profile
   */
  [[nodiscard]] virtual auto IsTransient() const -> bool;

  /**
   * @brief Whether the system credential store may protect the app key.
   *
   * False for anything that can leave this machine: a key sealed with one
   * computer's keychain cannot be opened on another, so honouring the request
   * would strand the profile rather than protect it.
   *
   * @return true when the keychain is a usable backend here
   */
  [[nodiscard]] virtual auto AllowsSystemKeychain() const -> bool;

  /**
   * @brief Whether the application key may rotate on a schedule.
   *
   * False for a packaged profile: rotation writes a new key file into storage
   * that is deleted at the end of the session, so the rotated key would be lost
   * along with everything it had encrypted.
   *
   * @return true when rotation is meaningful
   */
  [[nodiscard]] virtual auto AllowsKeyRotation() const -> bool;

  /**
   * @brief Whether this profile belongs in the machine's registry.
   *
   * Only a PersistProfile does. The two root profiles exist whether or not the
   * registry has heard of them, and a packaged profile is deliberately never
   * recorded — the profile manager lists what this machine keeps.
   *
   * @return true when the registry should carry an entry
   */
  [[nodiscard]] virtual auto IsRegistrable() const -> bool;

  /**
   * @brief Where this profile's settings live, or nowhere.
   *
   * A rooted profile is INI-backed on every platform: a native QSettings store
   * is keyed only by organization and application name, so every profile would
   * share one registry key or plist, and the file would sit outside the profile
   * root where no package could carry it.
   *
   * @return the INI path, or an empty string meaning "use the native store"
   */
  [[nodiscard]] virtual auto SettingsFilePath() const -> QString;

  /**
   * @brief The arguments that reopen this profile in a new process.
   *
   * Two profiles cannot share a process, so opening one is always a launch, and
   * this is the one place that knows what to put on the command line.
   *
   * @return the arguments, empty when the profile is the implicit default
   */
  [[nodiscard]] virtual auto LaunchArguments() const -> QStringList = 0;

  /**
   * @brief The directory whose lock decides who may open this profile.
   *
   * The same thing as Root() for every profile whose storage is where the lock
   * is, which is all of them today. It is separate because it must be a pure
   * function of the selection: a profile whose storage is *chosen* — by probing
   * for somewhere the platform does not leave it readable — would otherwise
   * have two processes probe differently, lock different directories, and both
   * decide they were the only one open.
   *
   * @return the directory holding profile.lock
   */
  [[nodiscard]] virtual auto LockRoot() const -> QString;

  /**
   * @brief Build the storage driver for this profile.
   *
   * The filesystem driver for every shape today. A profile stored somewhere
   * else would return its own driver from here and nothing above would change.
   *
   * @return the accessor, already pointed at Root()
   */
  [[nodiscard]] virtual auto MakeAccessor() const
      -> QSharedPointer<ProfileAccessor>;

  /// Where this profile's marker sits.
  [[nodiscard]] auto MarkerPath() const -> QString;

 protected:
  ProfilePolicy policy_;
};

/**
 * @brief A profile stored as an ordinary directory tree.
 *
 * On this machine, or on a USB key: either way the storage is already
 * unpacked, so mounting it is just making sure the areas exist. These are the
 * profiles that may bind to the local system — the credential store, a rotating
 * key — precisely because they are not written to travel.
 */
class GF_CORE_EXPORT UnpackagedProfile : public Profile {
 public:
  [[nodiscard]] auto Root() const -> QString override { return root_; }
  auto Mount(const ProfileMountContext &ctx) -> ProfileMountResult override;

 protected:
  explicit UnpackagedProfile(QString root) : root_(std::move(root)) {}

  QString root_;
};

/**
 * @brief A root profile: the layout the application had before profiles.
 *
 * Special in exactly one way — it is the directory that *contains* the others.
 * `<root>/profiles/<uuid>` is where every PersistProfile lives and where the
 * machine's registry sits, which is why there are only ever two of these and
 * why neither can be deleted or renamed.
 */
class GF_CORE_EXPORT RootProfile : public UnpackagedProfile {
 public:
  /// Where this root keeps its PersistProfiles and their registry.
  [[nodiscard]] auto ProfilesDir() const -> QString;

  [[nodiscard]] auto IsRegistrable() const -> bool override;

 protected:
  using UnpackagedProfile::UnpackagedProfile;
};

/**
 * @brief The root at the OS user-data location.
 *
 * The original layout, and still the implicit default. Its settings stay in the
 * platform's native store on POSIX: they are already there on every existing
 * installation, and moving them would orphan every one of them.
 */
class GF_CORE_EXPORT InstalledRootProfile final : public RootProfile {
 public:
  explicit InstalledRootProfile(QString root);

  [[nodiscard]] auto Kind() const -> ProfileKind override;
  [[nodiscard]] auto Id() const -> QString override;
  [[nodiscard]] auto SettingsFilePath() const -> QString override;
  [[nodiscard]] auto LaunchArguments() const -> QStringList override;
};

/**
 * @brief The root in the directory above the application.
 *
 * Written to be carried on removable media, so it is self-contained whatever
 * its marker says, and the credential store is never an option for it.
 */
class GF_CORE_EXPORT PortableRootProfile final : public RootProfile {
 public:
  explicit PortableRootProfile(QString root);

  [[nodiscard]] auto Kind() const -> ProfileKind override;
  [[nodiscard]] auto Id() const -> QString override;
  [[nodiscard]] auto AllowsSystemKeychain() const -> bool override;
  [[nodiscard]] auto LaunchArguments() const -> QStringList override;

  /// Self-contained by construction: a marker that says otherwise came from a
  /// copy of a profile that was not portable.
  void ApplyMarkerPolicy(const ProfileMarker &marker) override;
};

/**
 * @brief One of the profiles a root holds, named by a uuid.
 *
 * The directory name is the identity: opaque, unique by construction, and the
 * only thing the credential-store account and the registry need to agree on.
 * What the user calls it is in the marker.
 */
class GF_CORE_EXPORT PersistProfile final : public UnpackagedProfile {
 public:
  PersistProfile(QString id, QString root);

  [[nodiscard]] auto Kind() const -> ProfileKind override;
  [[nodiscard]] auto Id() const -> QString override;
  [[nodiscard]] auto IsRegistrable() const -> bool override;
  [[nodiscard]] auto LaunchArguments() const -> QStringList override;

 private:
  QString id_;
};

/**
 * @brief A `.gfp` opened for the length of one session.
 *
 * Everything that makes this different from the rest follows from one decision:
 * the storage is a single encrypted file that is not this machine's. So it is
 * extracted into a transient root, that root is deleted on the way out, and the
 * only protections available are the two that travel — none, or a passphrase.
 *
 * There is exactly **one secret**. The passphrase that opens the package is
 * also the PIN that seals the application key inside it, so the user is asked
 * once and no plaintext key is written to disk for a protected package.
 */
class GF_CORE_EXPORT PackagedProfile final : public Profile {
 public:
  /**
   * @brief Name a package, without touching it.
   *
   * @param package_path the `.gfp`
   * @param profiles_root where transient roots may be made
   */
  PackagedProfile(QString package_path, QString profiles_root);

  [[nodiscard]] auto Kind() const -> ProfileKind override;
  [[nodiscard]] auto Id() const -> QString override;
  [[nodiscard]] auto Root() const -> QString override;
  [[nodiscard]] auto DisplayName() const -> QString override;

  [[nodiscard]] auto IsTransient() const -> bool override;
  [[nodiscard]] auto AllowsSystemKeychain() const -> bool override;
  [[nodiscard]] auto AllowsKeyRotation() const -> bool override;
  [[nodiscard]] auto IsRegistrable() const -> bool override;
  [[nodiscard]] auto LaunchArguments() const -> QStringList override;

  /**
   * @brief Read the package's header, which is cheap and needs no secret.
   *
   * Answers "is this a package at all" and "does it need a passphrase" before
   * anybody is asked for one.
   *
   * @return the outcome; kNEEDS_PASSPHRASE when one is required
   */
  auto Inspect() -> ProfileMountResult;

  auto Mount(const ProfileMountContext &ctx) -> ProfileMountResult override;
  void Unmount(ProfileUnmountMode mode) override;

  [[nodiscard]] auto PackagePath() const -> QString { return package_path_; }

  /// Where transient session roots are made, this one included.
  [[nodiscard]] auto ProfilesRoot() const -> QString { return profiles_root_; }
  [[nodiscard]] auto Protection() const -> ProfilePackageProtection;
  [[nodiscard]] auto Manifest() const -> const ProfilePackageManifest &;

  /// The one secret: the package passphrase, which is also the app-key PIN.
  [[nodiscard]] auto Passphrase() const -> GFBuffer { return passphrase_; }

  /**
   * @brief Everything a write-back needs that the profile itself knows.
   *
   * The caller fills in the application key, the settings snapshot and the key
   * database list, which live in the running process rather than here.
   *
   * @return a request aimed back at the file this session came from
   */
  [[nodiscard]] auto WriteBackRequest() const -> ProfileExportRequest;

  [[nodiscard]] auto LockRoot() const -> QString override;

  [[nodiscard]] auto MakeAccessor() const
      -> QSharedPointer<ProfileAccessor> override;

  /**
   * @brief Throw away whatever storage was claimed for this session.
   *
   * For the path where the package would not open: the lock was taken and the
   * storage provisioned before anyone knew whether the file was readable, and
   * both have to be given back before a dialog stops to wait for a human.
   *
   * Safe to call when nothing was claimed.
   */
  void DiscardSessionStorage();

 private:
  /**
   * @brief Protect the extracted key with the passphrase that opened the
   * package.
   *
   * Idempotent, and the reason a protected package leaves no plaintext key on
   * this machine while it is open.
   *
   * @return an error message, empty on success
   */
  auto seal_extracted_key() -> QString;

  /// Make the session's stored protection say what its key file actually is.
  void record_key_protection(bool pin);

  QString package_path_;
  QString profiles_root_;

  /// Where the lock lives. A pure function of the package path, so that a
  /// second window computes the same one without being told.
  QString anchor_;

  QString id_;

  /// Made once, on the first MakeAccessor(). Cached because choosing where the
  /// session goes is a decision, and a decision taken twice is two decisions.
  mutable QSharedPointer<ProfileAccessor> storage_;

  /// Set when no storage this profile's policy would accept was available, so
  /// that asking again neither re-probes nor reports the refusal twice.
  mutable bool storage_refused_ = false;

  /// What the probe passed over, in the words the user will be shown.
  mutable QStringList storage_rejections_;

  /// Unpacked size the package declares, or 0 when it declares none. Read from
  /// the manifest by Mount() before any storage is asked for, which is the only
  /// window in which it can influence how much is provisioned.
  mutable qint64 declared_bytes_ = 0;

  /// What a sweep needs to find this session's tree after a crash. Kept from
  /// the moment the storage was chosen, because the accessor handed out above
  /// may be a decorator and asking it for this would mean knowing its type.
  mutable QJsonObject storage_anchor_state_;

  bool inspected_ = false;
  bool mounted_ = false;
  ProfilePackageProtection protection_ = ProfilePackageProtection::kNONE;
  ProfilePackageManifest manifest_;
  GFBuffer passphrase_;
};

/**
 * @brief Every layer the profile decision draws on, as plain values.
 *
 * Taking the layers as values rather than reading them makes the whole
 * precedence ladder assertable without starting a process, in the same shape
 * ResolveAppKeyProtection() already uses for the key-protection ladder.
 */
struct GF_CORE_EXPORT ProfileSelectionInput {
  /// Full argument list, argv[0] included.
  QStringList args;

  QString env_profile;  ///< GF_PROFILE

  /**
   * @brief Whether this is a portable build.
   *
   * Decides only the *implicit* default and where the profiles root sits; it
   * never overrides an explicitly named profile. A field rather than a direct
   * call to IsPortableBuild() so that both flavours stay assertable from one
   * test binary.
   */
  bool portable_build = false;

  QString portable_root;   ///< ResolvePortableDataPath()
  QString installed_root;  ///< QStandardPaths::AppLocalDataLocation

  /**
   * @brief The profile ids that exist on disk, from ScanProfilesRoot().
   *
   * Naming an id that is not here is an error rather than an invitation to
   * create it: silently opening the wrong keyring, or provisioning an empty
   * profile over a typo, are the two worst outcomes available. A directory scan
   * is always authoritative, so there is no "we do not know yet" case.
   */
  QStringList known_ids;

  /**
   * @brief Whether this build may open anything but its implicit default.
   *
   * False on a build that ships without profiles at all — the macOS App Store
   * one, where opening another profile means launching a second instance with
   * `--profile`, and LaunchServices refuses to pass arguments on behalf of a
   * sandboxed process. The successor would arrive with an empty argv, fall back
   * to the default profile, and collide with the lock the window that asked for
   * the switch is still holding.
   *
   * A field rather than a call to IsRunningInAppSandbox() so this stays pure
   * and both answers are assertable from one test binary.
   */
  bool multi_profile = true;
};

/**
 * @brief Which profile this process should open, as values.
 *
 * Deliberately not a Profile: this is the answer to "what was asked for",
 * decided without touching a disk, and MakeProfile() is what turns it into the
 * object that knows how to open it.
 */
struct GF_CORE_EXPORT ProfileSelection {
  ProfileKind kind = ProfileKind::kINSTALLED_ROOT;

  /// "classic", "portable", or the profile's uuid.
  QString id;

  /// Absolute root, for everything except a package, whose root is derived
  /// from its own path once the profiles root is known.
  QString root;

  /// Where this machine's persisted profiles live.
  QString profiles_root;

  /// The `.gfp` named on the command line, for kPACKAGED.
  QString package_path;
};

/**
 * @brief The selection, or the reason there is none.
 */
struct GF_CORE_EXPORT ProfileSelectionResult {
  ProfileSelection selection;

  /// Non-empty means the application must stop; @ref selection is a safe
  /// fallback so nothing downstream has to cope with a half-resolved process.
  QString error;
};

/**
 * @brief Decide which profile this process runs against.
 *
 * Pure. Precedence, highest first: `--profile`, a positional `.gfp`,
 * `GF_PROFILE`, and finally the implicit default — the portable root on a
 * portable build, otherwise the installed one.
 *
 * There is deliberately no "reopen what was open last" rung. An instance always
 * starts on its root profile; anything else is opened from there into a new
 * window, which is the only way two profiles can be used at once anyway.
 *
 * @param in every layer, as values
 * @return the selection, or an error with an installed-root fallback
 */
auto GF_CORE_EXPORT ResolveProfileSelection(const ProfileSelectionInput &in)
    -> ProfileSelectionResult;

/**
 * @brief Turn a selection into the profile that knows how to open it.
 *
 * @param selection what was asked for
 * @return the profile; never null
 */
auto GF_CORE_EXPORT MakeProfile(const ProfileSelection &selection)
    -> QSharedPointer<Profile>;

/**
 * @brief Whether a string may be used as a profile id and directory name.
 *
 * Ids become directory names, so they are restricted rather than escaped:
 * lowercase alphanumerics, underscore and hyphen, at most 64 characters, and
 * never a Windows device name, which would produce a directory that cannot be
 * created on one platform and can on another.
 *
 * New ids are generated uuids, which satisfy this by construction; the check
 * remains because ids also arrive from `--profile` and from directory names
 * found on disk, neither of which this build wrote.
 *
 * @param id candidate id
 * @return whether it is usable
 */
auto GF_CORE_EXPORT IsValidProfileId(const QString &id) -> bool;

}  // namespace GpgFrontend
