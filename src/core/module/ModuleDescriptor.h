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

#include "core/module/ModuleHostPolicy.h"
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
  kNOT_A_PACKAGE,  ///< not a readable archive of this shape
  kTOO_NEW,        ///< manifest schema beyond this build
  kMALFORMED,      ///< structurally wrong, including a bad manifest
  /// Not signed by this Host build's module key.
  ///
  /// There is deliberately no separate "bad signature" status. With a single
  /// trust root, a failed verification means either the bytes were tampered
  /// with or the signer was someone else, and nothing on this side can tell
  /// which. Two statuses would have meant reporting a distinction the code
  /// cannot actually make.
  kUNTRUSTED_BUILD_KEY,
  kWRONG_BUILD,  ///< signed by this key, but for a different build
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
/**
 * @brief Read back every non-META-INF member of a descriptor.
 *
 * For regenerating a descriptor in place: a rewritten one has to carry
 * forward exactly the resources the old one did, and re-reading them from the
 * signed original is the only way to be sure it does.
 *
 * Call this only on a descriptor that has already verified. It re-reads the
 * file rather than taking a verified handle because the one caller -- the
 * packager's `reseal` -- has just verified it and wants the bytes, and adding
 * a second lifetime to reason about buys nothing.
 *
 * @param descriptor_path the `*.gfmodule` to read
 * @param[out] out archive path -> bytes, for every member outside META-INF/
 * @param[out] reason set on failure
 * @return whether every member could be read
 */
auto GF_CORE_EXPORT ReadModuleDescriptorResources(
    const QString& descriptor_path, QMap<QString, QByteArray>& out,
    QString& reason) -> bool;

/**
 * @brief Verify an EXTERNAL module descriptor against the key it carries.
 *
 * The external half of the split, and the shapes are disjoint on purpose: an
 * integrated descriptor must not carry a key and is checked against the one
 * compiled into this Host; an external descriptor must carry one and is
 * checked against that. So a descriptor of either kind moved into the other's
 * directory is refused by structure, with no policy comparison involved.
 *
 * @warning A successful return does NOT mean the module may be loaded. It
 * means the descriptor is internally sound and names the key that signed it.
 * Whether that key is trusted, and whether the user has enabled this module,
 * are separate decisions made above this layer -- and both are required.
 *
 * `build_id` is not compared: it names the build tree that produced the
 * module, which is not this one. Compatibility is carried by `sdk_abi` and
 * `min_host_version`, which are checked exactly as they are for an integrated
 * descriptor.
 *
 * @return the verdict; on success @c build_public_key is the carried key, and
 * its fingerprint is what a person is asked about.
 */
auto GF_CORE_EXPORT VerifyExternalModuleDescriptor(const QString& package_path)
    -> ModuleDescriptorVerification;

/**
 * @brief Verify an INTEGRATED module descriptor.
 *
 * @warning There is no key parameter, and there must not be one. This
 * function always verifies against ModuleBuildPublicKey() and always requires
 * ModuleBuildId(), so no caller can ask for a verification on any other
 * terms. It used to take a defaulted key, which meant the integrated trust
 * root was a caller's argument -- a thing that can be passed wrongly -- when
 * it is properly a property of the build.
 *
 * ## The invariant this exists to hold
 *
 * The native-binding policy may relax what a descriptor says about its entry
 * native. It may never relax what proves the descriptor itself. Whatever
 * GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING is set to, an integrated
 * module is eligible to load only if all of this holds first:
 *
 * ```
 * signature verifies against this Host build's embedded Ed25519 public key
 * build_id equals this Host build's identity
 * it came from the Host-controlled integrated namespace
 * namespace key, path containment, file type and image header all pass
 * ```
 *
 * Only then does the binding policy apply, and it governs one thing: whether
 * the descriptor must also bind the bytes of its entry.
 */
auto GF_CORE_EXPORT VerifyModuleDescriptor(const QString& package_path)
    -> ModuleDescriptorVerification;

}  // namespace GpgFrontend::Module
