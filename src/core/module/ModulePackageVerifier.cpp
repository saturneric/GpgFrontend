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
#include <array>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/GFBufferFactory.h"
#include "core/module/ModuleLoadStats.h"
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

  // The seam publisher trust will use, present and unexercised. When the Host
  // supplies its embedded key, the key inside the package stops being the
  // thing trusted and becomes a value that has to match -- and at that point
  // the member itself goes, because a trust root that travels with what it
  // vouches for vouches for nothing.
  if (!expected_public_key.isEmpty() &&
      public_key_bytes != expected_public_key) {
    return Refuse(ModulePackageStatus::kBAD_SIGNATURE,
                  "it was not signed by the expected key");
  }

  const auto parsed = ParseModuleManifest(manifest_bytes);
  if (!parsed.ok) {
    return Refuse(parsed.status == ModuleManifestStatus::kTOO_NEW
                      ? ModulePackageStatus::kTOO_NEW
                      : ModulePackageStatus::kMALFORMED,
                  parsed.reason);
  }
  const auto& m = parsed.manifest;

  if (m.platform_os != ManifestHostOsName() ||
      m.platform_arch != QSysInfo::currentCpuArchitecture()) {
    return Refuse(ModulePackageStatus::kWRONG_PLATFORM,
                  QString("it was built for %1/%2, and this is %3/%4")
                      .arg(m.platform_os, m.platform_arch, ManifestHostOsName(),
                           QSysInfo::currentCpuArchitecture()));
  }

  // One decision point, shared with the loader's check of the module's own
  // table -- see SdkAbiRejection().
  if (const auto why = SdkAbiRejection(m.sdk_abi); why) {
    return Refuse(ModulePackageStatus::kINCOMPATIBLE_ABI, *why);
  }

  if (GFCompareSoftwareVersion(m.min_host_version, GetProjectVersion()) > 0) {
    return Refuse(ModulePackageStatus::kINCOMPATIBLE_ABI,
                  QString("it needs GpgFrontend %1 or newer, and this is %2")
                      .arg(m.min_host_version, GetProjectVersion()));
  }

  // Both directions, over the package's non-executable members. A declared
  // resource that is absent is a broken descriptor; an undeclared member that
  // is present is an appended payload the signature says nothing about, which
  // is the more interesting of the two.
  for (const auto& declared : m.resources) {
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
  if (actual_digests.size() != m.resources.size()) {
    QStringList declared_paths;
    declared_paths.reserve(m.resources.size());
    for (const auto& f : m.resources) declared_paths.append(f.path);
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
  }
  return "unknown";
}

namespace {

/// Verify a descriptor, from one immutable snapshot of its bytes.
///
/// Nothing executable is retained, because nothing executable is in here: the
/// entry native is an external file, and binding it is ResolveAndVerify\
/// NativeEntry()'s job, one layer up. This function knows about archives,
/// manifests, signatures and resources, and deliberately knows nothing about
/// how a library is found or loaded.
auto ReadPackage(const QString& package_path,
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

  // Read once, whole, and judged from that one snapshot. Nothing below reopens
  // the path, so what the signature covers and what the digests are computed
  // over cannot be two different files -- and a package replaced on disk
  // midway through verification cannot be half of each.
  //
  // Affordable because this is a package: the ceiling below is the same one
  // the walk enforces per entry, so a file too large to be one of these is
  // refused before it is held rather than after.
  if (!package.open(QIODevice::ReadOnly)) {
    return Refuse(ModulePackageStatus::kIO_FAILED,
                  "this file could not be "
                  "read");
  }
  if (package.size() > kMaxPackageTotalBytes) {
    return Refuse(ModulePackageStatus::kMALFORMED,
                  "this file is larger than a module package may be");
  }
  const auto package_bytes = package.readAll();
  package.close();
  if (package_bytes.isEmpty()) {
    return Refuse(ModulePackageStatus::kNOT_A_PACKAGE, "this file is empty");
  }

  QByteArray manifest_bytes;
  QByteArray signature_bytes;
  QByteArray public_key_bytes;
  int manifest_count = 0;
  int signature_count = 0;
  int public_key_count = 0;

  // Path -> digest of what the package actually holds. Filled as the walk
  // streams, so no entry is kept beyond the moment it is hashed -- except the
  // single binary a retaining caller asked for.
  QMap<QString, QString> actual_digests;
  bool hash_failed = false;

  QString reason;
  // No destination, no disk writer, no filter that has to remember to claim
  // everything. The safety property -- no byte of an unverified package ever
  // reaches a filesystem -- is now a fact about the function being called
  // rather than a promise about how a general extractor is being used.
  //
  // The sink takes ordinary memory. A native module image is tens of megabytes
  // and is not a secret; the secure tier is locked, guarded pages whose budget
  // one such image would exhaust on its own.
  const auto error = ArchiveFileOperator::ReadArchiveMembersSync(
      package_bytes, PackagePolicy(),
      [&](const QString& path, const QByteArray& bytes) {
        if (path == kModulePackageManifestPath) {
          ++manifest_count;
          manifest_bytes = bytes;
          return true;
        }
        if (path == kModulePackageSignaturePath) {
          ++signature_count;
          signature_bytes = bytes;
          return true;
        }
        if (path == kModulePackageBuildKeyPath) {
          ++public_key_count;
          public_key_bytes = bytes;
          return true;
        }

        ModuleLoadStats::GetInstance().AddHashedBytes(
            static_cast<qint64>(bytes.size()));
        const auto digest = GFBufferFactory::Sha256Hex(bytes);
        if (digest.isEmpty()) {
          hash_failed = true;
          return false;
        }
        actual_digests.insert(path, digest);

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

  return conclusion;
}

}  // namespace

auto VerifyModulePackage(const QString& package_path,
                         const QByteArray& expected_public_key)
    -> ModulePackageVerification {
  return ReadPackage(package_path, expected_public_key);
}

}  // namespace GpgFrontend::Module
