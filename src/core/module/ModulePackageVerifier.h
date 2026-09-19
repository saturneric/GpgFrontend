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

/// Where a package keeps its one native image. The only place this prefix is
/// spelled: the manager used to repeat it to re-find the binary the verifier
/// had already found.
constexpr auto kModulePackageBinaryDir = "bin/";

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

  /// The signed digest of the one `bin/` entry, found while verifying it.
  /// This is what a pre-load inspection checks the image against.
  QString library_sha256;

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

struct ModulePackageImage;

/**
 * @brief A native module image that verification has already vouched for.
 *
 * The point of the type is that it cannot be forged by accident. Only
 * ReadVerifiedModuleImage() can produce a non-empty one, so a `QByteArray` that
 * came from anywhere else -- a file someone read, a download, a test fixture --
 * has no way to reach the loader. Materialisation takes this and nothing else,
 * which is what makes "no unverified byte reaches the native loader" a property
 * of the types rather than a rule someone has to remember.
 *
 * The bytes are ordinary memory. A native library is not a secret, and the
 * secure tier is locked, guarded pages whose budget one module image would
 * exhaust by itself.
 */
class GF_CORE_EXPORT VerifiedModuleImage {
 public:
  /// An empty image, which nothing will load.
  VerifiedModuleImage() = default;

  [[nodiscard]] auto IsValid() const -> bool {
    return !library_name_.isEmpty() && !bytes_.isEmpty();
  }

  /// The library's name as the signed manifest spells it.
  [[nodiscard]] auto LibraryName() const -> QString { return library_name_; }

  [[nodiscard]] auto Bytes() const -> const QByteArray& { return bytes_; }

  [[nodiscard]] auto Size() const -> qint64 {
    return static_cast<qint64>(bytes_.size());
  }

 private:
  friend auto ReadVerifiedModuleImage(const QString&, const QByteArray&)
      -> ModulePackageImage;

  VerifiedModuleImage(QString library_name, QByteArray bytes)
      : library_name_(std::move(library_name)), bytes_(std::move(bytes)) {}

  QString library_name_;
  QByteArray bytes_;
};

/**
 * @brief What reading a package concluded, and the image if it concluded yes.
 */
struct GF_CORE_EXPORT ModulePackageImage {
  bool ok = false;
  ModulePackageStatus status = ModulePackageStatus::kOK;
  QString reason;

  ModuleManifest manifest;

  /// The signed digest of the retained image. See ModulePackageVerification.
  QString library_sha256;

  QByteArray build_public_key;

  /// Empty unless @c ok. Never populated on any refusal path.
  VerifiedModuleImage image;
};

/**
 * @brief Verify a package and keep its library, without writing anything.
 *
 * The same verification as VerifyModulePackage(), which this is the whole of:
 * every entry is diverted, so no byte of an unverified package reaches a
 * filesystem. The difference is that the one `bin/` entry is *retained* rather
 * than hashed and dropped, which is what lets a module be loaded from a package
 * without the package ever being extracted, installed or cached.
 *
 * The ordering is the safety property and is why this is one function: the
 * image is attached only after ConcludeVerification() has passed, so a package
 * that fails yields nothing that anything could map.
 *
 * @param package_path the `*.gfmodule` to read
 * @param expected_public_key when non-empty, the key the package MUST carry
 * @return the verdict, with the manifest and image filled in only on success
 */
auto GF_CORE_EXPORT ReadVerifiedModuleImage(
    const QString& package_path, const QByteArray& expected_public_key = {})
    -> ModulePackageImage;

}  // namespace GpgFrontend::Module
