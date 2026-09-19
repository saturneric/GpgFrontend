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

/**
 * @file ModuleEntryBinding.h
 * @brief Binding a descriptor to the external native library it names.
 *
 * This layer knows about logical names, the Host's canonical layout,
 * containment and verification modes. It knows nothing about QLibrary,
 * dlopen or LoadLibraryExW: it hands back a path that has been verified, and
 * the loader above it does the rest.
 *
 * ## Why the mode is platform-specific
 *
 * A full-file digest is right on Linux and wrong on the other two. Windows
 * Authenticode signing appends a certificate table and rewrites the checksum;
 * macOS code signing rewrites `__LINKEDIT` and the App Store may re-sign on
 * download. Binding to raw bytes there would either forbid normal platform
 * signing or force the descriptor to be regenerated after it -- and on macOS,
 * regenerating it after signing is exactly what would drag a build-produced
 * packager into a privileged signing job.
 *
 * So each platform binds what is stable on it:
 *
 * ```
 * linux    file-sha256             the exact final ELF bytes
 * windows  pe-authenticode-sha256  PE image content, certificates excluded
 * macos    apple-binding-id        an id embedded before Apple signing;
 *                                  Apple's own signature covers the content
 * ```
 *
 * The common contract is that the descriptor strongly binds the entry
 * native's identity. The mechanism is deliberately not the same everywhere.
 */

/// Whether these leading bytes look like a shared library this platform could
/// map. A cheap sanity filter that keeps text files, scripts and truncated
/// downloads away from the loader; it says nothing about who produced the file.
auto GF_CORE_EXPORT HasNativeImageHeader(const QByteArray& header) -> bool;

/**
 * @brief What a binding is computed relative to.
 *
 * Only `apple-binding-id` uses it, and it is a required argument rather than
 * an optional one so that adding a mode which needs identity cannot silently
 * get an empty one.
 */
struct GF_CORE_EXPORT ModuleEntryBindingContext {
  QString module_id;
  QString build_id;
  int sdk_abi = 0;
};

/**
 * @brief The GpgFrontend binding id for a module in a build.
 *
 * Not secret, not a signature, and not derived from the file: it is a name for
 * "this module, in this build", computed from three public values so that a
 * test, CMake and the runtime all reach the same answer independently.
 *
 * ```
 * SHA-256( "GpgFrontend.ModuleBinding.v1" 0x00
 *          module_id                      0x00
 *          build_id                       0x00
 *          sdk_abi as ASCII decimal )
 * ```
 *
 * The domain prefix and the version in it make the scheme replaceable without
 * ambiguity. The separators are single NULs and there are no length prefixes,
 * which is safe here only because none of the three inputs may contain a NUL:
 * a module id and a build id are both constrained character sets, and the ABI
 * is a number.
 */
auto GF_CORE_EXPORT
ModuleEntryBindingId(const ModuleEntryBindingContext& context) -> QString;

/**
 * @brief Compute what a descriptor should record for an entry native.
 *
 * The producer's half of the contract, and the same function the verifier
 * uses, so "the build tool and the runtime agree" is a property of there
 * being one implementation rather than of a test that two of them match.
 *
 * @param mode which binding applies; fixed by the target platform
 * @param context the identity `apple-binding-id` is derived from; required
 * rather than optional so a mode that needs it cannot silently get an empty
 * one
 * @param native_path the finished native file, after all platform preparation
 * @param out set to the value on success, 64 lower-case hex characters
 * @param reason set on failure, to something worth showing a person
 * @return whether a value could be computed
 */
auto GF_CORE_EXPORT ComputeEntryVerificationValue(
    ModuleEntryVerificationMode mode, const QString& native_path,
    const ModuleEntryBindingContext& context, QString& out, QString& reason)
    -> bool;

/**
 * @brief The Authenticode image digest of a PE file.
 *
 * Not a digest of the file. It skips exactly the three regions that
 * Authenticode signing and timestamping write, per Microsoft's *Windows
 * Authenticode Portable Executable Signature Format*:
 *
 *  1. the `CheckSum` field in the Optional Header (4 bytes);
 *  2. the Certificate Table entry in the Data Directory (8 bytes);
 *  3. the attribute certificate table itself, at the end of the file.
 *
 * That is what lets a module DLL be signed after its descriptor is final, by
 * CI or by hand, without the binding breaking and without the build key being
 * needed again.
 *
 * Exposed for testing. Everything else should go through
 * ComputeEntryVerificationValue(), so the producer and the verifier are
 * calling one implementation rather than two that agree today.
 */
auto GF_CORE_EXPORT PeAuthenticodeDigest(const QByteArray& pe_bytes,
                                         QString& reason) -> QString;

/**
 * @brief Read the GpgFrontend binding section out of a Mach-O, without loading
 * it.
 *
 * Walks load commands rather than trusting file offsets, so it is indifferent
 * to `codesign` having appended to `__LINKEDIT` and rewritten them. A
 * universal (fat) binary is refused rather than parsed: no build in this
 * matrix produces one, so accepting it would mean guessing which slice was
 * authoritative.
 */
auto GF_CORE_EXPORT MachOBindingSection(const QByteArray& macho_bytes,
                                        QString& reason) -> QString;

/**
 * @brief The directory a module's native files live in.
 *
 * A value type rather than a bare QString so that this layer cannot be handed
 * an arbitrary path by accident, and so a test can point it somewhere else
 * without that looking like ordinary string handling.
 */
struct GF_CORE_EXPORT ModuleNativeRoot {
  QString path;
};

/// A descriptor's logical entry name, as a filename on this platform.
///
/// `gf_mod_email` becomes `libgf_mod_email.so`, `gf_mod_email.dylib` or
/// `gf_mod_email.dll` depending on what built it. The affixes come from CMake
/// (GF_SHARED_LIBRARY_PREFIX/SUFFIX), not from an #ifdef on the operating
/// system: MinGW and MSVC are both "windows" and disagree about the prefix.
auto GF_CORE_EXPORT ModuleNativeFileName(const QString& logical_name)
    -> QString;

/// Why an entry native was refused.
enum class ModuleEntryStatus {
  kOK,
  kBAD_ENTRY_NAME,               ///< not a logical name at all
  kNATIVE_PATH_ESCAPE,           ///< it resolved outside the native root
  kMISSING_ENTRY_NATIVE,         ///< the file is not there
  kBAD_NATIVE_FILE_TYPE,         ///< a symlink, a directory, or not an image
  kENTRY_VERIFICATION_MISMATCH,  ///< it is not the one the descriptor binds
  kENTRY_BINDING_ABSENT,         ///< macOS: the Mach-O carries no binding
  kIO_FAILED,
};

auto GF_CORE_EXPORT ModuleEntryStatusToString(ModuleEntryStatus status) -> const
    char*;

/// What resolving and verifying a descriptor's entry native concluded.
struct GF_CORE_EXPORT VerifiedNativeEntry {
  bool ok = false;
  ModuleEntryStatus status = ModuleEntryStatus::kOK;
  QString reason;

  /// Absolute, verified, and safe to hand a loader. Empty unless @c ok.
  QString path;
};

/**
 * @brief Find the entry native a descriptor names, and prove it is the one.
 *
 * The descriptor never says where its native lives -- it cannot, because a
 * logical name has no room for a path. This is the only place the mapping
 * happens, which is what keeps a package from influencing a loader's search.
 *
 * In order: validate the logical name; map it to a filename; resolve it inside
 * @p root and prove it did not escape; require a regular file with a native
 * image header; then check it under the mode the manifest's platform mandates.
 *
 * @note There is a window between this returning and a loader opening the
 * path. It is documented rather than closed: anyone able to write into the
 * native root can already replace the Host binary itself, and closing it would
 * need either a non-portable fdlopen() or loading through /proc/self/fd, which
 * reintroduces the $ORIGIN problem that externalising the native removed. On
 * macOS the window is closed by Apple's own signature, which dyld validates in
 * the kernel at map time.
 */
auto GF_CORE_EXPORT ResolveAndVerifyNativeEntry(const ModuleManifest& manifest,
                                                const ModuleNativeRoot& root)
    -> VerifiedNativeEntry;

}  // namespace GpgFrontend::Module
