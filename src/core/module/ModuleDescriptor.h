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
#include "core/module/ModuleTrustRoot.h"

namespace GpgFrontend::Module {

/// The extension a module descriptor carries. Deliberately unchanged: the
/// name is user-visible, and renaming it would break every existing file for
/// the sake of an internal vocabulary change.
constexpr auto kModulePackageSuffix = ".gfmodule";

/// The descriptor's filename inside a module namespace. Fixed, not derived:
/// the old scheme named the package for its CMake target and the library for
/// its SDK prefix, and reconciling the two was a source of real bugs.
constexpr auto kModuleDescriptorFileName = "module.gfmodule";

/// Where the signed metadata lives inside the descriptor.
constexpr auto kModuleDescriptorManifestPath = "META-INF/manifest.json";
constexpr auto kModuleDescriptorSignaturePath = "META-INF/manifest.sig";
/// Where the build key USED to live. Retained only so a descriptor that still
/// carries one can be refused by name rather than as an undeclared member.
constexpr auto kModuleDescriptorBuildKeyPath = "META-INF/build-key.pub";

/**
 * @brief Why a descriptor was refused.
 *
 * Mirrors ProfilePackageReadStatus rather than inventing a second vocabulary
 * for the same job: a caller that already knows how to turn one of those into
 * a sentence needs no new habits for this.
 */
enum class ModuleDescriptorStatus {
  kOK,
  kNOT_A_PACKAGE,        ///< not a readable archive of this shape
  kTOO_NEW,              ///< manifest schema beyond this build
  kMALFORMED,            ///< structurally wrong, including a bad manifest
  kBAD_SIGNATURE,        ///< the signature does not cover these bytes
  kUNTRUSTED_BUILD_KEY,  ///< not signed by this Host build's module key
  kWRONG_BUILD,          ///< signed by this key, but for a different build
  /// A declared resource is not the resource that is there.
  kRESOURCE_DIGEST_MISMATCH,
  /// A member the manifest does not declare, so the signature does not cover
  /// it. Refused outright rather than ignored: see the set rule below.
  kUNDECLARED_RESOURCE,
  /// The manifest names a resource the descriptor lacks.
  kMISSING_DECLARED_RESOURCE,
  kWRONG_PLATFORM,    ///< built for another os or architecture
  kINCOMPATIBLE_ABI,  ///< outside [GF_SDK_ABI_MIN_SUPPORTED, ...]
  kIO_FAILED,         ///< the file could not be read
};

/**
 * @brief Human-readable spelling of a status, for logs and messages.
 *
 * @param s status to spell
 * @return a short static string
 */
auto GF_CORE_EXPORT ModuleDescriptorStatusToString(ModuleDescriptorStatus s)
    -> const char*;

/**
 * @brief What verification concluded about a descriptor.
 *
 * AUTHORITATIVE. Every fact here was established against the bytes the
 * signature covers, and callers are expected to *consume* them -- not to
 * reopen the descriptor and work any of them out again. A second search is a
 * second chance to search differently.
 *
 * In particular @ref manifest carries the entry-native binding, and
 * ResolveAndVerifyNativeEntry() takes it from here rather than re-reading the
 * file.
 */
struct GF_CORE_EXPORT ModuleDescriptorVerification {
  bool ok = false;
  ModuleDescriptorStatus status = ModuleDescriptorStatus::kOK;
  QString reason;  ///< human-readable, for the log and the UI

  /// Parsed only after the signature over its raw bytes verified.
  ModuleManifest manifest;

  /// The trust root this verdict was reached under -- the key the caller
  /// passed, echoed back so a caller holding several need not track which one
  /// answered. It is NOT read out of the descriptor: nothing inside a
  /// descriptor is ever treated as a trust root, which is why
  /// `META-INF/build-key.pub` was removed rather than merely ignored.
  QByteArray build_public_key;
};

/**
 * @brief Authenticate a descriptor against this Host build's trust root.
 *
 * Reads the file once into an owned buffer and works entirely in memory: no
 * extraction, no store, no temporary directory, no second open. That is a
 * structural property rather than a discipline -- the archive is walked by
 * ReadArchiveMembersSync(), which takes bytes and has no destination
 * parameter to point anywhere.
 *
 * ## What this proves
 *
 * The descriptor was signed by the ephemeral Ed25519 key belonging to *this*
 * Host build, over exactly the manifest bytes stored in it, and it names this
 * build's id. A descriptor from another build, or one re-signed with a fresh
 * key, is refused -- @ref ModuleDescriptorStatus::kWRONG_BUILD and
 * @ref ModuleDescriptorStatus::kUNTRUSTED_BUILD_KEY respectively.
 *
 * The member *set* is checked exactly: every declared resource present, every
 * present member declared. Resource *bytes* are hashed lazily, on
 * ReadResource(), so a large unused resource costs nothing here.
 *
 * ## What this does not prove
 *
 * Nothing about the entry native. The descriptor names it logically and
 * carries its binding value; resolving and verifying it is
 * ResolveAndVerifyNativeEntry()'s job, under the mode the target platform
 * mandates. Nor does it authenticate the entry's dependency closure, which
 * stays the platform loader's business.
 *
 * @param package_path the `*.gfmodule` file to verify
 * @param expected_public_key the trust root to verify against. Defaults to
 * the key compiled into this Host; a test passes a different one to prove
 * that a foreign key is refused rather than accepted.
 * @return the verdict, with the manifest filled in only when it verified
 */
auto GF_CORE_EXPORT VerifyModuleDescriptor(
    const QString& package_path,
    const QByteArray& expected_public_key = ModuleBuildPublicKey())
    -> ModuleDescriptorVerification;

}  // namespace GpgFrontend::Module
