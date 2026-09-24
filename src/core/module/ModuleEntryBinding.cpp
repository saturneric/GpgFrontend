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

#include "core/module/ModuleEntryBinding.h"

#if defined(Q_OS_WINDOWS)
// <windows.h> before <imagehlp.h>: the latter is not self-contained. Both are
// only needed for the Authenticode image digest, which is the one value here
// the operating system computes rather than this file.
#include <windows.h>
// clang-format off
#include <imagehlp.h>
// clang-format on
#else
#include <sys/stat.h>
#endif

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

#include "GpgFrontendBuildInstallInfo.h"
#include "core/function/GFBufferFactory.h"
#include "core/module/ModuleLoadStats.h"

namespace GpgFrontend::Module {

namespace {

auto Refuse(ModuleEntryStatus status, QString reason) -> VerifiedNativeEntry {
  VerifiedNativeEntry v;
  v.ok = false;
  v.status = status;
  v.reason = std::move(reason);
  return v;
}

/// The same logical-name rule the manifest parser applies.
///
/// Checked twice on purpose: the parser refuses a descriptor that carries a
/// bad name, and this refuses to build a path out of one. A name that reached
/// here without passing the parser would mean the two had drifted, and the
/// cost of asking again is a loop over at most sixty-four characters.
auto IsLogicalNativeName(const QString& s) -> bool {
  if (s.isEmpty() || s.size() > 64) return false;
  if (s.front() < u'a' || s.front() > u'z') return false;
  for (const auto c : s) {
    const auto ch = c.unicode();
    const auto ok =
        (ch >= u'a' && ch <= u'z') || (ch >= u'0' && ch <= u'9') || ch == u'_';
    if (!ok) return false;
  }
  return !s.startsWith("lib");
}

}  // namespace

auto HasNativeImageHeader(const QByteArray& header) -> bool {
#if defined(Q_OS_WINDOWS)
  return header.size() >= 2 && header.startsWith("MZ");
#elif defined(Q_OS_MACOS)
  if (header.size() < 4) return false;

  const auto magic = static_cast<quint32>(
      (static_cast<quint8>(header[0]) << 24) |
      (static_cast<quint8>(header[1]) << 16) |
      (static_cast<quint8>(header[2]) << 8) | static_cast<quint8>(header[3]));

  // thin Mach-O in both endiannesses, plus a fat/universal archive
  return magic == 0xFEEDFACE || magic == 0xFEEDFACF || magic == 0xCEFAEDFE ||
         magic == 0xCFFAEDFE || magic == 0xCAFEBABE || magic == 0xBEBAFECA;
#else
  return header.size() >= 4 && header[0] == '\x7f' && header[1] == 'E' &&
         header[2] == 'L' && header[3] == 'F';
#endif
}

auto ModuleNativeFileName(const QString& logical_name) -> QString {
  if (logical_name.isEmpty()) return {};
  return QString(GF_SHARED_LIBRARY_PREFIX) + logical_name +
         GF_SHARED_LIBRARY_SUFFIX;
}

auto ModuleEntryStatusToString(ModuleEntryStatus status) -> const char* {
  switch (status) {
    case ModuleEntryStatus::kOK:
      return "ok";
    case ModuleEntryStatus::kBAD_ENTRY_NAME:
      return "its entry name is not a usable one";
    case ModuleEntryStatus::kNATIVE_PATH_ESCAPE:
      return "its entry resolved outside the module native directory";
    case ModuleEntryStatus::kMISSING_ENTRY_NATIVE:
      return "the library it names is not installed";
    case ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE:
      return "what it names is not a loadable library";
    case ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH:
      return "the library it names is not the one it was signed for";
    case ModuleEntryStatus::kENTRY_BINDING_ABSENT:
      return "its descriptor binds no library, and one was required";
    case ModuleEntryStatus::kIO_FAILED:
      return "the library it names could not be read";
  }
  return "unknown";
}

auto ResolveNativeEntry(const ModuleManifest& manifest,
                        const ModuleNativeRoot& root) -> VerifiedNativeEntry {
  const auto& entry = manifest.entry_native;

  if (!IsLogicalNativeName(entry.name)) {
    return Refuse(
        ModuleEntryStatus::kBAD_ENTRY_NAME,
        QString("\"%1\" isn't a valid module file name").arg(entry.name));
  }
  if (root.path.isEmpty()) {
    return Refuse(ModuleEntryStatus::kIO_FAILED,
                  "no module folder was given to look in");
  }

  // Built from a validated name and a directory this process chose. A logical
  // name cannot contain a separator, a dot or a drive letter, so there is
  // nothing here to traverse with -- the check below is against a symlink or
  // a root that was itself surprising, not against the descriptor.
  const QDir dir(root.path);
  const auto path = dir.absoluteFilePath(ModuleNativeFileName(entry.name));

  const QFileInfo info(path);
  if (!info.exists()) {
    return Refuse(ModuleEntryStatus::kMISSING_ENTRY_NATIVE,
                  QString("\"%1\" is not installed").arg(info.fileName()));
  }

  // isSymLink() before canonicalFilePath(), because canonicalising follows
  // the link and would report the target as though it had been named
  // directly. A link here is refused outright rather than followed: the
  // descriptor names a file in this directory, and anything else is a
  // different file wearing its name.
  if (info.isSymLink() || !info.isFile()) {
    // Say WHICH, and for a link say where it points. "is not a regular file"
    // covers three quite different situations -- a symlink, a directory, and
    // something exotic -- and a reader who cannot tell them apart cannot tell
    // a packaging mistake from a build-system one. This cost a CI round trip.
    QString what;
    if (info.isSymLink()) {
      what = QString("a symlink to \"%1\"").arg(info.symLinkTarget());
    } else if (info.isDir()) {
      what = "a directory";
    } else {
      what = "not a regular file";
    }
    return Refuse(ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE,
                  QString("\"%1\" is %2").arg(info.fileName(), what));
  }

  const auto canonical_root = QFileInfo(root.path).canonicalFilePath();
  const auto canonical_file = info.canonicalFilePath();
  if (canonical_root.isEmpty() || canonical_file.isEmpty() ||
      QFileInfo(canonical_file).absolutePath() != canonical_root) {
    return Refuse(ModuleEntryStatus::kNATIVE_PATH_ESCAPE,
                  QString("\"%1\" isn't located where this module expects it")
                      .arg(info.fileName()));
  }

  {
    QFile file(canonical_file);
    if (!file.open(QIODevice::ReadOnly)) {
      return Refuse(ModuleEntryStatus::kIO_FAILED,
                    QString("\"%1\" could not be read").arg(info.fileName()));
    }
    const auto header = file.read(8);
    file.close();
    if (!HasNativeImageHeader(header)) {
      return Refuse(
          ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE,
          QString("\"%1\" is not a loadable library").arg(info.fileName()));
    }
  }

  VerifiedNativeEntry resolved;
  resolved.ok = true;
  resolved.path = canonical_file;
  return resolved;
}

auto ResolveAndVerifyNativeEntry(const ModuleManifest& manifest,
                                 const ModuleNativeRoot& root,
                                 const ModuleEntryTrustPolicy& policy)
    -> VerifiedNativeEntry {
  const auto& entry = manifest.entry_native;

  const auto resolved = ResolveNativeEntry(manifest, root);
  if (!resolved.ok) return resolved;

  const auto& canonical_file = resolved.path;
  const QFileInfo info(canonical_file);

  // Taken BEFORE the bytes are hashed and compared again after, so a file
  // replaced while it was being hashed is caught here, and one replaced later
  // is caught by the loader against this same value.
  const auto identity = CaptureModuleFileIdentity(canonical_file);

  // Asked AFTER resolution, deliberately. A module whose file is missing must
  // be reported as missing, not as unbound: the two call for different fixes
  // by different people.
  if (!entry.verification.has_value()) {
    if (policy.BindingRequired()) {
      return Refuse(
          ModuleEntryStatus::kENTRY_BINDING_ABSENT,
          QString("this module doesn't include the security information "
                  "needed to verify \"%1\", which %2 modules are required "
                  "to have")
              .arg(info.fileName(),
                   QString::fromLatin1(ModuleOriginToString(policy.origin))));
    }

    // Accepted, and worth being precise about what was just accepted. The
    // structural checks above have run and passed: this is a regular file,
    // not a symlink, directly inside this module's native directory, with a
    // native image header. What has NOT been established is that it is the
    // right library -- a well-formed image of the correct name is
    // indistinguishable from the intended one here. That is the cost of a
    // relaxed integrated policy, and the Host build chose it.
    VerifiedNativeEntry unbound;
    unbound.ok = true;
    unbound.status = ModuleEntryStatus::kOK;
    unbound.path = canonical_file;
    unbound.identity = identity;
    return unbound;
  }

  // Present, so checked -- whatever the policy says. "You need not prove
  // this" and "ignore the proof you were given" are different sentences, and
  // only the first is ever true here.
  const auto& verification = *entry.verification;

  // Cheap, and only where it means anything: the parser permits a size only
  // under file-sha256, because Authenticode signing legitimately changes the
  // size of the other. It is an early exit, never a proof.
  if (verification.size >= 0 && info.size() != verification.size) {
    // A smaller file is worth naming, because there is one overwhelmingly
    // likely cause and a packager cannot be expected to guess it: `dh_strip`
    // and `rpmbuild` strip installed binaries by default, after `make install`
    // has finished and where no install rule can see it. Where the binding
    // covers the exact ELF bytes, stripping breaks every module -- and the
    // symptom is an application that starts perfectly well with no features.
    const auto hint =
        info.size() < verification.size
            ? QString(
                  "; it is smaller than the descriptor records, which is what "
                  "stripping it after installation looks like")
            : QString();
    return Refuse(ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH,
                  QString("\"%1\" is %2 bytes, and this descriptor was "
                          "signed for %3%4")
                      .arg(info.fileName())
                      .arg(info.size())
                      .arg(verification.size)
                      .arg(hint));
  }

  // Counted here because this is now where the bytes are. The descriptor
  // carries no payload, so the resource walk hashes almost nothing; the entry
  // native is the whole of the cost, and a startup figure that omitted it
  // would report a few kilobytes for work measured in tens of megabytes.
  ModuleLoadStats::GetInstance().AddHashedBytes(info.size());

  QString actual;
  QString why;
  if (!ComputeEntryVerificationValue(verification.mode, canonical_file, actual,
                                     why)) {
    return Refuse(ModuleEntryStatus::kIO_FAILED, why);
  }
  if (actual != verification.value) {
    return Refuse(ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH,
                  QString("\"%1\" isn't the file this module was signed for")
                      .arg(info.fileName()));
  }

  if (CaptureModuleFileIdentity(canonical_file) != identity) {
    return Refuse(ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH,
                  QString("\"%1\" changed while it was being verified")
                      .arg(info.fileName()));
  }

  VerifiedNativeEntry v;
  v.ok = true;
  v.status = ModuleEntryStatus::kOK;
  v.path = canonical_file;
  v.identity = identity;
  return v;
}

auto CaptureModuleFileIdentity(const QString& path) -> ModuleFileIdentity {
  ModuleFileIdentity id;
#if defined(Q_OS_WINDOWS)
  const QFileInfo info(path);
  if (!info.exists() || !info.isFile()) return id;
  id.size = info.size();
  id.modified_ms =
      info.fileTime(QFileDevice::FileModificationTime).toMSecsSinceEpoch();
#else
  struct stat st{};
  if (::stat(QFile::encodeName(path).constData(), &st) != 0 ||
      !S_ISREG(st.st_mode)) {
    return id;
  }
  id.size = static_cast<qint64>(st.st_size);
#if defined(Q_OS_MACOS)
  id.modified_ms = static_cast<qint64>(st.st_mtimespec.tv_sec) * 1000 +
                   st.st_mtimespec.tv_nsec / 1000000;
#else
  id.modified_ms = static_cast<qint64>(st.st_mtim.tv_sec) * 1000 +
                   st.st_mtim.tv_nsec / 1000000;
#endif
  id.file_id = static_cast<quint64>(st.st_ino);
#endif
  return id;
}

namespace {

#if defined(Q_OS_WINDOWS)
/// Receives the image bytes ImageGetDigestStream() decides are covered.
auto WINAPI PeDigestSink(DIGEST_HANDLE handle, PBYTE data, DWORD length)
    -> BOOL {
  auto* hash = reinterpret_cast<QCryptographicHash*>(handle);
  hash->addData(QByteArray(reinterpret_cast<const char*>(data),
                           static_cast<qsizetype>(length)));
  return TRUE;
}
#endif

}  // namespace

auto PeAuthenticodeDigestOfFile(const QString& path, QString& reason)
    -> QString {
#if defined(Q_OS_WINDOWS)
  const auto handle = CreateFileW(
      reinterpret_cast<const wchar_t*>(path.utf16()), GENERIC_READ,
      FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    reason = QString("\"%1\" could not be opened to hash it").arg(path);
    return {};
  }

  QCryptographicHash hash(QCryptographicHash::Sha256);
  // DigestLevel 0: the Authenticode image digest. The CERT_PE_IMAGE_DIGEST_*
  // flags widen it to cover debug info, resources and import tables, none of
  // which Authenticode includes.
  const auto ok = ImageGetDigestStream(handle, 0, PeDigestSink, &hash);
  CloseHandle(handle);

  if (ok == FALSE) {
    reason = QString("\"%1\" is not a PE image this can hash").arg(path);
    return {};
  }
  return QString::fromLatin1(hash.result().toHex());
#else
  Q_UNUSED(path)
  // Unreachable in production: a descriptor declaring `windows` is refused on
  // another platform by the platform check, long before a binding is
  // computed. Refusing rather than guessing keeps that true if it ever stops
  // being.
  reason =
      "a PE image digest can only be computed on Windows, where the operating "
      "system provides it";
  return {};
#endif
}

auto ComputeEntryVerificationValue(ModuleEntryVerificationMode mode,
                                   const QString& native_path, QString& out,
                                   QString& reason) -> bool {
  // No default arm. The switch is exhaustive over the enumerators, so adding
  // a mode is a compile error here rather than a runtime string nobody reads
  // until a package cannot be built.
  switch (mode) {
    case ModuleEntryVerificationMode::kFILE_SHA256: {
      const auto digest = GFBufferFactory::Sha256HexOfFile(native_path);
      if (digest.isEmpty()) {
        reason = QString("\"%1\" could not be read").arg(native_path);
        return false;
      }
      out = digest;
      return true;
    }

    case ModuleEntryVerificationMode::kPE_AUTHENTICODE_SHA256: {
      // The OS reads the file; nothing here pulls a whole DLL into memory to
      // hash it.
      const auto digest = PeAuthenticodeDigestOfFile(native_path, reason);
      if (digest.isEmpty()) return false;
      out = digest;
      return true;
    }
  }

  reason = "unknown entry verification mode";
  return false;
}

}  // namespace GpgFrontend::Module
