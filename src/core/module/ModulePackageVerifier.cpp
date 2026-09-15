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

#include "ModulePackageVerifier.h"

#include <sodium.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>
#include <array>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/GFBufferFactory.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "sdk/GFSDKBuildInfo.h"

namespace GpgFrontend::Module {

namespace {

/// Ceilings for the verification walk. Generous enough for a real module and
/// far below anything that could exhaust memory: every entry is read into
/// memory here, one at a time, because none of them are allowed to reach a
/// filesystem before the package has been judged.
constexpr qint64 kMaxPackageEntryBytes = 512LL * 1024 * 1024;
constexpr qint64 kMaxPackageTotalBytes = 1024LL * 1024 * 1024;
constexpr int kMaxPackageEntries = 4096;
constexpr qint64 kMaxPackageCompressionRatio = 200;

/**
 * @brief The policy a module package is walked under.
 *
 * Strict() already refuses links, non-regular entries, traversal, absolute
 * paths and the rest. What is added here is what a package needs and an
 * arbitrary archive does not: names that mean one thing everywhere, appearing
 * once each, and a bound on how cheap it is to ask for a lot of data.
 */
auto PackagePolicy() -> ArchiveExtractPolicy {
  auto policy =
      ArchiveExtractPolicy::Strict(kMaxPackageTotalBytes, kMaxPackageEntries);
  policy.max_entry_bytes = kMaxPackageEntryBytes;
  policy.reject_duplicate_paths = true;
  policy.reject_case_colliding_paths = true;
  policy.require_utf8_paths = true;
  policy.max_compression_ratio = kMaxPackageCompressionRatio;
  // Nothing is ever written: every entry is diverted. The destination is a
  // throwaway directory the extractor insists on having, so requiring it to be
  // empty would only make it delete a directory it never touched.
  policy.require_empty_destination = false;
  return policy;
}

auto Refuse(ModulePackageStatus status, const QString& reason)
    -> ModulePackageVerification {
  ModulePackageVerification v;
  v.ok = false;
  v.status = status;
  v.reason = reason;
  return v;
}

/// The os string a manifest must carry to run here.
auto HostOsName() -> QString {
#if defined(Q_OS_WIN)
  return "windows";
#elif defined(Q_OS_MACOS)
  return "macos";
#elif defined(Q_OS_LINUX)
  return "linux";
#else
  return QSysInfo::kernelType();
#endif
}

auto ToHex(const GFBuffer& digest) -> QString {
  return QString::fromLatin1(digest.ConvertToQByteArray().toHex());
}

/**
 * @brief Everything that is true of a verified package, wherever it came from.
 *
 * Shared by the archive and the extracted-tree entry points on purpose: an
 * installed module is re-checked by exactly the rules its package passed, so
 * the two cannot drift into disagreeing about what "verified" means.
 *
 * @param manifest_bytes the manifest exactly as stored
 * @param signature_bytes the detached signature over those bytes
 * @param public_key_bytes the key found alongside them
 * @param counts how many of each META-INF member were seen
 * @param actual_digests every non-META-INF file found, and its digest
 * @param expected_public_key when non-empty, the key that MUST have been used
 * @return the verdict
 */
auto ConcludeVerification(const QByteArray& manifest_bytes,
                          const QByteArray& signature_bytes,
                          const QByteArray& public_key_bytes,
                          const std::array<int, 3>& counts,
                          const QMap<QString, QString>& actual_digests,
                          const QByteArray& expected_public_key)
    -> ModulePackageVerification {
  // Exactly one of each, and each present. More than one is how an archive
  // says two different things at once; a reader that takes the first and a
  // reader that takes the last then disagree about what was signed.
  if (counts[0] != 1 || counts[1] != 1 || counts[2] != 1) {
    return Refuse(ModulePackageStatus::kMALFORMED,
                  "it does not carry exactly one manifest, one signature and "
                  "one build key");
  }
  if (signature_bytes.size() != crypto_sign_BYTES ||
      public_key_bytes.size() != crypto_sign_PUBLICKEYBYTES) {
    return Refuse(ModulePackageStatus::kMALFORMED,
                  "its signature or build key is the wrong size");
  }

  // The seam publisher trust will use, present and unexercised. When a catalog
  // supplies a key, the key inside the package stops being the thing trusted
  // and becomes a value that has to match.
  if (!expected_public_key.isEmpty() &&
      public_key_bytes != expected_public_key) {
    return Refuse(ModulePackageStatus::kBAD_SIGNATURE,
                  "it was not signed by the expected key");
  }

  // Over the bytes as stored, and BEFORE they are parsed. That ordering is
  // what removes canonicalisation from the verifier entirely: there is no
  // second serialiser here to disagree with the one that built the package,
  // and the JSON parser only ever sees bytes a signature already covers.
  if (crypto_sign_verify_detached(
          reinterpret_cast<const unsigned char*>(signature_bytes.constData()),
          reinterpret_cast<const unsigned char*>(manifest_bytes.constData()),
          static_cast<unsigned long long>(manifest_bytes.size()),
          reinterpret_cast<const unsigned char*>(
              public_key_bytes.constData())) != 0) {
    return Refuse(ModulePackageStatus::kBAD_SIGNATURE,
                  "its manifest does not match its signature");
  }

  const auto parsed = ParseModuleManifest(manifest_bytes);
  if (!parsed.ok) {
    return Refuse(parsed.status == ModuleManifestStatus::kTOO_NEW
                      ? ModulePackageStatus::kTOO_NEW
                      : ModulePackageStatus::kMALFORMED,
                  parsed.reason);
  }
  const auto& m = parsed.manifest;

  if (m.platform_os != HostOsName() ||
      m.platform_arch != QSysInfo::currentCpuArchitecture()) {
    return Refuse(ModulePackageStatus::kWRONG_PLATFORM,
                  QString("it was built for %1/%2, and this is %3/%4")
                      .arg(m.platform_os, m.platform_arch, HostOsName(),
                           QSysInfo::currentCpuArchitecture()));
  }

  if (m.sdk_abi < GF_SDK_ABI_MIN_SUPPORTED || m.sdk_abi > GF_SDK_ABI_VERSION) {
    return Refuse(ModulePackageStatus::kINCOMPATIBLE_ABI,
                  QString("it was built against sdk abi %1, and this version "
                          "of GpgFrontend supports %2 to %3")
                      .arg(m.sdk_abi)
                      .arg(GF_SDK_ABI_MIN_SUPPORTED)
                      .arg(GF_SDK_ABI_VERSION));
  }

  if (GFCompareSoftwareVersion(m.min_host_version, GetProjectVersion()) > 0) {
    return Refuse(ModulePackageStatus::kINCOMPATIBLE_ABI,
                  QString("it needs GpgFrontend %1 or newer, and this is %2")
                      .arg(m.min_host_version, GetProjectVersion()));
  }

  // Both directions. A declared file that is absent is a broken package; an
  // undeclared file that is present is an appended payload the signature says
  // nothing about, which is the more interesting of the two.
  for (const auto& declared : m.files) {
    const auto it = actual_digests.constFind(declared.path);
    if (it == actual_digests.constEnd()) {
      return Refuse(ModulePackageStatus::kMISSING_DECLARED_FILE,
                    QString("it promises \"%1\" and does not carry it")
                        .arg(declared.path));
    }
    if (*it != declared.sha256) {
      return Refuse(ModulePackageStatus::kFILE_DIGEST_MISMATCH,
                    QString("\"%1\" is not the file this package was signed "
                            "for")
                        .arg(declared.path));
    }
  }
  if (actual_digests.size() != m.files.size()) {
    QStringList declared_paths;
    declared_paths.reserve(m.files.size());
    for (const auto& f : m.files) declared_paths.append(f.path);
    for (auto it = actual_digests.constBegin(); it != actual_digests.constEnd();
         ++it) {
      if (!declared_paths.contains(it.key())) {
        return Refuse(ModulePackageStatus::kUNDECLARED_FILE,
                      QString("it carries \"%1\", which nothing in it vouches "
                              "for")
                          .arg(it.key()));
      }
    }
  }

  ModulePackageVerification v;
  v.ok = true;
  v.status = ModulePackageStatus::kOK;
  v.manifest = m;
  v.build_public_key = public_key_bytes;
  return v;
}

}  // namespace

auto ModulePackageStatusToString(ModulePackageStatus s) -> const char* {
  switch (s) {
    case ModulePackageStatus::kOK:
      return "ok";
    case ModulePackageStatus::kNOT_A_PACKAGE:
      return "not a module package";
    case ModulePackageStatus::kTOO_NEW:
      return "written by a newer version";
    case ModulePackageStatus::kMALFORMED:
      return "malformed";
    case ModulePackageStatus::kBAD_SIGNATURE:
      return "the signature does not match";
    case ModulePackageStatus::kFILE_DIGEST_MISMATCH:
      return "a file in it has changed";
    case ModulePackageStatus::kUNDECLARED_FILE:
      return "it carries a file nothing vouches for";
    case ModulePackageStatus::kMISSING_DECLARED_FILE:
      return "a file it promises is not there";
    case ModulePackageStatus::kWRONG_PLATFORM:
      return "built for a different system";
    case ModulePackageStatus::kINCOMPATIBLE_ABI:
      return "built against a different sdk";
    case ModulePackageStatus::kIO_FAILED:
      return "it could not be read";
    case ModulePackageStatus::kNOT_INSTALLED:
      return "it is not installed";
  }
  return "unknown";
}

auto VerifyModulePackage(const QString& package_path,
                         const QByteArray& expected_public_key)
    -> ModulePackageVerification {
  if (!EnsureSodiumInit()) {
    return Refuse(ModulePackageStatus::kIO_FAILED,
                  "the cryptography library could not be started");
  }

  QFile package(package_path);
  if (!package.exists()) {
    return Refuse(ModulePackageStatus::kIO_FAILED, "this file does not exist");
  }

  // The digest of the package as a whole. Nothing in this phase compares it
  // against anything -- it is what a catalog would key on -- but computing it
  // here is what keeps the catalog from needing its own pass over the file.
  QString package_sha256;
  {
    if (!package.open(QIODevice::ReadOnly)) {
      return Refuse(ModulePackageStatus::kIO_FAILED,
                    "this file could not be read");
    }
    auto digest = GFBufferFactory::ToSha256(
        [&package](const GFBufferFactory::Sha256Chunk& chunk) {
          QByteArray buf(64 * 1024, Qt::Uninitialized);
          while (true) {
            const auto n = package.read(buf.data(), buf.size());
            if (n <= 0) break;
            chunk(buf.constData(), static_cast<size_t>(n));
          }
        });
    package.close();
    if (!digest) {
      return Refuse(ModulePackageStatus::kIO_FAILED,
                    "this file could not be read");
    }
    package_sha256 = ToHex(*digest);
  }

  // Nothing is written into it. The extractor still insists on a destination,
  // and an invalid one would hand it an empty path.
  QTemporaryDir nowhere;
  if (!nowhere.isValid()) {
    return Refuse(ModulePackageStatus::kIO_FAILED,
                  "a temporary folder could not be made");
  }

  QByteArray manifest_bytes;
  QByteArray signature_bytes;
  QByteArray public_key_bytes;
  int manifest_count = 0;
  int signature_count = 0;
  int public_key_count = 0;

  // Path -> digest of what the package actually holds. Filled as the walk
  // streams, so no entry is kept beyond the moment it is hashed.
  QMap<QString, QString> actual_digests;
  bool hash_failed = false;

  QString reason;
  const auto error = ArchiveFileOperator::ExtractArchiveFromFileSync(
      package_path, nowhere.path(), PackagePolicy(),
      // Claim every entry. This is the whole safety property of this
      // function: no byte of an unverified package ever reaches a filesystem,
      // so there is nothing for a later step to accidentally execute.
      [](const QString&) { return true; },
      [&](const QString& path, const GFBuffer& bytes) {
        if (path == kModulePackageManifestPath) {
          ++manifest_count;
          manifest_bytes = bytes.ConvertToQByteArray();
          return true;
        }
        if (path == kModulePackageSignaturePath) {
          ++signature_count;
          signature_bytes = bytes.ConvertToQByteArray();
          return true;
        }
        if (path == kModulePackageBuildKeyPath) {
          ++public_key_count;
          public_key_bytes = bytes.ConvertToQByteArray();
          return true;
        }

        auto digest = GFBufferFactory::ToSha256(
            [&bytes](const GFBufferFactory::Sha256Chunk& chunk) {
              chunk(bytes.Data(), bytes.Size());
            });
        if (!digest) {
          hash_failed = true;
          return false;
        }
        actual_digests.insert(path, ToHex(*digest));
        return true;
      },
      &reason);

  if (error != 0) {
    return Refuse(ModulePackageStatus::kNOT_A_PACKAGE,
                  reason.isEmpty() ? QString("this file is not a module "
                                             "package")
                                   : reason);
  }
  if (hash_failed) {
    return Refuse(ModulePackageStatus::kIO_FAILED,
                  "a file in it could not be read");
  }

  auto conclusion =
      ConcludeVerification(manifest_bytes, signature_bytes, public_key_bytes,
                           {manifest_count, signature_count, public_key_count},
                           actual_digests, expected_public_key);
  if (!conclusion.ok) return conclusion;

  conclusion.package_sha256 = package_sha256;
  return conclusion;
}

auto VerifyExtractedModuleTree(const QString& directory,
                               const QByteArray& expected_public_key)
    -> ModulePackageVerification {
  if (!EnsureSodiumInit()) {
    return Refuse(ModulePackageStatus::kIO_FAILED,
                  "the cryptography library could not be started");
  }

  const QDir root(directory);
  if (!root.exists()) {
    return Refuse(ModulePackageStatus::kNOT_INSTALLED,
                  "this module is not installed");
  }

  const auto read_whole = [](const QString& path, QByteArray& out) -> bool {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    out = f.readAll();
    return true;
  };

  QByteArray manifest_bytes;
  QByteArray signature_bytes;
  QByteArray public_key_bytes;
  std::array<int, 3> counts{0, 0, 0};
  if (read_whole(root.filePath(kModulePackageManifestPath), manifest_bytes)) {
    counts[0] = 1;
  }
  if (read_whole(root.filePath(kModulePackageSignaturePath), signature_bytes)) {
    counts[1] = 1;
  }
  if (read_whole(root.filePath(kModulePackageBuildKeyPath), public_key_bytes)) {
    counts[2] = 1;
  }

  // Every regular file under the tree except the three META-INF members, so
  // the undeclared-file rule holds here too: something dropped into an
  // installed module is a file nothing vouches for, exactly as it would be
  // inside the package.
  QMap<QString, QString> actual_digests;
  QDirIterator it(directory, QDir::Files | QDir::NoSymLinks,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto absolute = it.next();
    const auto relative = root.relativeFilePath(absolute);
    if (relative == kModulePackageManifestPath ||
        relative == kModulePackageSignaturePath ||
        relative == kModulePackageBuildKeyPath) {
      continue;
    }

    QFile file(absolute);
    if (!file.open(QIODevice::ReadOnly)) {
      return Refuse(ModulePackageStatus::kIO_FAILED,
                    QString("\"%1\" could not be read").arg(relative));
    }
    auto digest = GFBufferFactory::ToSha256(
        [&file](const GFBufferFactory::Sha256Chunk& chunk) {
          QByteArray buf(64 * 1024, Qt::Uninitialized);
          while (true) {
            const auto n = file.read(buf.data(), buf.size());
            if (n <= 0) break;
            chunk(buf.constData(), static_cast<size_t>(n));
          }
        });
    if (!digest) {
      return Refuse(ModulePackageStatus::kIO_FAILED,
                    QString("\"%1\" could not be read").arg(relative));
    }
    actual_digests.insert(relative, ToHex(*digest));
  }

  return ConcludeVerification(manifest_bytes, signature_bytes, public_key_bytes,
                              counts, actual_digests, expected_public_key);
}

auto UnpackVerifiedModulePackage(const QString& package_path,
                                 const QString& destination)
    -> ModulePackageUnpack {
  ModulePackageUnpack result;

  const auto verification = VerifyModulePackage(package_path);
  if (!verification.ok) {
    result.ok = false;
    result.status = verification.status;
    result.reason = verification.reason;
    return result;
  }

  // Only now. Everything above read the package into memory and wrote nothing,
  // so up to this line there is no file anywhere for anything to execute.
  QString reason;
  const auto error = ArchiveFileOperator::ExtractArchiveFromFileSync(
      package_path, destination, ArchiveExtractPolicy::Strict(-1, -1), {}, {},
      &reason);
  if (error != 0) {
    result.ok = false;
    result.status = ModulePackageStatus::kIO_FAILED;
    result.reason =
        reason.isEmpty() ? QString("it could not be unpacked") : reason;
    return result;
  }

  // The manifest names what the package carries and has already been checked
  // against what it actually holds, so this picks the binary out of a list
  // that is known to be both complete and accurate.
  QString binary;
  for (const auto& file : verification.manifest.files) {
    if (!file.path.startsWith("bin/")) continue;
    if (!binary.isEmpty()) {
      result.ok = false;
      result.status = ModulePackageStatus::kMALFORMED;
      result.reason = "it carries more than one module binary";
      return result;
    }
    binary = file.path;
  }
  if (binary.isEmpty()) {
    result.ok = false;
    result.status = ModulePackageStatus::kMALFORMED;
    result.reason = "it carries no module binary";
    return result;
  }

  result.ok = true;
  result.status = ModulePackageStatus::kOK;
  result.library_path = destination + "/" + binary;
  result.manifest = verification.manifest;
  return result;
}

}  // namespace GpgFrontend::Module
