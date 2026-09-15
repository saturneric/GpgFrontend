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

#include "core/module/ModulePackageVerifier.h"

namespace GpgFrontend::Module {

/**
 * @brief Where installed modules live, relative to the modules directory.
 *
 * Dot-prefixed for the same reason every transient profile root is: the
 * directory scan that offers packages to the loader must never walk into the
 * store and offer an already-installed copy as a second candidate.
 */
constexpr auto kModuleStoreDirName = ".store";

/**
 * @brief What installing or resolving a module produced.
 */
struct GF_CORE_EXPORT ModuleInstallResult {
  bool ok = false;
  ModulePackageStatus status = ModulePackageStatus::kOK;
  QString reason;

  QString install_dir;   ///< the extracted, verified tree
  QString library_path;  ///< the module binary inside it
  ModuleManifest manifest;

  /// True when the package was already installed at this exact version and
  /// nothing was written. A rebuilt package with different bytes is a
  /// different version even at the same version string.
  bool already_installed = false;
};

/**
 * @brief The store root for a given modules directory.
 *
 * @param mods_dir the profile's modules directory
 * @return absolute path to the store, which need not exist yet
 */
auto GF_CORE_EXPORT ModuleStoreRoot(const QString& mods_dir) -> QString;

/**
 * @brief Verify a package and install it, or report that it already is.
 *
 * Ordering is the whole of the safety property, and it is the same ordering
 * the loader used before a store existed: the package is verified completely
 * -- in memory, writing nothing -- and only then extracted. A package that
 * fails leaves the store exactly as it found it.
 *
 * Installation itself is staged and then committed: the tree is extracted into
 * a dot-prefixed scratch directory, **re-verified where it landed**, and only
 * then moved into place under a name derived from the package's own digest.
 * Re-verifying after extraction rather than trusting the extraction is what
 * catches a truncated write or a full disk, which are the realistic failures
 * here and which a rename would otherwise make permanent.
 *
 * A version directory is named by the package digest, so two builds of the
 * same version string are two installs and neither silently becomes the other.
 * Installing over an existing version is therefore a no-op rather than an
 * overwrite, and switching versions is recorded so it can be undone.
 *
 * @param package_path the `*.gfmodule` to install
 * @param store_root where installed modules live
 * @param expected_public_key when non-empty, the key the package MUST carry
 * @return where it was installed, or why it was refused
 */
auto GF_CORE_EXPORT InstallModulePackage(
    const QString& package_path, const QString& store_root,
    const QByteArray& expected_public_key = {}) -> ModuleInstallResult;

/**
 * @brief Find an installed module and re-verify it before handing it over.
 *
 * The re-verification is not belt and braces: an installed tree is ordinary
 * files on a disk the user owns, so the only enforceable form of immutability
 * is that a change is **detected before the module is loaded**. Marking the
 * files read-only, which the installer also does, is advisory.
 *
 * @param store_root where installed modules live
 * @param module_id the module wanted
 * @param expected_public_key when non-empty, the key the tree MUST carry
 * @return the installed tree, or why it cannot be used
 */
auto GF_CORE_EXPORT ResolveInstalledModule(
    const QString& store_root, const QString& module_id,
    const QByteArray& expected_public_key = {}) -> ModuleInstallResult;

/**
 * @brief Go back to the version this module was on before the current one.
 *
 * Only ever one step: the store keeps the previous version and no more,
 * because a chain of them is a retention policy rather than a safety net, and
 * the case this exists for is "the update I just took does not work".
 *
 * The version being left is kept, so a rollback can itself be undone.
 *
 * @param store_root where installed modules live
 * @param module_id the module to roll back
 * @return the now-current tree, or why nothing changed
 */
auto GF_CORE_EXPORT RollbackInstalledModule(const QString& store_root,
                                            const QString& module_id)
    -> ModuleInstallResult;

/**
 * @brief Every module id the store holds an installed version for.
 *
 * @param store_root where installed modules live
 * @return module identifiers, sorted
 */
auto GF_CORE_EXPORT ListInstalledModules(const QString& store_root)
    -> QStringList;

/**
 * @brief Drop version directories nothing points at, and leftover staging.
 *
 * A crashed install leaves a dot-prefixed scratch directory; a superseded
 * version that has itself been superseded is no longer reachable by a
 * rollback. Neither is referenced by the store's state, which is what makes
 * removing them safe rather than a guess.
 *
 * @param store_root where installed modules live
 * @return how many directories were removed
 */
auto GF_CORE_EXPORT SweepModuleStore(const QString& store_root) -> int;

}  // namespace GpgFrontend::Module
