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

#include "core/module/ModuleManifest.h"

namespace GpgFrontend::Module {

/// The extension a module package carries.
constexpr auto kModulePackageSuffix = ".gfmodule";

/// Where the signed metadata lives inside the package.
constexpr auto kModulePackageManifestPath = "META-INF/manifest.json";
constexpr auto kModulePackageSignaturePath = "META-INF/manifest.sig";
constexpr auto kModulePackageBuildKeyPath = "META-INF/build-key.pub";

/**
 * @brief Why a package was refused.
 *
 * Mirrors ProfilePackageReadStatus rather than inventing a second vocabulary
 * for the same job: a caller that already knows how to turn one of those into
 * a sentence needs no new habits for this.
 */
enum class ModulePackageStatus {
  kOK,
  kNOT_A_PACKAGE,          ///< not a readable archive of this shape
  kTOO_NEW,                ///< manifest schema beyond this build
  kMALFORMED,              ///< structurally wrong, including a bad manifest
  kBAD_SIGNATURE,          ///< the signature does not cover these bytes
  kFILE_DIGEST_MISMATCH,   ///< a declared file is not the file that is there
  kUNDECLARED_FILE,        ///< a file the signature does not cover
  kMISSING_DECLARED_FILE,  ///< the manifest names a file the package lacks
  kWRONG_PLATFORM,         ///< built for another os or architecture
  kINCOMPATIBLE_ABI,       ///< outside [GF_SDK_ABI_MIN_SUPPORTED, ...]
  kIO_FAILED,              ///< the file could not be read
};

/**
 * @brief Human-readable spelling of a status, for logs and messages.
 *
 * @param s status to spell
 * @return a short static string
 */
auto GF_CORE_EXPORT ModulePackageStatusToString(ModulePackageStatus s) -> const
    char*;

/**
 * @brief What verification concluded about a package.
 *
 * AUTHORITATIVE. Every fact here was established against the bytes the
 * signature covers, and callers are expected to *consume* them -- not to
 * reopen the package and work any of them out again. The manager used to
 * re-scan `manifest.files` for the `bin/` entry that @ref library_sha256 now
 * carries; the two agreed, but nothing made them agree, and a second search
 * is a second chance to search differently.
 */
struct GF_CORE_EXPORT ModulePackageVerification {
  bool ok = false;
  ModulePackageStatus status = ModulePackageStatus::kOK;
  QString reason;  ///< human-readable, for the log and the UI

  /// Parsed only after the signature over its raw bytes verified.
  ModuleManifest manifest;

  /// The public key as found in META-INF. A hint, not a trust root.
  QByteArray build_public_key;
};

/**
 * @brief Decide whether a package is internally consistent, without running it.
 *
 * Pure with respect to the package: it reads the file and nothing else. No
 * extraction, no store, no side effects beyond a temporary directory that
 * nothing is ever written into. Keeping it that way is what lets a persistent
 * module store arrive later without touching this function, and it is why
 * every negative case below is a single call rather than a fixture.
 *
 * ## What this proves, and what it does not
 *
 * `META-INF/build-key.pub` travels *inside* the package. So an adversary who
 * can replace the package can also generate a keypair, re-sign an altered
 * manifest, and ship the matching public key -- and this function will say
 * yes. What it therefore establishes is exactly three things:
 *
 * - **internal consistency**: manifest, signature and file digests agree;
 * - **accidental or partial modification**: a truncated download, a
 *   half-written file, a botched repack;
 * - **bit rot and single-file tampering**, *provided the rest of the package
 *   is unchanged* -- someone editing `bin/module.so` inside the zip without
 *   re-signing everything.
 *
 * It does **not** establish who built the package, and it does **not** stop
 * anyone substituting the whole package with a freshly self-signed one --
 * including on the download path. Publisher authenticity needs a key delivered
 * out of band, which is what @p expected_public_key is for and which nothing
 * supplies yet.
 *
 * @param package_path the `*.gfmodule` file to verify
 * @param expected_public_key when non-empty, the key the package MUST carry;
 * the package's own key is then a value to check rather than a value to trust.
 * Empty in this phase, which means self-consistency only.
 * @return the verdict, with the manifest filled in only when it verified
 */
auto GF_CORE_EXPORT VerifyModulePackage(
    const QString& package_path, const QByteArray& expected_public_key = {})
    -> ModulePackageVerification;

}  // namespace GpgFrontend::Module
