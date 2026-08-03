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

namespace GpgFrontend {

/**
 * @brief Which shape of profile the process is running against.
 *
 * One enum for the whole feature: the bootstrap resolves it, the registry
 * stores it, and profile.json records it. Keeping a single spelling is what
 * stops "classic" and "portable" drifting into path comparisons scattered
 * across the code.
 */
enum class ProfileRootKind {
  kCLASSIC,          ///< legacy split storage: data and settings live apart
  kPORTABLE,         ///< the directory above the application, from ENV.ini
  kNAMED,            ///< <profiles-root>/<id>
  kEXPLICIT_ROOT,    ///< an absolute path given on the command line
  kPACKAGE_LINKED,   ///< extracted from a .gfprofile and remembers which
  kPACKAGE_PENDING,  ///< a package was named but is not extracted yet
};

/**
 * @brief Canonical spelling of a profile kind, as stored.
 *
 * @param kind kind to spell
 * @return the canonical lowercase token
 */
auto GF_CORE_EXPORT ProfileRootKindToString(ProfileRootKind kind) -> QString;

/**
 * @brief Parse the stored spelling of a profile kind.
 *
 * Anything unrecognised reads as kCLASSIC, which is the shape every existing
 * installation already has.
 *
 * @param s stored token
 * @return the parsed kind
 */
auto GF_CORE_EXPORT ProfileRootKindFromString(const QString &s)
    -> ProfileRootKind;

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
 * @brief Everything the process resolved about its profile, resolved once.
 */
struct GF_CORE_EXPORT ProfileRuntimeState {
  ProfileRootKind kind = ProfileRootKind::kCLASSIC;

  /// "classic", "portable", or the profile slug.
  QString id;

  /// Absolute profile root. Meaningless, and illegal to use, for
  /// kPACKAGE_PENDING — see RequireProfileRoot().
  QString root;

  /// Where named profiles and the registry live on this machine.
  QString profiles_root;

  ProfilePolicy policy;

  /// Package awaiting extraction; set only for kPACKAGE_PENDING.
  QString pending_package;
};

/**
 * @brief Read the root of a state, refusing the one state that has none.
 *
 * A kPACKAGE_PENDING state names a package that has not been extracted, so it
 * has no root yet. Reading one anyway would silently resolve to an empty path
 * and put the whole application on the wrong directory, so this aborts instead.
 *
 * @param state state to read
 * @return the absolute profile root
 */
auto GF_CORE_EXPORT RequireProfileRoot(const ProfileRuntimeState &state)
    -> QString;

/**
 * @brief What the registry says should happen when no profile is named.
 */
enum class ProfileStartupPolicy {
  kLAST_USED,  ///< reopen whatever was open last
  kASK,        ///< show a picker
  kFIXED,      ///< always open one specific profile
  kCLASSIC,    ///< always open the legacy location
};

auto GF_CORE_EXPORT ProfileStartupPolicyToString(ProfileStartupPolicy p)
    -> QString;
auto GF_CORE_EXPORT ProfileStartupPolicyFromString(const QString &s)
    -> ProfileStartupPolicy;

/**
 * @brief Every layer the profile decision draws on, as plain values.
 *
 * Taking the layers as values rather than reading them makes the whole
 * precedence ladder assertable without starting a process, in the same shape
 * ResolveAppKeyProtection() already uses for the key-protection ladder.
 */
struct GF_CORE_EXPORT ProfileBootstrapInput {
  /// Full argument list, argv[0] included.
  QStringList args;

  QString env_profile;       ///< GF_PROFILE
  QString env_profile_root;  ///< GF_PROFILE_ROOT

  /// ENV.ini PortableMode. Decides only the *implicit* default and where the
  /// profiles root sits; it never overrides an explicitly named profile.
  bool env_ini_portable = false;

  QString portable_root;  ///< ResolvePortableDataPath()
  QString classic_root;   ///< QStandardPaths::AppLocalDataLocation

  QString registry_last_used;
  QString registry_startup_profile;
  ProfileStartupPolicy startup_policy = ProfileStartupPolicy::kLAST_USED;

  /// Profile ids the registry knows about.
  QStringList known_ids;

  /**
   * @brief Whether @ref known_ids is authoritative.
   *
   * False before the registry exists, when an unknown id cannot be
   * distinguished from a registry that has simply never been written. Once
   * true, naming an id that is not in the list is an error rather than an
   * invitation to create it: silently opening the wrong keyring is the worst
   * outcome available here.
   */
  bool registry_available = false;
};

/**
 * @brief The resolved state, or the reason there is none.
 */
struct GF_CORE_EXPORT ProfileBootstrapResult {
  ProfileRuntimeState state;

  /// Non-empty means the application must stop; @ref state is a safe fallback
  /// so nothing downstream has to cope with a half-resolved process.
  QString error;
};

/**
 * @brief Decide which profile this process runs against.
 *
 * Pure. Precedence, highest first: `--profile-root`, `--profile`, a positional
 * `.gfprofile`, the environment variables, the registry's startup policy, and
 * finally the implicit default — the portable root when ENV.ini asked for it,
 * otherwise the classic location.
 *
 * @param in every layer, as values
 * @return the resolved state, or an error with a classic fallback state
 */
auto GF_CORE_EXPORT ResolveProfileBootstrap(const ProfileBootstrapInput &in)
    -> ProfileBootstrapResult;

/**
 * @brief The data root a portable installation uses: the directory above the
 * application.
 *
 * On Linux the application directory points inside a read-only AppImage mount,
 * so $APPIMAGE is followed instead.
 *
 * @return absolute path
 */
auto GF_CORE_EXPORT ResolvePortableDataPath() -> QString;

/**
 * @brief The directory holding the application binary, AppImage-aware.
 *
 * @return absolute path
 */
auto GF_CORE_EXPORT ResolveApplicationDirPath() -> QString;

/**
 * @brief Whether a string may be used as a profile id and directory name.
 *
 * Ids become directory names, so they are restricted rather than escaped:
 * lowercase alphanumerics, underscore and hyphen, at most 64 characters, and
 * never a Windows device name, which would produce a directory that cannot be
 * created on one platform and can on another.
 *
 * @param id candidate id
 * @return whether it is usable
 */
auto GF_CORE_EXPORT IsValidProfileId(const QString &id) -> bool;

/**
 * @brief Turn a display name into a usable profile id.
 *
 * @param name free-form name
 * @return a valid id, or an empty string when nothing usable remains
 */
auto GF_CORE_EXPORT MakeProfileId(const QString &name) -> QString;

/**
 * @brief The one resolved profile state for this process.
 *
 * A dynamic property on qApp would be untyped, writable by anyone, and would
 * read back as an empty value when consulted too early — which for a profile
 * root means operating on the wrong key material. So the authority is this,
 * established exactly once, and every failure to respect that is fatal in every
 * build configuration rather than an assertion that release builds drop.
 */
class GF_CORE_EXPORT ProfileRuntime {
 public:
  /**
   * @brief Fix the profile for the lifetime of the process.
   *
   * @param state resolved state; must not be kPACKAGE_PENDING's successor
   */
  static void Establish(const ProfileRuntimeState &state);

  /**
   * @brief Replace a kPACKAGE_PENDING state once the package is extracted.
   *
   * The only legal transition out of kPACKAGE_PENDING, and legal only from it.
   *
   * @param state state naming the extracted root
   */
  static void EstablishFromPackage(const ProfileRuntimeState &state);

  /**
   * @brief Whether the profile has been resolved yet.
   *
   * @return true once Establish() has run
   */
  static auto Established() -> bool;

  /**
   * @brief The resolved state. Aborts if called before Establish().
   *
   * @return the state
   */
  static auto Instance() -> const ProfileRuntimeState &;

  /**
   * @brief Drop the established state. Test harnesses only.
   *
   * Never call this from application code: everything that cached a path
   * derived from the old state would keep using it.
   */
  static void ResetForTesting();
};

}  // namespace GpgFrontend
