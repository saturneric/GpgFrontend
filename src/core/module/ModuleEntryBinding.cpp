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

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "GpgFrontendBuildInstallInfo.h"
#include "core/function/GFBufferFactory.h"

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
    const auto ok = (ch >= u'a' && ch <= u'z') || (ch >= u'0' && ch <= u'9') ||
                    ch == u'_';
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

auto ResolveAndVerifyNativeEntry(const ModuleManifest& manifest,
                                 const ModuleNativeRoot& root)
    -> VerifiedNativeEntry {
  const auto& entry = manifest.entry_native;

  if (!IsLogicalNativeName(entry.name)) {
    return Refuse(ModuleEntryStatus::kBAD_ENTRY_NAME,
                  QString("\"%1\" is not a logical native name")
                      .arg(entry.name));
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
    return Refuse(ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE,
                  QString("\"%1\" is not a regular file")
                      .arg(info.fileName()));
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
                    QString("\"%1\" could not be read")
                        .arg(info.fileName()));
    }
    const auto header = file.read(8);
    file.close();
    if (!HasNativeImageHeader(header)) {
      return Refuse(ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE,
                    QString("\"%1\" is not a loadable library")
                        .arg(info.fileName()));
    }
  }

  // Cheap, and only where it means anything: the parser permits a size only
  // under file-sha256, because platform signing legitimately changes the size
  // of the other two. It is an early exit, never a proof.
  if (entry.size >= 0 && info.size() != entry.size) {
    return Refuse(ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH,
                  QString("\"%1\" is %2 bytes, and this descriptor was "
                          "signed for %3")
                      .arg(info.fileName())
                      .arg(info.size())
                      .arg(entry.size));
  }

  QString actual;
  QString why;
  if (!ComputeEntryVerificationValue(entry.mode, canonical_file, actual, why)) {
    return Refuse(ModuleEntryStatus::kIO_FAILED, why);
  }
  if (actual != entry.value) {
    return Refuse(
        ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH,
        QString("\"%1\" is not the library this descriptor binds")
            .arg(info.fileName()));
  }

  VerifiedNativeEntry v;
  v.ok = true;
  v.status = ModuleEntryStatus::kOK;
  v.path = canonical_file;
  return v;
}

auto ComputeEntryVerificationValue(ModuleEntryVerificationMode mode,
                                   const QString& native_path, QString& out,
                                   QString& reason) -> bool {
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

    case ModuleEntryVerificationMode::kPE_AUTHENTICODE_SHA256:
    case ModuleEntryVerificationMode::kAPPLE_BINDING_ID:
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
