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

#include "ModuleStore.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/GFBufferFactory.h"

namespace GpgFrontend::Module {

namespace {

/// The store's own bookkeeping, one file per module.
constexpr auto kStateFileName = "state.json";
constexpr auto kVersionsDirName = "versions";

/// Scratch directories are dot-prefixed so the version scan never sees a
/// half-extracted tree as an installed version.
constexpr auto kScratchPrefix = ".staging-";

/// Package digest -> module id, so an already-installed package can be
/// recognised without walking its archive.
constexpr auto kIndexFileName = "index.json";

auto Fail(ModulePackageStatus status, const QString& reason)
    -> ModuleInstallResult {
  ModuleInstallResult r;
  r.ok = false;
  r.status = status;
  r.reason = reason;
  return r;
}

/**
 * @brief A module id turned into one safe path component.
 *
 * A module id is a reverse-dns string the module chooses, so it reaches here
 * as untrusted text and must not be able to name a directory of its own
 * choosing. Anything outside the allowed set becomes an underscore, and the
 * digest suffix keeps two ids that sanitise alike from colliding.
 */
auto StoreKeyFor(const QString& module_id) -> QString {
  QString safe;
  safe.reserve(module_id.size());
  for (const auto ch : module_id) {
    const auto c = ch.unicode();
    const auto ok = (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') ||
                    (c >= u'0' && c <= u'9') || c == u'.' || c == u'-' ||
                    c == u'_';
    safe.append(ok ? ch : QChar('_'));
  }
  if (safe.isEmpty()) safe = "module";

  const auto digest =
      QCryptographicHash::hash(module_id.toUtf8(), QCryptographicHash::Sha256)
          .toHex()
          .left(8);
  return safe + "-" + QString::fromLatin1(digest);
}

/**
 * @brief Which module a package with this digest installed, if any.
 *
 * Purely a cache, and safe to be wrong in either direction. A miss costs a
 * full verification, which is what would have happened anyway. A hit is not
 * trusted on its own: it only selects which installed tree to re-verify, and
 * that tree carries its own signature. Nothing here decides that something is
 * genuine -- it decides where to look.
 */
auto ReadDigestIndex(const QString& store_root) -> QJsonObject {
  QFile f(store_root + "/" + kIndexFileName);
  if (!f.open(QIODevice::ReadOnly)) return {};
  const auto doc = QJsonDocument::fromJson(f.readAll());
  return doc.isObject() ? doc.object() : QJsonObject{};
}

void WriteDigestIndex(const QString& store_root, const QString& digest,
                      const QString& module_id) {
  auto index = ReadDigestIndex(store_root);
  if (index.value(digest).toString() == module_id) return;
  index.insert(digest, module_id);

  QDir().mkpath(store_root);
  QSaveFile f(store_root + "/" + kIndexFileName);
  if (!f.open(QIODevice::WriteOnly)) return;
  f.write(QJsonDocument(index).toJson(QJsonDocument::Compact));
  f.commit();
}

auto ModuleDir(const QString& store_root, const QString& module_id) -> QString {
  return store_root + "/" + StoreKeyFor(module_id);
}

auto VersionsDir(const QString& store_root, const QString& module_id)
    -> QString {
  return ModuleDir(store_root, module_id) + "/" + kVersionsDirName;
}

auto StateFile(const QString& store_root, const QString& module_id) -> QString {
  return ModuleDir(store_root, module_id) + "/" + kStateFileName;
}

/// What the store remembers about one module. Deliberately two fields: the
/// version in use, and the one to go back to.
struct ModuleState {
  QString installed;
  QString previous;
};

auto ReadState(const QString& store_root, const QString& module_id)
    -> ModuleState {
  ModuleState state;
  QFile f(StateFile(store_root, module_id));
  if (!f.open(QIODevice::ReadOnly)) return state;

  const auto doc = QJsonDocument::fromJson(f.readAll());
  if (!doc.isObject()) return state;

  const auto o = doc.object();
  if (o.value("installed").isString())
    state.installed = o["installed"].toString();
  if (o.value("previous").isString()) state.previous = o["previous"].toString();
  return state;
}

auto WriteState(const QString& store_root, const QString& module_id,
                const ModuleState& state) -> bool {
  QDir().mkpath(ModuleDir(store_root, module_id));

  // QSaveFile so a crash mid-write cannot leave the store pointing at a
  // version directory that is only half named.
  QSaveFile f(StateFile(store_root, module_id));
  if (!f.open(QIODevice::WriteOnly)) return false;

  QJsonObject o;
  o.insert("installed", state.installed);
  if (!state.previous.isEmpty()) o.insert("previous", state.previous);

  f.write(QJsonDocument(o).toJson(QJsonDocument::Compact));
  return f.commit();
}

/**
 * @brief Take write permission off everything under @p directory.
 *
 * Advisory, and labelled as such wherever it is mentioned: the user owns these
 * files and can always put the permission back. What it buys is that nothing
 * writes there *by accident* -- a stray extract, a module writing next to its
 * own binary. Detection is what actually holds the line, and that is
 * VerifyExtractedModuleTree() at load.
 */
void MakeTreeReadOnly(const QString& directory) {
  QDirIterator it(directory, QDir::Files | QDir::NoSymLinks,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto path = it.next();
    auto permissions = QFile::permissions(path);
    // WriteUser as well as WriteOwner, and the distinction is not cosmetic:
    // on Unix both map to the same mode bit, and setPermissions() builds the
    // mode from the *User* flags -- so clearing WriteOwner alone leaves the
    // file writable and the whole call silently does nothing.
    permissions &= ~(QFileDevice::WriteOwner | QFileDevice::WriteUser |
                     QFileDevice::WriteGroup | QFileDevice::WriteOther);
    QFile::setPermissions(path, permissions);
  }
}

/// Removing a read-only tree needs the permission back first.
auto RemoveInstalledTree(const QString& directory) -> bool {
  QDirIterator it(directory, QDir::Files | QDir::NoSymLinks,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto path = it.next();
    QFile::setPermissions(path, QFile::permissions(path) |
                                    QFileDevice::WriteOwner |
                                    QFileDevice::WriteUser);
  }
  return QDir(directory).removeRecursively();
}

/// The module binary inside an extracted tree, as the manifest names it.
auto LibraryPathIn(const QString& directory, const ModuleManifest& manifest)
    -> QString {
  for (const auto& file : manifest.files) {
    if (file.path.startsWith("bin/")) return directory + "/" + file.path;
  }
  return {};
}

/// SHA-256 of a whole file, streamed, as lower-case hex.
auto FileDigest(const QString& path) -> QString {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) return {};

  auto digest = GFBufferFactory::ToSha256(
      [&f](const GFBufferFactory::Sha256Chunk& chunk) {
        QByteArray buf(64 * 1024, Qt::Uninitialized);
        while (true) {
          const auto n = f.read(buf.data(), buf.size());
          if (n <= 0) break;
          chunk(buf.constData(), static_cast<size_t>(n));
        }
      });
  if (!digest) return {};
  return QString::fromLatin1(digest->ConvertToQByteArray().toHex());
}

auto Succeed(const QString& install_dir, const ModuleManifest& manifest,
             bool already_installed) -> ModuleInstallResult {
  ModuleInstallResult r;
  r.ok = true;
  r.status = ModulePackageStatus::kOK;
  r.install_dir = install_dir;
  r.library_path = LibraryPathIn(install_dir, manifest);
  r.manifest = manifest;
  r.already_installed = already_installed;
  return r;
}

}  // namespace

auto ModuleStoreRoot(const QString& mods_dir) -> QString {
  if (mods_dir.isEmpty()) return {};
  return mods_dir + "/" + kModuleStoreDirName;
}

auto InstallModulePackage(const QString& package_path,
                          const QString& store_root,
                          const QByteArray& expected_public_key)
    -> ModuleInstallResult {
  if (store_root.isEmpty()) {
    return Fail(ModulePackageStatus::kIO_FAILED, "there is no module store");
  }

  // Recognise an already-installed package before paying to verify it again.
  // Hashing the file is a single fast pass; verifying it means walking the
  // whole archive and hashing every member, which for a large module was
  // seconds of startup spent re-deciding something already decided.
  //
  // This is a lookup, not a trust decision. The tree it points at is verified
  // in full against its own signature before anything loads, so a wrong answer
  // here costs correctness nothing -- a miss just falls through to the slow
  // path below.
  const auto package_digest = FileDigest(package_path);
  if (!package_digest.isEmpty()) {
    const auto cached =
        ReadDigestIndex(store_root).value(package_digest).toString();
    if (!cached.isEmpty()) {
      auto resolved =
          ResolveInstalledModule(store_root, cached, expected_public_key);
      if (resolved.ok) {
        resolved.already_installed = true;
        return resolved;
      }
    }
  }

  // Completely, and before anything is written. A package that fails here has
  // had nothing extracted, so there is nothing anywhere for anything to load.
  const auto verification =
      VerifyModulePackage(package_path, expected_public_key);
  if (!verification.ok) {
    return Fail(verification.status, verification.reason);
  }

  const auto& manifest = verification.manifest;
  const auto version_key =
      manifest.version + "-" + verification.package_sha256.left(16);
  const auto versions = VersionsDir(store_root, manifest.id);
  const auto final_dir = versions + "/" + version_key;

  auto state = ReadState(store_root, manifest.id);

  // Already there, and byte-identical: the version directory is named by the
  // package's own digest, so this is not a guess about sameness.
  if (QDir(final_dir).exists()) {
    if (state.installed != version_key) {
      state.previous = state.installed;
      state.installed = version_key;
      WriteState(store_root, manifest.id, state);
    }
    WriteDigestIndex(store_root, verification.package_sha256, manifest.id);
    return Succeed(final_dir, manifest, true);
  }

  if (!QDir().mkpath(versions)) {
    return Fail(ModulePackageStatus::kIO_FAILED,
                "the module store could not be created");
  }

  const auto staging =
      versions + "/" + kScratchPrefix +
      QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
  if (!QDir().mkpath(staging)) {
    return Fail(ModulePackageStatus::kIO_FAILED,
                "a staging directory could not be created");
  }

  QString reason;
  const auto error = ArchiveFileOperator::ExtractArchiveFromFileSync(
      package_path, staging, ArchiveExtractPolicy::Strict(-1, -1), {}, {},
      &reason);
  if (error != 0) {
    QDir(staging).removeRecursively();
    return Fail(
        ModulePackageStatus::kIO_FAILED,
        reason.isEmpty() ? QString("it could not be unpacked") : reason);
  }

  // Re-verified where it landed, not trusted because it was verified in the
  // archive. A truncated write or a full disk is the realistic failure at this
  // point, and a rename would make either of them permanent.
  const auto landed = VerifyExtractedModuleTree(staging, expected_public_key);
  if (!landed.ok) {
    QDir(staging).removeRecursively();
    return Fail(
        landed.status,
        QString("it did not survive being unpacked: %1").arg(landed.reason));
  }

  MakeTreeReadOnly(staging);

  if (!QDir().rename(staging, final_dir)) {
    QDir(staging).removeRecursively();
    return Fail(ModulePackageStatus::kIO_FAILED,
                "it could not be moved into the module store");
  }

  // Only now does anything point at it. The order matters: a crash before this
  // leaves an unreferenced directory, which the sweep collects; a crash after
  // it leaves a module installed, which is the outcome that was wanted.
  if (state.installed != version_key) {
    state.previous = state.installed;
    state.installed = version_key;
    if (!WriteState(store_root, manifest.id, state)) {
      return Fail(ModulePackageStatus::kIO_FAILED,
                  "the module store could not record the install");
    }
  }

  WriteDigestIndex(store_root, verification.package_sha256, manifest.id);
  return Succeed(final_dir, landed.manifest, false);
}

auto ResolveInstalledModule(const QString& store_root, const QString& module_id,
                            const QByteArray& expected_public_key)
    -> ModuleInstallResult {
  if (store_root.isEmpty()) {
    return Fail(ModulePackageStatus::kNOT_INSTALLED,
                "there is no module store");
  }

  const auto state = ReadState(store_root, module_id);
  if (state.installed.isEmpty()) {
    return Fail(ModulePackageStatus::kNOT_INSTALLED,
                "this module is not installed");
  }

  const auto dir = VersionsDir(store_root, module_id) + "/" + state.installed;
  const auto verification = VerifyExtractedModuleTree(dir, expected_public_key);
  if (!verification.ok) {
    return Fail(verification.status, verification.reason);
  }

  // The manifest inside the tree is signed, so it is the authority on which
  // module this is -- a store that filed it under the wrong id would otherwise
  // hand back a module by a name nothing vouches for.
  if (verification.manifest.id != module_id) {
    return Fail(ModulePackageStatus::kMALFORMED,
                QString("the store holds %1 under the name %2")
                    .arg(verification.manifest.id, module_id));
  }

  return Succeed(dir, verification.manifest, true);
}

auto RollbackInstalledModule(const QString& store_root,
                             const QString& module_id) -> ModuleInstallResult {
  auto state = ReadState(store_root, module_id);
  if (state.previous.isEmpty()) {
    return Fail(ModulePackageStatus::kNOT_INSTALLED,
                "there is no earlier version to go back to");
  }

  const auto target = VersionsDir(store_root, module_id) + "/" + state.previous;
  const auto verification = VerifyExtractedModuleTree(target);
  if (!verification.ok) {
    return Fail(verification.status,
                QString("the earlier version cannot be used: %1")
                    .arg(verification.reason));
  }

  // Swapped rather than dropped, so a rollback can itself be undone -- which
  // matters when the rollback turns out not to have been the problem.
  std::swap(state.installed, state.previous);
  if (!WriteState(store_root, module_id, state)) {
    return Fail(ModulePackageStatus::kIO_FAILED,
                "the module store could not record the rollback");
  }

  return Succeed(target, verification.manifest, true);
}

auto ListInstalledModules(const QString& store_root) -> QStringList {
  QStringList ids;
  QDir root(store_root);
  if (!root.exists()) return ids;

  for (const auto& entry :
       root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    // The directory name is a sanitised key, not the id, so the id is read
    // back from the state's own installed tree rather than guessed from it.
    const auto state_path = root.filePath(entry) + "/" + kStateFileName;
    QFile f(state_path);
    if (!f.open(QIODevice::ReadOnly)) continue;

    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) continue;
    const auto installed = doc.object().value("installed").toString();
    if (installed.isEmpty()) continue;

    const auto dir =
        root.filePath(entry) + "/" + kVersionsDirName + "/" + installed;
    QFile manifest_file(dir + "/" + kModulePackageManifestPath);
    if (!manifest_file.open(QIODevice::ReadOnly)) continue;

    const auto parsed = ParseModuleManifest(manifest_file.readAll());
    if (parsed.ok) ids.append(parsed.manifest.id);
  }

  ids.sort();
  return ids;
}

auto SweepModuleStore(const QString& store_root) -> int {
  QDir root(store_root);
  if (!root.exists()) return 0;

  auto removed = 0;
  for (const auto& entry :
       root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    const auto module_dir = root.filePath(entry);
    const auto versions_dir = module_dir + "/" + kVersionsDirName;

    QString installed;
    QString previous;
    {
      QFile f(module_dir + "/" + kStateFileName);
      if (f.open(QIODevice::ReadOnly)) {
        const auto doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) {
          installed = doc.object().value("installed").toString();
          previous = doc.object().value("previous").toString();
        }
      }
    }

    QDir versions(versions_dir);
    if (!versions.exists()) continue;

    // Hidden included deliberately: abandoned staging is dot-prefixed, and it
    // is the main thing there is to collect. The two names that are kept can
    // never be dot-prefixed, so including hidden entries cannot protect the
    // wrong one.
    for (const auto& version : versions.entryList(
             QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name)) {
      // Kept: the version in use, and the one a rollback would return to.
      // Everything else is either abandoned staging or a version that has been
      // superseded twice, and nothing can reach either.
      if (version == installed || version == previous) continue;
      if (RemoveInstalledTree(versions.filePath(version))) ++removed;
    }
  }

  return removed;
}

}  // namespace GpgFrontend::Module
