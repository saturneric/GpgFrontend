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

namespace GpgFrontend {

/**
 * @brief Whether anything outside this process needs a real path for an area.
 *
 * The question a storage driver asks before deciding it may hold an area in
 * memory. It is a property of the area rather than of the driver: GnuPG is
 * handed a home directory, a module is dlopen'd and QSettings opens a file, and
 * no driver can make those not need a path.
 */
enum class AreaResidency : std::uint8_t {
  kPathRequired,   ///< an external process or library opens it by path
  kVirtualisable,  ///< only this process reads it; a driver may hold it in
                   ///< memory
};

/**
 * @brief Whether an area belongs inside a profile package.
 *
 * The allow-list. A `.gfp` is handed to another person, so anything not named
 * here stays home — including whatever a user drops in the profile folder and
 * whatever a later feature starts writing there. Deciding by omission is the
 * point: a deny-list fails open, and this one has to fail closed.
 */
enum class AreaPackaging : std::uint8_t {
  kNever,     ///< machine-local: logs, modules, scratch
  kAlways,    ///< part of the profile wherever it is opened
  kOptional,  ///< only when the user asks for it (workspace)
};

/**
 * @brief Where a packer reads an area's bytes from.
 *
 * Separate from packaging because an area that travels is not necessarily an
 * area that exists on disk: a driver may hold it in memory, and then the only
 * way to read it is through the accessor.
 */
enum class AreaPackSource : std::uint8_t {
  kTree,      ///< walk the files under the storage root
  kAccessor,  ///< read through ProfileAccessor, wherever the driver put it
};

/**
 * @brief Everything the rest of the program needs to decide about one area.
 *
 * One row per top-level name a profile root may contain. Every question that
 * used to be answered by a hand-written special case somewhere — may this be
 * held in memory, does it travel in a package, where do its bytes come from, is
 * it secret — is answered from here instead.
 *
 * The reason this is a table rather than a set of functions: those questions
 * were being answered in several places that could disagree, and did. Adding an
 * area now means adding a row, and a row cannot be added without answering all
 * four.
 */
struct GF_CORE_EXPORT ProfileAreaTraits {
  /// Top-level name under the profile root; empty for the root itself.
  QLatin1StringView dir;

  /// The area this name is, when it is one. The key databases are directories
  /// GnuPG owns rather than areas this program addresses, so they have none.
  std::optional<ProfileArea> area;

  AreaResidency residency = AreaResidency::kPathRequired;
  AreaPackaging packaging = AreaPackaging::kNever;
  AreaPackSource pack_source = AreaPackSource::kTree;

  /// Holds key material: zeroize rather than drop, scrub rather than unlink,
  /// and never write its path into a log.
  bool secret = false;
};

/**
 * @brief Every top-level name a profile root may contain.
 *
 * @return the table, in no significant order
 */
/// The name of the profile's own application key inside the secure area.
///
/// Here rather than beside either of its users. It is the one object in the
/// secure area whose bytes do not come from where the others' do, so it is
/// load-bearing in two unrelated places: the guard that keeps it out of the
/// rotated-key trial-decrypt loop, and the rule that resolves it from the key
/// in hand rather than from storage. Two private copies of the string meant
/// changing one silently fed the root key into that loop.
inline constexpr auto kProfileRootKeyName = "app.key";

/// Where a profile tree sits inside a package, relative to the archive root.
inline constexpr auto kProfileTreePrefix = "profile";

/// @note The rows are copies. TraitsForArea() and TraitsForTopLevel() return
/// pointers into the underlying constexpr array instead, so a pointer from one
/// never equals the address of a row from the other; compare by `dir` or
/// `area`.
auto GF_CORE_EXPORT ProfileAreaTable() -> const QList<ProfileAreaTraits> &;

/**
 * @brief Look up a top-level name.
 *
 * @param name a single path component, not a path
 * @return its row, or nullptr when the name is not one this program knows —
 * which is the answer that keeps it out of packages
 */
auto GF_CORE_EXPORT TraitsForTopLevel(QStringView name)
    -> const ProfileAreaTraits *;

/**
 * @brief Look up an area.
 *
 * @param area area to describe
 * @return its row; never null, since every enumerator has one
 */
auto GF_CORE_EXPORT TraitsForArea(ProfileArea area)
    -> const ProfileAreaTraits *;

/**
 * @brief The directory an area occupies under a filesystem root.
 *
 * @param area area to resolve
 * @return the directory name, or empty for the root itself
 */
auto GF_CORE_EXPORT ProfileAreaDirName(ProfileArea area) -> QString;

/**
 * @brief Whether a profile-relative path belongs inside a package.
 *
 * Two rules, in this order. The **top level** is an allow-list read from the
 * table: a name with no row does not travel, whatever it contains. **Within**
 * an admitted directory a deny-list still applies, because `db/` and `dbs/*`
 * are gpg-agent's working directories and hold sockets, lock files and editor
 * leftovers that mean nothing on another machine.
 *
 * Pure, so the whole truth table can be asserted without a filesystem.
 *
 * @param relative_path path relative to the profile root, forward slashes
 * @param include_workspace whether the user asked for their own files
 * @return true when the path may be packed
 */
auto GF_CORE_EXPORT IsIncludedInPackage(const QString &relative_path,
                                        bool include_workspace) -> bool;

/**
 * @brief Whether a name inside an admitted directory is refused anyway.
 *
 * Split out from IsIncludedInPackage() so the two rules can be tested apart:
 * this one is about a process that will not exist at the other end, not about
 * what the profile contains.
 *
 * @param relative_path path relative to the profile root
 * @return true when the entry must not be packed
 */
auto GF_CORE_EXPORT IsRefusedInsidePackagedArea(const QString &relative_path)
    -> bool;

/**
 * @brief The key-database directories a package carries.
 *
 * A key database the user pointed at by hand is an arrangement on their own
 * machine and does not travel, wherever they put it. Only the managed locations
 * do, and this is the one list that says which — used both by the packer and by
 * the manifest, so the two cannot disagree about what is inside the file.
 *
 * @return the managed top-level directory names
 */
auto GF_CORE_EXPORT ManagedKeyDatabaseDirs() -> QStringList;

/**
 * @brief Whether a profile-relative key-database path is a managed one.
 *
 * @param relative_path the database's path relative to the profile root
 * @return true when it lives somewhere a package carries
 */
auto GF_CORE_EXPORT IsManagedKeyDatabasePath(const QString &relative_path)
    -> bool;

}  // namespace GpgFrontend
