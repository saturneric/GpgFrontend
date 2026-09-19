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

namespace GpgFrontend::Module {

/**
 * @file ModuleSetVerification.h
 * @brief Checking a whole module tree, not one module at a time.
 *
 * Per-module verification cannot see the problems that are properties of the
 * *set*: two namespaces claiming one identity, a namespace with no descriptor,
 * a native file nothing binds. Those only exist between modules, so they need
 * a check that looks at all of them.
 *
 * ## What this replaces
 *
 * The old invariant was "a shipping tree contains no loose native module
 * binaries", enforced by looking for files. That is no longer true or
 * desirable: native libraries are intentional shipping artifacts now, because
 * shipping them is what lets $ORIGIN, @loader_path, dependency scanners and
 * platform code signing treat them as what they are.
 *
 * The replacement is a property a directory listing cannot check:
 *
 * > Every shipped module-owned native image is bound by exactly one verified
 * > descriptor, and every descriptor resolves to the entry it declares.
 */

/// One thing wrong with a module tree.
struct GF_CORE_EXPORT ModuleSetFinding {
  QString where;   ///< the namespace or file it is about
  QString reason;  ///< what is wrong with it, for a person
};

/// What checking a whole module tree concluded.
struct GF_CORE_EXPORT ModuleSetVerification {
  bool ok = false;

  /// Descriptors that verified and resolved, by module id.
  QMap<QString, QString> verified;

  /// Anything that makes the tree unshippable.
  QVector<ModuleSetFinding> problems;

  /// Worth saying, not worth failing over: a stale helper left by a rename,
  /// for instance. Reported so it is visible, not fatal so it cannot invent a
  /// requirement nothing actually has.
  QVector<ModuleSetFinding> warnings;
};

/**
 * @brief Verify every module namespace under @p root.
 *
 * Checks, in this order, and reporting everything rather than stopping at the
 * first problem -- a release gate that names one fault per run is a gate that
 * takes as many runs as there are faults:
 *
 * - every namespace holds a descriptor, and every descriptor verifies under
 *   this Host's embedded build key and build id;
 * - the namespace directory name is the one the signed identity derives;
 * - the entry native exists in that namespace, is the right kind of file, and
 *   satisfies the binding the descriptor records;
 * - no two namespaces claim the same module id;
 * - the number of modules is the number expected, when one is given.
 *
 * @param root the namespace root; each subdirectory is one module
 * @param expected_count how many modules should be there, or -1 for any
 */
auto GF_CORE_EXPORT VerifyModuleSet(const QString& root,
                                    int expected_count = -1)
    -> ModuleSetVerification;

/**
 * @brief Find module-owned native files that are outside any namespace.
 *
 * The inverted survivor of the old assert-no-native-modules check. A native
 * library inside a module's own `native/` directory is where it belongs; one
 * anywhere else in a shipping tree is a leak from a staging step.
 *
 * @param root the tree to walk
 * @param namespace_root the one place such files are allowed to be
 */
auto GF_CORE_EXPORT FindNativeModuleBinariesOutside(
    const QString& root, const QString& namespace_root) -> QStringList;

}  // namespace GpgFrontend::Module
