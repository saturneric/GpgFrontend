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
 * ## Why the mode is platform-specific, and why macOS has none
 *
 * A full-file digest is right on Linux and wrong on Windows, where
 * Authenticode signing appends a certificate table and rewrites the checksum
 * after the descriptor is final. So each platform binds what is stable on it:
 *
 * ```
 * linux    file-sha256             the exact final ELF bytes
 * windows  pe-authenticode-sha256  PE image content, certificates excluded
 * macos    none                    see below
 * ```
 *
 * macOS carries no binding at all. Module dylibs ship inside the application
 * bundle and are signed with the application's own identity, and dyld checks
 * that in the kernel at map time; Library Validation additionally refuses any
 * code whose Team ID is not the application's. A GpgFrontend-side claim over
 * the same bytes would restate that more weakly -- ours checked once, at open
 * time, in user space -- while obstructing the re-signing Apple's own
 * distribution pipeline performs.
 *
 * ```
 * descriptor -> authenticates module metadata, resources and build identity
 * Apple      -> authenticates executable code and enforces it at load time
 * ```
 *
 * The descriptor does not cryptographically bind a macOS dylib, and nothing
 * here should be read as claiming it does.
 *
 * ## Whether a binding is required at all
 *
 * Separate from which one applies, and answered by origin plus Host build
 * policy rather than by the descriptor. See ModuleHostPolicy.h.
 */

/// Whether these leading bytes look like a shared library this platform could
/// map. A cheap sanity filter that keeps text files, scripts and truncated
/// downloads away from the loader; it says nothing about who produced the file.
auto GF_CORE_EXPORT HasNativeImageHeader(const QByteArray& header) -> bool;

/**
 * @brief Compute what a descriptor should record for an entry native.
 *
 * The producer's half of the contract, and the same function the verifier
 * uses, so "the build tool and the runtime agree" is a property of there
 * being one implementation rather than of a test that two of them match.
 *
 * @param mode which binding applies; fixed by the target platform
 * @param native_path the finished native file, after all platform preparation
 * @param out set to the value on success, 64 lower-case hex characters
 * @param reason set on failure, to something worth showing a person
 * @return whether a value could be computed
 */
auto GF_CORE_EXPORT ComputeEntryVerificationValue(
    ModuleEntryVerificationMode mode, const QString& native_path, QString& out,
    QString& reason) -> bool;

/**
 * @brief The Authenticode image digest of a PE file, from the OS.
 *
 * Windows only, and computed by `ImageGetDigestStream()` rather than by
 * parsing the file here.
 *
 * This used to be a portable PE parser, so the Linux unit suite could exercise
 * the Windows path from synthetic fixtures. That was a testing convenience
 * rather than a requirement -- both sides that compute this value in
 * production, the packager building Windows modules and the Host verifying
 * them, run on Windows -- and it was paid for in the only way that matters: a
 * signer must pad a file to an eight-byte boundary before appending a
 * certificate, that padding falls INSIDE the hashed region, and the
 * implementation disagreed with itself about it. Every module DLL failed the
 * moment it was signed, and the tests said it was right because the synthetic
 * fixture happened to be eight-byte aligned.
 *
 * Microsoft owns the specification. Now it owns the implementation too, and
 * both sides agree by construction because both call the same API.
 *
 * What replaces the deleted fixtures is the gate that already signs real
 * module DLLs with a throwaway certificate and re-verifies them, extended to
 * prove the other half: that mutating executable content IS detected.
 *
 * @return the digest as lower-case hex, or empty with @p reason set. Always
 * empty off Windows, where there is no such thing to compute.
 */
auto GF_CORE_EXPORT PeAuthenticodeDigestOfFile(const QString& path,
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
  kENTRY_BINDING_ABSENT,         ///< no binding recorded, and one was required
  kIO_FAILED,
};

auto GF_CORE_EXPORT ModuleEntryStatusToString(ModuleEntryStatus status) -> const
    char*;

/// What resolving and verifying a descriptor's entry native concluded.
/**
 * @brief Which file a path named when it was looked at.
 *
 * The library is verified by path in one phase and handed to the loader by
 * path in another, seconds later. This is what lets the second phase notice
 * that the path no longer names the bytes the first one checked.
 */
struct GF_CORE_EXPORT ModuleFileIdentity {
  qint64 size = -1;
  qint64 modified_ms = -1;
  quint64 file_id = 0;  ///< inode, or 0 where the platform has none to offer

  [[nodiscard]] auto IsValid() const -> bool { return size >= 0; }
  auto operator==(const ModuleFileIdentity& o) const -> bool {
    return size == o.size && modified_ms == o.modified_ms &&
           file_id == o.file_id;
  }
  auto operator!=(const ModuleFileIdentity& o) const -> bool {
    return !(*this == o);
  }
};

/// The identity of @p path right now; invalid when it cannot be read.
auto GF_CORE_EXPORT CaptureModuleFileIdentity(const QString& path)
    -> ModuleFileIdentity;

struct GF_CORE_EXPORT VerifiedNativeEntry {
  bool ok = false;
  ModuleEntryStatus status = ModuleEntryStatus::kOK;
  QString reason;

  /// Absolute, verified, and safe to hand a loader. Empty unless @c ok.
  QString path;

  /// The file @c path named while it was verified. The loader re-checks it
  /// immediately before mapping, and refuses a file that changed.
  ModuleFileIdentity identity;
};

/**
 * @brief Find the entry native a descriptor names, without checking its
 * binding.
 *
 * The resolution half of ResolveAndVerifyNativeEntry(): validate the logical
 * name, map it to a filename, resolve it inside @p root and prove it did not
 * escape, refuse a symlink or anything that is not a regular file, and require
 * a native image header. What it does NOT do is compare the entry's size or
 * its verification value.
 *
 * Exposed for exactly one caller: the packager's `reseal`, which exists
 * precisely because the binding is stale -- a deployment tool has just
 * rewritten the native -- and so cannot use the verifying form. It used to
 * build the path itself and settle for `isFile()`, which meant the one command
 * the release pipeline actually runs was the one that would follow a symlink,
 * accept a text file, and seal whatever it found. A resolution rule with two
 * implementations is a resolution rule the stricter half does not own.
 */
auto GF_CORE_EXPORT ResolveNativeEntry(const ModuleManifest& manifest,
                                       const ModuleNativeRoot& root)
    -> VerifiedNativeEntry;

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
 * ## What @p policy does, and what it does not do
 *
 * It answers one question: may this descriptor omit a binding? External
 * modules may never; integrated modules may, when this Host build says so.
 *
 * It does NOT make a present binding optional. A descriptor that carries
 * verification data is checked against it whatever the policy says, because
 * "you need not prove this" and "ignore the proof you were given" are
 * different sentences and only the first one is ever true here.
 *
 * ## What survives when no binding is required
 *
 * Everything structural: the name is logical, the file exists, is a regular
 * file and not a symlink, sits directly inside @p root, and carries a native
 * image header. What does NOT survive is any claim that it is the RIGHT
 * library. A structurally valid but wrong native -- another module's, an
 * older build's, any well-formed image under the expected name -- passes.
 * That is the cost of the relaxed policy, and it is stated here rather than
 * left to be inferred from an absence.
 *
 * @note There is a window between this returning and a loader opening the
 * path. It is documented rather than closed: anyone able to write into the
 * native root can already replace the Host binary itself, and closing it would
 * need either a non-portable fdlopen() or loading through /proc/self/fd, which
 * reintroduces the $ORIGIN problem that externalising the native removed. On
 * macOS the window is closed by Apple's own signature, which dyld validates in
 * the kernel at map time.
 */
auto GF_CORE_EXPORT ResolveAndVerifyNativeEntry(
    const ModuleManifest& manifest, const ModuleNativeRoot& root,
    const ModuleEntryTrustPolicy& policy) -> VerifiedNativeEntry;

}  // namespace GpgFrontend::Module
