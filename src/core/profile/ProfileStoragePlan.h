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
 * @brief Where a packaged profile's session tree is allowed to live.
 *
 * A `.gfp` is somebody else's profile, so unpacking it writes another machine's
 * secret key material onto this one. This says how hard to try to avoid leaving
 * it readable here.
 *
 * Deliberately not named after a mechanism: what counts as protection differs
 * per platform, and a policy that said "memory only" would be a permanent lie
 * on the two platforms that have no memory-backed filesystem.
 */
enum class ProfileStoragePolicy : std::uint8_t {
  kAUTO,            ///< the best protected storage available, else a plain one
  kPROTECTED_ONLY,  ///< refuse to open rather than fall back to a plain one
  kDISK,            ///< the profiles folder, as every earlier build did
};

/// Environment variable that overrides the stored policy, for the case where
/// the setting itself cannot be reached — a package is opened before any of
/// this machine's profiles are, so its own config is not readable yet.
constexpr auto kProfileStoragePolicyEnv = "GF_PROFILE_PACKAGE_STORAGE";

/// Environment variable that puts one directory ahead of every probed
/// candidate. Same spirit as GPGFRONTEND_LIBSECRET_PATH: the only way to try a
/// fix on someone else's machine without building them a new binary first.
constexpr auto kProfileStorageDirEnv = "GF_PROFILE_PACKAGE_STORAGE_DIR";

/// The setting this policy is read from, on the profile that launched the
/// session rather than on the session itself.
constexpr auto kProfileStorageSettingKey = "advanced/profile_package_storage";

/**
 * @brief Canonical stored spelling of a policy.
 *
 * @param policy policy to spell
 * @return the canonical lowercase token
 */
auto GF_CORE_EXPORT ProfileStoragePolicyToString(ProfileStoragePolicy policy)
    -> QString;

/**
 * @brief Parse the stored spelling of a policy.
 *
 * Anything unrecognised reads as kAUTO. A setting file written by a newer build
 * must not stop this one from opening a package at all, and refusing to open is
 * the one outcome a typo should never be able to cause.
 *
 * @param s stored token
 * @return the parsed policy
 */
auto GF_CORE_EXPORT ProfileStoragePolicyFromString(const QString &s)
    -> ProfileStoragePolicy;

/**
 * @brief The policy in force for this process.
 *
 * A packaged session cannot read its own settings to answer this — its config
 * lives inside the storage being decided — so the answer comes from the
 * environment, and otherwise from the profile that launched it.
 *
 * @return the policy, kAUTO when nothing says otherwise
 */
auto GF_CORE_EXPORT ResolveProfileStoragePolicy() -> ProfileStoragePolicy;

/**
 * @brief Directories to try, in order, when looking for RAM-backed storage.
 *
 * Only meaningful where a RAM-backed filesystem exists at all; on other
 * platforms the caller does not ask.
 *
 * Every input is a parameter rather than an environment read so that the order,
 * which is the whole point of this function, can be tested without a test
 * having to mutate an environment it does not own.
 *
 * @param xdg_runtime_dir $XDG_RUNTIME_DIR, empty when unset. Ignored unless
 * absolute: a relative one would name whatever directory the process sits in.
 * @param override_path $GF_PROFILE_PACKAGE_STORAGE_DIR, empty when unset
 * @param uid the effective user id, for the paths that embed one
 * @return candidates, most specific first; free of duplicates
 */
auto GF_CORE_EXPORT VolatileStoreSearchPaths(const QString &xdg_runtime_dir,
                                             const QString &override_path,
                                             uint uid) -> QStringList;

/**
 * @brief Whether a statfs(2) f_type names a filesystem that lives in memory.
 *
 * The check that the whole Linux side rests on. $XDG_RUNTIME_DIR is a tmpfs on
 * every systemd distribution but nothing guarantees it — a container can point
 * it at real disk — and a directory that merely looks like the right place is
 * exactly the failure this feature exists to prevent. Anything not recognised
 * is refused rather than assumed.
 *
 * @param f_type the f_type field of a statfs result
 * @return true only for tmpfs and ramfs
 */
auto GF_CORE_EXPORT IsRamBackedMagic(quint64 f_type) -> bool;

/**
 * @brief Whether an existing directory is safe to put a session tree in.
 *
 * The candidate bases are shared: /dev/shm is world-writable, and a directory
 * already sitting at the name this build would pick may not be ours. Adopting
 * one would hand another local user a live view of the profile.
 *
 * @param owner_uid st_uid of the directory
 * @param mode the permission bits of the directory, st_mode & 0777
 * @param is_symlink whether the name resolved to a symlink
 * @param euid the effective user id to compare against
 * @return true only for a real directory this user owns, at 0700
 */
auto GF_CORE_EXPORT IsAcceptableOwnership(uint owner_uid, uint mode,
                                          bool is_symlink, uint euid) -> bool;

/**
 * @brief One place a session tree could go, and what it would be worth.
 *
 * The two protection axes are separate because no platform offers both, and
 * collapsing them would mean calling one of them the real one. A tmpfs is gone
 * when the power goes; an encrypted volume is not, but what survives is
 * unreadable. A caller that wants "not left readable on this disk" wants
 * either, and says so by asking IsProtected().
 */
struct GF_CORE_EXPORT StorageCandidate {
  QString path;    ///< the base directory, not the session root within it
  QString driver;  ///< stable token for logs and the status strip

  bool is_volatile = false;  ///< contents die with power
  /// what reaches the medium is ciphertext this process cannot read once it has
  /// released the storage
  bool is_encrypted_at_rest = false;

  bool usable = false;    ///< survived probing; false means see reason
  qint64 free_bytes = 0;  ///< headroom measured at probe time
  QString reason;         ///< why not, in the words the user will be shown

  /// Whether this candidate leaves the tree unreadable here, either way.
  [[nodiscard]] auto IsProtected() const -> bool {
    return is_volatile || is_encrypted_at_rest;
  }
};

/**
 * @brief The chosen candidate, and what was passed over to get there.
 */
struct GF_CORE_EXPORT ProfileStoragePlan {
  QString path;    ///< empty when refuse is true
  QString driver;  ///< the chosen candidate's token

  bool is_volatile = false;
  bool is_encrypted_at_rest = false;

  /// No protected candidate was usable and the policy forbids falling back.
  bool refuse = false;

  /// One line per candidate that was not chosen, each naming its reason. Shown
  /// verbatim in the refusal dialog: "this machine cannot" is not an answer a
  /// user can act on, and the reasons are the part that is actionable.
  QStringList rejections;
};

/**
 * @brief Choose where the session tree goes.
 *
 * Pure, so that the decision can be tested against every shape of machine
 * without needing one. Probing belongs to the caller; this only judges what the
 * probe found.
 *
 * The candidate order is the caller's preference and is honoured as given,
 * except that a protected candidate always beats an unprotected one under
 * kAUTO — otherwise a platform whose best option happens to be listed second
 * would silently never use it.
 *
 * @param policy what the user asked for
 * @param candidates probed candidates, most preferred first
 * @return the plan; check refuse before path
 */
auto GF_CORE_EXPORT PlanProfileStorage(
    ProfileStoragePolicy policy, const QList<StorageCandidate> &candidates)
    -> ProfileStoragePlan;

/**
 * @brief How much room a package's session will need.
 *
 * Deliberately generous, because the moment this is wrong is the worst one:
 * running out of space at extraction merely fails to open, but running out at
 * close time fails the write-back and loses everything the user did in the
 * session. The tree is staged a second time on the way out, on the same medium,
 * and GnuPG grows its own home directory while the window is open.
 *
 * @param package_bytes size of the .gfp on disk; the only signal old packages
 * carry, since the manifest that would say more is inside the ciphertext
 * @param declared_uncompressed the manifest's recorded unpacked size, or 0 when
 * the package predates it
 * @return a byte budget, never below a usable floor
 */
auto GF_CORE_EXPORT ProfileStorageBudget(qint64 package_bytes,
                                         qint64 declared_uncompressed)
    -> qint64;

}  // namespace GpgFrontend
