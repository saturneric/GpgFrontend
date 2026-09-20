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

  // thin mach-o in both endiannesses, plus a fat/universal archive
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
      return "the library it names carries no binding";
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
        QString("\"%1\" is not a logical native name").arg(entry.name));
  }
  if (root.path.isEmpty()) {
    return Refuse(ModuleEntryStatus::kIO_FAILED,
                  "no module native directory was given");
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
                  QString("\"%1\" does not live in the module native "
                          "directory")
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
                                 const ModuleNativeRoot& root)
    -> VerifiedNativeEntry {
  const auto& entry = manifest.entry_native;

  const auto resolved = ResolveNativeEntry(manifest, root);
  if (!resolved.ok) return resolved;

  const auto& canonical_file = resolved.path;
  const QFileInfo info(canonical_file);

  // Cheap, and only where it means anything: the parser permits a size only
  // under file-sha256, because platform signing legitimately changes the size
  // of the other two. It is an early exit, never a proof.
  if (entry.size >= 0 && info.size() != entry.size) {
    // A smaller file is worth naming, because there is one overwhelmingly
    // likely cause and a packager cannot be expected to guess it: `dh_strip`
    // and `rpmbuild` strip installed binaries by default, after `make install`
    // has finished and where no install rule can see it. On Linux the binding
    // covers the exact ELF bytes, so stripping breaks every module -- and the
    // symptom is an application that starts perfectly well with no features.
    //
    // Saying "it is 40 bytes shorter than it should be" leaves the reader to
    // work that out. Saying it costs one sentence.
    const auto hint =
        info.size() < entry.size
            ? QString(
                  "; it is smaller than the descriptor records, which is what "
                  "stripping it after installation looks like")
            : QString();
    return Refuse(ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH,
                  QString("\"%1\" is %2 bytes, and this descriptor was "
                          "signed for %3%4")
                      .arg(info.fileName())
                      .arg(info.size())
                      .arg(entry.size)
                      .arg(hint));
  }

  // Counted here because this is now where the bytes are. The descriptor
  // carries no payload, so the resource walk hashes almost nothing; the entry
  // native is the whole of the cost, and a startup figure that omitted it
  // would report a few kilobytes for work measured in tens of megabytes.
  ModuleLoadStats::GetInstance().AddHashedBytes(info.size());

  QString actual;
  QString why;
  const ModuleEntryBindingContext context{manifest.id, manifest.build_id,
                                          manifest.sdk_abi};
  if (!ComputeEntryVerificationValue(entry.mode, canonical_file, context,
                                     actual, why)) {
    return Refuse(ModuleEntryStatus::kIO_FAILED, why);
  }
  if (actual != entry.value) {
    return Refuse(ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH,
                  QString("\"%1\" is not the library this descriptor binds")
                      .arg(info.fileName()));
  }

  VerifiedNativeEntry v;
  v.ok = true;
  v.status = ModuleEntryStatus::kOK;
  v.path = canonical_file;
  return v;
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

/// Little-endian reads, bounds-checked, because every offset in a PE or a
/// Mach-O header comes from the file being examined.
auto ReadU32(const QByteArray& b, qsizetype at, quint32& out) -> bool {
  if (at < 0 || at + 4 > b.size()) return false;
  out = static_cast<quint32>(static_cast<quint8>(b[at])) |
        (static_cast<quint32>(static_cast<quint8>(b[at + 1])) << 8) |
        (static_cast<quint32>(static_cast<quint8>(b[at + 2])) << 16) |
        (static_cast<quint32>(static_cast<quint8>(b[at + 3])) << 24);
  return true;
}

auto ReadU64(const QByteArray& b, qsizetype at, quint64& out) -> bool {
  quint32 lo = 0;
  quint32 hi = 0;
  if (!ReadU32(b, at, lo) || !ReadU32(b, at + 4, hi)) return false;
  out = static_cast<quint64>(lo) | (static_cast<quint64>(hi) << 32);
  return true;
}

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

auto MachOBindingSection(const QByteArray& macho, QString& reason) -> QString {
  const auto fail = [&reason](const QString& why) -> QString {
    reason = why;
    return {};
  };

  quint32 magic = 0;
  if (!ReadU32(macho, 0, magic)) return fail("this file has no Mach-O header");

  if (magic == 0xCAFEBABE || magic == 0xBEBAFECA) {
    // Refused rather than parsed: no build in this matrix produces a universal
    // binary, so picking a slice would be guessing which one was authoritative.
    return fail(
        "this is a universal binary, and a single-architecture one was "
        "expected");
  }

  const auto sixty_four = magic == 0xFEEDFACF;
  if (magic != 0xFEEDFACE && !sixty_four) {
    return fail("this file is not a thin Mach-O image");
  }

  quint32 command_count = 0;
  if (!ReadU32(macho, 16, command_count)) return fail("a truncated header");

  auto at = static_cast<qsizetype>(sixty_four ? 32 : 28);

  for (quint32 i = 0; i < command_count; ++i) {
    quint32 command = 0;
    quint32 command_size = 0;
    if (!ReadU32(macho, at, command) || !ReadU32(macho, at + 4, command_size) ||
        command_size < 8 || at + command_size > macho.size()) {
      return fail("a truncated load command");
    }

    constexpr quint32 kSegment32 = 0x01;
    constexpr quint32 kSegment64 = 0x19;

    if (command == kSegment64 || command == kSegment32) {
      const auto wide = command == kSegment64;
      const auto section_count_at = at + (wide ? 64 : 48);
      quint32 section_count = 0;
      if (!ReadU32(macho, section_count_at, section_count)) {
        return fail("a truncated segment command");
      }

      const auto sections_at = at + (wide ? 72 : 56);
      const auto section_size = wide ? 80 : 68;

      for (quint32 sec = 0; sec < section_count; ++sec) {
        const auto section_at =
            sections_at + static_cast<qsizetype>(sec) * section_size;
        if (section_at + section_size > macho.size()) {
          return fail("a truncated section header");
        }

        // 16 bytes of section name, then 16 of segment name, both NUL-padded
        // rather than NUL-terminated, so they are taken by length.
        const auto section_name =
            QByteArray(macho.constData() + section_at, 16);
        const auto segment_name =
            QByteArray(macho.constData() + section_at + 16, 16);

        const auto matches = [](const QByteArray& padded, const char* want) {
          return padded.left(static_cast<qsizetype>(qstrlen(want))) == want &&
                 (padded.size() == static_cast<qsizetype>(qstrlen(want)) ||
                  padded.at(static_cast<qsizetype>(qstrlen(want))) == '\0');
        };

        if (!matches(section_name, "__gf_binding") ||
            !matches(segment_name, "__GPGFRONTEND")) {
          continue;
        }

        quint64 offset = 0;
        quint64 size = 0;
        if (wide) {
          quint32 narrow_offset = 0;
          if (!ReadU64(macho, section_at + 40, size) ||
              !ReadU32(macho, section_at + 48, narrow_offset)) {
            return fail("a truncated section header");
          }
          offset = narrow_offset;
        } else {
          quint32 narrow_size = 0;
          quint32 narrow_offset = 0;
          if (!ReadU32(macho, section_at + 40, narrow_size) ||
              !ReadU32(macho, section_at + 44, narrow_offset)) {
            return fail("a truncated section header");
          }
          size = narrow_size;
          offset = narrow_offset;
        }

        // 64 lower-case hex characters, as text. Text rather than 32 raw
        // bytes because the section content is produced by the build, and a
        // build system writing exact binary is a build system with an
        // encoding bug waiting in it.
        if (size != 64 || static_cast<qint64>(offset) + 64 > macho.size()) {
          return fail("its binding section is not sixty-four characters");
        }

        const auto text =
            QString::fromLatin1(QByteArray(macho.constData() + offset, 64));
        if (!IsModuleHexDigest(text)) {
          return fail("its binding section is not a binding id");
        }
        return text;
      }
    }

    at += command_size;
  }

  return fail("it carries no GpgFrontend binding section");
}

auto ModuleEntryBindingId(const ModuleEntryBindingContext& context) -> QString {
  QByteArray input;
  input.append("GpgFrontend.ModuleBinding.v1");
  input.append('\0');
  input.append(context.module_id.toUtf8());
  input.append('\0');
  input.append(context.build_id.toUtf8());
  input.append('\0');
  input.append(QByteArray::number(context.sdk_abi));

  return QString::fromLatin1(
      QCryptographicHash::hash(input, QCryptographicHash::Sha256).toHex());
}

auto ComputeEntryVerificationValue(ModuleEntryVerificationMode mode,
                                   const QString& native_path,
                                   const ModuleEntryBindingContext& context,
                                   QString& out, QString& reason) -> bool {
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

    case ModuleEntryVerificationMode::kAPPLE_BINDING_ID: {
      // Two questions, both of which have to answer yes. What the Mach-O
      // carries has to be what this module in this build should carry --
      // otherwise a dylib from another module, signed by the same Apple team,
      // would satisfy a descriptor it has nothing to do with.
      QFile file(native_path);
      if (!file.open(QIODevice::ReadOnly)) {
        reason = QString("\"%1\" could not be read").arg(native_path);
        return false;
      }
      const auto bytes = file.readAll();
      file.close();

      const auto embedded = MachOBindingSection(bytes, reason);
      if (embedded.isEmpty()) return false;

      const auto expected = ModuleEntryBindingId(context);
      if (embedded != expected) {
        reason = QString(
                     "\"%1\" carries a binding for a different module or "
                     "build")
                     .arg(QFileInfo(native_path).fileName());
        return false;
      }

      out = expected;
      return true;
    }

    default:
      // Deliberately a loud refusal rather than a plausible-looking value.
      //
      // Both of these are real algorithms with real specifications -- the PE
      // Authenticode image digest skips the checksum, the certificate table
      // directory entry and the certificate table itself; the Apple binding
      // is an identifier read out of a `__GPGFRONTEND,__gf_binding` section
      // without loading the Mach-O -- and both need committed fixtures and
      // known-answer tests to be worth trusting. They land together, with
      // those tests, rather than as an approximation that happens to produce
      // sixty-four hexadecimal characters.
      //
      // Until then a Windows or macOS package cannot be built, which is the
      // correct failure: the alternative is one that builds and cannot be
      // verified by the Host that ships with it.
      reason = QString(
                   "the \"%1\" entry binding is not implemented yet; a "
                   "package for this platform cannot be built by this tree")
                   .arg(ModuleEntryVerificationModeKey(mode));
      return false;
  }

  reason = "unknown entry verification mode";
  return false;
}

}  // namespace GpgFrontend::Module
