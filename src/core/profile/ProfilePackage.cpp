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

#include "core/profile/ProfilePackage.h"

#include <sodium.h>

#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>
#include <optional>
#include <thread>

#include "core/function/AESCryptoHelper.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/model/GFDataExchanger.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileLock.h"
#include "core/profile/ProfileMarker.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"

#ifdef Q_OS_UNIX
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace GpgFrontend {

namespace {

constexpr int kMaxHeaderLength = 64 * 1024;
constexpr qint64 kDrainChunk = 64 * 1024;

/// Absolute floor and ceiling for the one-shot payload cap. The floor keeps a
/// container with a tiny locked-memory allowance from refusing every export;
/// the ceiling keeps a machine with none from trying to hold a DVD in memory.
constexpr qint64 kPayloadCapFloor = 16LL * 1024 * 1024;
constexpr qint64 kPayloadCapCeiling = 256LL * 1024 * 1024;

/// Never packed. Logs and modules are not the profile; the quarantine and the
/// collector's bookkeeping describe this machine's history with it; lock files
/// and agent sockets describe a process that will not exist at the other end.
auto IsExcludedFromPackage(const QString &relative_path) -> bool {
  const auto name = relative_path.section('/', -1);
  const auto top = relative_path.section('/', 0, 0);

  if (top == "logs" || top == "mods" || top == "data_objs.quarantine") {
    return true;
  }
  // A root profile has the profiles root inside it: every other profile on this
  // machine, every session root of a live window, and the scratch directory
  // this very export is staging into. None of that is this profile's own data.
  if (top == "profiles") return true;
  if (relative_path == "data_objs.gc.json") return true;
  if (name == "profile.lock" || name == "profiles.lock") return true;
  if (name.startsWith("S.gpg-agent") || name == "S.dirmngr") return true;
  if (name == ".DS_Store" || name == "Thumbs.db" || name == "desktop.ini") {
    return true;
  }
  if (name.endsWith('~')) return true;
  if (top.startsWith('.')) return true;

  // secure/app.key is written separately, unwrapped: what is on disk here may
  // be sealed by this machine's credential store and would not open elsewhere
  if (relative_path == "secure/app.key") return true;

  return false;
}

auto DirectorySize(const QString &path) -> qint64 {
  if (!QFileInfo::exists(path)) return 0;

  qint64 total = 0;
  QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    total += it.fileInfo().size();
  }
  return total;
}

auto DrainExchanger(const QSharedPointer<GFDataExchanger> &ex, qint64 cap,
                    QByteArray &out) -> bool {
  QByteArray chunk(static_cast<int>(kDrainChunk), Qt::Uninitialized);

  while (true) {
    const auto read =
        ex->Read(reinterpret_cast<std::byte *>(chunk.data()), kDrainChunk);
    if (read <= 0) break;

    if (cap > 0 && out.size() + read > cap) {
      // Keep reading is not an option and neither is stopping silently: the
      // writer is blocked on us, so drain to the end and report afterwards.
      out.clear();
      while (ex->Read(reinterpret_cast<std::byte *>(chunk.data()),
                      kDrainChunk) > 0) {
      }
      return false;
    }
    out.append(chunk.constData(), static_cast<int>(read));
  }
  return true;
}

auto CopyTreeForPacking(const QString &source, const QString &destination,
                        const std::function<bool(const QString &)> &excluded,
                        const QString &prefix, const QString &guard,
                        qint64 &bytes) -> QString {
  QDir source_dir(source);
  if (!source_dir.exists()) return {};

  if (!QDir().mkpath(destination)) {
    return QString("cannot create %1").arg(destination);
  }

  const auto entries = source_dir.entryInfoList(
      QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);

  for (const auto &entry : entries) {
    const auto relative =
        prefix.isEmpty() ? entry.fileName() : prefix + "/" + entry.fileName();
    if (excluded(relative)) continue;

    const auto target = destination + "/" + entry.fileName();

    if (entry.isSymLink()) continue;  // a package carries no links, by design

    // The staging tree may sit inside the tree being copied, and copying it
    // into itself does not terminate. The exclusions above should have taken
    // it already; this is what makes that a bug and not an infinite walk.
    if (!guard.isEmpty() &&
        QDir::cleanPath(entry.absoluteFilePath()) == guard) {
      continue;
    }

    if (entry.isDir()) {
      const auto error = CopyTreeForPacking(entry.absoluteFilePath(), target,
                                            excluded, relative, guard, bytes);
      if (!error.isEmpty()) return error;
      continue;
    }

    if (!QFile::copy(entry.absoluteFilePath(), target)) {
      // A file the collector removed while we walked past it is not a reason
      // to lose the whole export; anything else would have failed the mkpath.
      if (QFileInfo::exists(entry.absoluteFilePath())) {
        return QString("cannot copy %1").arg(relative);
      }
      LOG_W() << "file vanished while staging, skipped:" << relative;
      continue;
    }
    bytes += entry.size();
  }
  return {};
}

auto RemoveDirectoryQuietly(const QString &path) -> void {
  if (path.isEmpty() || !QFileInfo::exists(path)) return;
  QDir(path).removeRecursively();
}

/// Empty a session root of everything a dead process left, except the lock the
/// live one is holding: removing that would hand the root to a second window
/// while this one is still extracting into it.
auto ClearSessionRootContents(const QString &path) -> void {
  QDir dir(path);
  if (!dir.exists()) return;

  for (const auto &entry : dir.entryInfoList(
           QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
    if (entry.fileName() == "profile.lock") continue;
    if (entry.isDir()) {
      QDir(entry.absoluteFilePath()).removeRecursively();
      continue;
    }
    QFile::remove(entry.absoluteFilePath());
  }
}

/// A scratch tree that does not outlive the call that made it: until it is
/// gone, an unprotected copy of an application key is sitting on this disk.
struct ScratchGuard {
  QString path;
  ~ScratchGuard() { RemoveDirectoryQuietly(path); }
};

auto ReadWholeFile(const QString &path, qint64 max_bytes, QByteArray &out)
    -> bool {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return false;
  if (max_bytes > 0 && file.size() > max_bytes) return false;
  out = file.readAll();
  return true;
}

auto WriteStagedAppKey(const QString &staging_dir, const GFBuffer &app_key)
    -> bool {
  if (app_key.Empty()) return false;

  const auto directory =
      staging_dir + "/" + kProfilePackageTreePrefix + "/secure";
  if (!QDir().mkpath(directory)) return false;

  QSaveFile file(directory + "/app.key");
  if (!file.open(QIODevice::WriteOnly)) return false;

  file.write(app_key.ConvertToQByteArray());
  return file.commit();
}

auto WriteStagedSettings(const QString &staging_dir,
                         const QMap<QString, QVariant> &settings) -> bool {
  const auto directory =
      staging_dir + "/" + kProfilePackageTreePrefix + "/config";
  if (!QDir().mkpath(directory)) return false;

  QSettings staged(directory + "/config.ini", QSettings::IniFormat);
  staged.clear();
  for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
    staged.setValue(it.key(), it.value());
  }
  staged.sync();

  return staged.status() == QSettings::NoError;
}

auto WriteStagedManifest(const QString &staging_dir,
                         const ProfilePackageManifest &manifest) -> bool {
  QSaveFile file(staging_dir + "/manifest.json");
  if (!file.open(QIODevice::WriteOnly)) return false;

  file.write(EncodeProfilePackageManifest(manifest));
  return file.commit();
}

auto WriteProfilePackage(const QString &staging_dir, const QString &dest_path,
                         const ProfilePackageHeader &header,
                         const GFBuffer &passphrase)
    -> ProfilePackageWriteResult {
  ProfilePackageWriteResult result;

  const auto cap = ProfilePackagePayloadCap();

  auto exchanger = CreateStandardGFDataExchanger();
  GFError archive_error = 0;

  // The archive is produced on another thread because the exchanger is a pipe:
  // it holds a few megabytes and then blocks its writer until someone reads.
  std::thread producer([&]() {
    archive_error = ArchiveFileOperator::NewArchive2DataExchangerSync(
        staging_dir, exchanger, ArchiveCompression::kGZIP);
  });

  QByteArray payload;
  const auto within_cap = DrainExchanger(exchanger, cap, payload);
  producer.join();

  if (!within_cap) {
    result.error =
        QString(
            "this profile is too large to export in one piece on this machine "
            "(the limit is %1 MB)")
            .arg(cap / (1024 * 1024));
    return result;
  }
  if (archive_error < 0 || payload.isEmpty()) {
    result.error = "the profile could not be packed";
    return result;
  }

  QByteArray body;
  if (header.protection == ProfilePackageProtection::kPIN) {
    const auto encrypted =
        AESCryptoHelper::Encrypt(passphrase, GFBuffer(payload));
    if (!encrypted) {
      result.error = "the package could not be encrypted";
      return result;
    }
    body = encrypted->ConvertToQByteArray();
  } else {
    body = payload;
  }

  const auto header_bytes = EncodeProfilePackageHeader(header);

  // Written beside the destination, so the final step is a rename inside one
  // filesystem rather than a copy that could fail halfway. The destination is
  // never opened for writing: a failed save that had truncated it would
  // destroy keys, settings and workspace at once, and this file is the backup.
  const auto temporary_path =
      QString("%1/.%2.tmp-%3")
          .arg(QFileInfo(dest_path).absolutePath(),
               QFileInfo(dest_path).fileName(),
               QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));

  {
    QFile temporary(temporary_path);
    if (!temporary.open(QIODevice::WriteOnly)) {
      result.error = "the package file could not be created";
      return result;
    }
    if (temporary.write(header_bytes) != header_bytes.size() ||
        temporary.write(body) != body.size()) {
      temporary.close();
      QFile::remove(temporary_path);
      result.error = "the package could not be written";
      return result;
    }
    temporary.flush();

    // Durable before it is believed: reading back what only the page cache
    // knows would verify nothing about the file that survives a power cut.
#ifdef Q_OS_UNIX
    ::fsync(static_cast<int>(temporary.handle()));
#endif
    temporary.close();
  }

  // A package that cannot be read back is not a package. One extra decryption
  // is worth knowing that before the only copy of something is trusted to it.
  QByteArray written;
  if (!ReadWholeFile(temporary_path, 0, written)) {
    QFile::remove(temporary_path);
    result.error = "the package could not be read back";
    return result;
  }

  const auto view = ParseProfilePackageHeader(written);
  if (!view.Ok() || view.header_bytes != header_bytes) {
    QFile::remove(temporary_path);
    result.error = "the package was written but does not read back correctly";
    return result;
  }

  const auto written_body = written.mid(view.body_offset);
  if (header.protection == ProfilePackageProtection::kPIN) {
    const auto decrypted =
        AESCryptoHelper::Decrypt(passphrase, GFBuffer(written_body));
    if (!decrypted || decrypted->ConvertToQByteArray() != payload) {
      QFile::remove(temporary_path);
      result.error = "the package was written but cannot be decrypted";
      return result;
    }
  } else if (written_body != payload) {
    QFile::remove(temporary_path);
    result.error = "the package was written but does not read back correctly";
    return result;
  }

  if (QFileInfo::exists(dest_path) && !QFile::remove(dest_path)) {
    QFile::remove(temporary_path);
    result.error = "the existing package could not be replaced";
    return result;
  }
  if (!QFile::rename(temporary_path, dest_path)) {
    QFile::remove(temporary_path);
    result.error = "the package could not be moved into place";
    return result;
  }

  result.ok = true;
  result.bytes = header_bytes.size() + body.size();
  return result;
}

}  // namespace

auto ProfilePackageProtectionToString(ProfilePackageProtection protection)
    -> QString {
  return protection == ProfilePackageProtection::kPIN ? "pin" : "none";
}

auto ProfilePackageProtectionFromString(const QString &value)
    -> ProfilePackageProtection {
  return value == "pin" ? ProfilePackageProtection::kPIN
                        : ProfilePackageProtection::kNONE;
}

auto EncodeProfilePackageHeader(const ProfilePackageHeader &header)
    -> QByteArray {
  QJsonObject json;
  json["format"] = "gfprofile";
  json["format_version"] = header.format_version;
  json["min_reader"] = header.min_reader;
  json["writer"] = header.writer;
  json["writer_stable"] = header.writer_stable;
  json["created"] = header.created;
  json["protection"] = ProfilePackageProtectionToString(header.protection);
  json["container"] =
      header.protection == ProfilePackageProtection::kPIN ? "GFSEC2" : "none";

  const auto body = QJsonDocument(json).toJson(QJsonDocument::Compact);

  QByteArray out;
  out.append(kProfilePackageMagic, kProfilePackageMagicLength);

  const auto length = static_cast<quint32>(body.size());
  for (int shift = 24; shift >= 0; shift -= 8) {
    out.append(static_cast<char>((length >> shift) & 0xFF));
  }
  out.append(body);
  return out;
}

auto ParseProfilePackageHeader(const QByteArray &bytes)
    -> ProfilePackageHeaderView {
  ProfilePackageHeaderView view;

  const auto prefix = kProfilePackageMagicLength + 4;
  if (bytes.size() < kProfilePackageMagicLength ||
      !bytes.startsWith(
          QByteArray(kProfilePackageMagic, kProfilePackageMagicLength))) {
    view.status = ProfilePackageHeaderStatus::kNOT_A_PACKAGE;
    view.detail = "this file is not a GpgFrontend profile package";
    return view;
  }

  if (bytes.size() < prefix) {
    view.status = ProfilePackageHeaderStatus::kTRUNCATED;
    view.detail = "the file ends before its header does";
    return view;
  }

  quint32 length = 0;
  for (int i = 0; i < 4; ++i) {
    length = (length << 8) |
             static_cast<quint8>(bytes.at(kProfilePackageMagicLength + i));
  }

  if (length == 0 || length > kMaxHeaderLength) {
    view.status = ProfilePackageHeaderStatus::kMALFORMED;
    view.detail = "the header length is not believable";
    return view;
  }
  if (bytes.size() < prefix + static_cast<int>(length)) {
    view.status = ProfilePackageHeaderStatus::kTRUNCATED;
    view.detail = "the file ends before its header does";
    return view;
  }

  view.header_bytes = bytes.left(prefix + static_cast<int>(length));
  view.body_offset = view.header_bytes.size();

  QJsonParseError error{};
  const auto document = QJsonDocument::fromJson(
      bytes.mid(prefix, static_cast<int>(length)), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    view.status = ProfilePackageHeaderStatus::kMALFORMED;
    view.detail = "the header is not readable";
    return view;
  }

  const auto json = document.object();
  if (json["format"].toString() != "gfprofile") {
    view.status = ProfilePackageHeaderStatus::kNOT_A_PACKAGE;
    view.detail = "this file is not a GpgFrontend profile package";
    return view;
  }

  view.header.format_version = json["format_version"].toInt();
  view.header.min_reader = json["min_reader"].toInt();
  view.header.writer = json["writer"].toString();
  view.header.writer_stable = json["writer_stable"].toBool();
  view.header.created = json["created"].toString();
  view.header.protection =
      ProfilePackageProtectionFromString(json["protection"].toString());

  // The one decision the plaintext header is allowed to make on its own, and
  // it is a refusal: it saves a hundred milliseconds of key derivation on a
  // file we could not use anyway. Nothing positive is ever concluded here.
  //
  // Only min_reader is grounds for it. format_version says what was written,
  // min_reader says what it takes to read it, and refusing on the first would
  // make the second dead weight: a writer that adds a field it does not need
  // us to understand raises format_version and leaves min_reader alone, and
  // that package has to open here. That is the entire reason there are two
  // numbers rather than one.
  if (view.header.min_reader > kProfilePackageFormatVersion) {
    view.status = ProfilePackageHeaderStatus::kTOO_NEW;
    view.detail =
        QString("this package was written by a newer version of GpgFrontend%1")
            .arg(view.header.writer.isEmpty()
                     ? QString{}
                     : QString(" (%1)").arg(view.header.writer));
    return view;
  }

  return view;
}

auto ProfilePackageHeaderDigest(const QByteArray &header_bytes) -> QString {
  std::array<unsigned char, 32> digest{};
  crypto_generichash(
      digest.data(), digest.size(),
      reinterpret_cast<const unsigned char *>(header_bytes.constData()),
      static_cast<unsigned long long>(header_bytes.size()), nullptr, 0);
  return QByteArray(reinterpret_cast<const char *>(digest.data()),
                    static_cast<int>(digest.size()))
      .toHex();
}

auto EncodeProfilePackageManifest(const ProfilePackageManifest &manifest)
    -> QByteArray {
  // Seeded with what this build did not understand, so that everything below
  // overwrites rather than competes: a field this build knows is always
  // written from the struct, and one it does not is carried through untouched.
  QJsonObject json = manifest.unknown_fields;
  json["manifest_version"] = manifest.manifest_version;
  json["format_version"] = manifest.format_version;
  json["min_reader"] = manifest.min_reader;
  json["protection"] = manifest.protection;
  json["header_digest"] = manifest.header_digest;

  json["schema_version"] = manifest.schema_version;
  json["min_reader_version"] = manifest.min_reader_version;

  json["app_profile"] = manifest.app_profile;
  json["display_name"] = manifest.display_name;
  json["profile_id"] = manifest.profile_id;
  json["writer_version"] = manifest.writer_version;
  json["created"] = manifest.created;
  json["package_id"] = manifest.package_id;

  json["app_key_protection"] = manifest.app_key_protection;
  json["workspace_included"] = manifest.workspace_included;
  json["self_contained"] = manifest.self_contained;

  QJsonArray databases;
  for (const auto &entry : manifest.key_databases) {
    QJsonObject item;
    item["name"] = entry.name;
    item["stored_path"] = entry.stored_path;
    item["backend_type"] = entry.backend_type;
    item["external"] = entry.external;
    databases.append(item);
  }
  json["key_databases"] = databases;

  return QJsonDocument(json).toJson(QJsonDocument::Indented);
}

auto ParseProfilePackageManifest(const QByteArray &bytes)
    -> std::optional<ProfilePackageManifest> {
  QJsonParseError error{};
  const auto document = QJsonDocument::fromJson(bytes, &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    return {};
  }

  const auto json = document.object();
  ProfilePackageManifest manifest;

  manifest.manifest_version = json["manifest_version"].toInt(1);
  manifest.format_version = json["format_version"].toInt();
  manifest.min_reader = json["min_reader"].toInt();
  manifest.protection = json["protection"].toString();
  manifest.header_digest = json["header_digest"].toString();

  manifest.schema_version = json["schema_version"].toInt();
  manifest.min_reader_version = json["min_reader_version"].toInt();

  manifest.app_profile = json["app_profile"].toString();
  manifest.display_name = json["display_name"].toString();
  manifest.profile_id = json["profile_id"].toString();
  manifest.writer_version = json["writer_version"].toString();
  manifest.created = json["created"].toString();
  manifest.package_id = json["package_id"].toString();

  manifest.app_key_protection = json["app_key_protection"].toString("none");
  manifest.workspace_included = json["workspace_included"].toBool();
  manifest.self_contained = json["self_contained"].toBool();

  for (const auto item : json["key_databases"].toArray()) {
    const auto object = item.toObject();
    ProfilePackageKeyDatabaseEntry entry;
    entry.name = object["name"].toString();
    entry.stored_path = object["stored_path"].toString();
    entry.backend_type = object["backend_type"].toString();
    entry.external = object["external"].toBool();
    manifest.key_databases.append(entry);
  }

  // Anything this build does not know is carried through untouched, so that a
  // package written by a newer one can be imported and exported again here
  // without silently losing what that build depends on.
  static const QSet<QString> kKnown = {
      "manifest_version",   "format_version",     "min_reader",
      "protection",         "header_digest",      "schema_version",
      "min_reader_version", "app_profile",        "display_name",
      "profile_id",         "writer_version",     "created",
      "package_id",         "app_key_protection", "workspace_included",
      "self_contained",     "key_databases"};
  for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
    if (!kKnown.contains(it.key()))
      manifest.unknown_fields[it.key()] = it.value();
  }

  return manifest;
}

auto CheckPackageHeaderAgainstManifest(const ProfilePackageHeader &header,
                                       const QByteArray &header_bytes,
                                       const ProfilePackageManifest &manifest)
    -> QString {
  if (manifest.format_version != header.format_version ||
      manifest.min_reader != header.min_reader) {
    return "the package's version does not match what its header claims";
  }
  if (manifest.protection !=
      ProfilePackageProtectionToString(header.protection)) {
    return "the package's protection does not match what its header claims";
  }
  if (!manifest.header_digest.isEmpty() &&
      manifest.header_digest != ProfilePackageHeaderDigest(header_bytes)) {
    return "the package's header has been altered since it was written";
  }
  return {};
}

auto RewriteKeyDatabaseListForPacking(
    const QContainer<KeyDatabaseItemSO> &databases, const QString &profile_root)
    -> QContainer<KeyDatabaseItemSO> {
  QContainer<KeyDatabaseItemSO> out;
  out.reserve(databases.size());

  for (auto item : databases) {
    const auto rewritten =
        ToProfileRelativeKeyDatabasePath(item.path, profile_root);
    if (!rewritten.isEmpty()) item.path = rewritten;
    out.push_back(item);
  }
  return out;
}

auto DescribeKeyDatabasesForManifest(
    const QContainer<KeyDatabaseItemSO> &databases)
    -> QList<ProfilePackageKeyDatabaseEntry> {
  QList<ProfilePackageKeyDatabaseEntry> out;

  for (const auto &item : databases) {
    ProfilePackageKeyDatabaseEntry entry;
    entry.name = item.name;
    entry.stored_path = item.path;
    entry.backend_type = item.backend_type;

    // After the rewrite, travelling means carrying the token. Anything still
    // holding an absolute path is somewhere this package cannot reach.
    entry.external = !item.path.startsWith(QLatin1String(kProfilePathToken));
    out.append(entry);
  }
  return out;
}

auto MeasureProfileAreas(const QString &profile_root) -> QMap<QString, qint64> {
  QMap<QString, qint64> areas;
  areas["config"] = DirectorySize(profile_root + "/config");
  areas["data_objs"] = DirectorySize(profile_root + "/data_objs");
  areas["secure"] = DirectorySize(profile_root + "/secure");
  areas["key_databases"] = DirectorySize(profile_root + "/db") +
                           DirectorySize(profile_root + "/dbs");
  areas["workspace"] = DirectorySize(profile_root + "/workspace");
  return areas;
}

auto ProfilePackagePayloadCap() -> qint64 {
#ifdef Q_OS_UNIX
  struct rlimit limit{};
  if (getrlimit(RLIMIT_MEMLOCK, &limit) == 0 &&
      limit.rlim_cur != RLIM_INFINITY) {
    // The payload and its ciphertext are both live, both in locked memory, and
    // the allocator needs headroom on top; a quarter of the allowance is the
    // conservative reading of that.
    const auto quarter = static_cast<qint64>(limit.rlim_cur) / 4;
    return qBound(kPayloadCapFloor, quarter, kPayloadCapCeiling);
  }
#endif
  return kPayloadCapCeiling;
}

auto MakeProfilePackageScratchDir(const QString &profiles_root,
                                  const QString &purpose) -> QString {
  for (int attempt = 0; attempt < 64; ++attempt) {
    const auto candidate =
        QString("%1/.gfp-%2-%3")
            .arg(profiles_root, purpose,
                 QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    if (!QFileInfo::exists(candidate)) return candidate;
  }
  return {};
}

auto StageProfileTree(const QString &profile_root, const QString &staging_dir,
                      bool include_workspace) -> ProfileStagingResult {
  ProfileStagingResult result;

  // Checked rather than assumed: the copy skips a source that is not there,
  // so without this an export of nothing would succeed and produce an empty
  // package — over whatever file the user pointed it at.
  if (!QFileInfo(profile_root).isDir()) {
    result.error = "the profile folder is not there";
    return result;
  }

  if (QFileInfo::exists(staging_dir)) {
    result.error = "the staging directory already exists";
    return result;
  }

  const auto tree = staging_dir + "/" + kProfilePackageTreePrefix;
  if (!QDir().mkpath(tree)) {
    result.error = "the staging directory could not be created";
    return result;
  }

  const auto excluded = [include_workspace](const QString &relative) {
    if (!include_workspace && relative.section('/', 0, 0) == "workspace") {
      return true;
    }
    return IsExcludedFromPackage(relative);
  };

  const auto error = CopyTreeForPacking(
      profile_root, tree, excluded, {},
      QDir::cleanPath(QFileInfo(staging_dir).absoluteFilePath()), result.bytes);
  if (!error.isEmpty()) {
    result.error = error;
    RemoveDirectoryQuietly(staging_dir);
    return result;
  }

  result.ok = true;
  return result;
}

auto SnapshotSettings(QSettings &settings) -> QMap<QString, QVariant> {
  settings.sync();

  QMap<QString, QVariant> snapshot;
  for (const auto &key : settings.allKeys()) {
    snapshot.insert(key, settings.value(key));
  }
  return snapshot;
}

auto InspectProfilePackage(const QString &package_path)
    -> ProfilePackageReadResult {
  ProfilePackageReadResult result;

  QFile file(package_path);
  if (!file.open(QIODevice::ReadOnly)) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "this file could not be opened";
    return result;
  }

  const auto view = ParseProfilePackageHeader(
      file.read(kProfilePackageMagicLength + 4 + kMaxHeaderLength));

  switch (view.status) {
    case ProfilePackageHeaderStatus::kOK:
      result.header = view.header;
      return result;
    case ProfilePackageHeaderStatus::kTOO_NEW:
      result.status = ProfilePackageReadStatus::kTOO_NEW;
      break;
    case ProfilePackageHeaderStatus::kNOT_A_PACKAGE:
      result.status = ProfilePackageReadStatus::kNOT_A_PACKAGE;
      break;
    default:
      result.status = ProfilePackageReadStatus::kMALFORMED;
      break;
  }
  result.detail = view.detail;
  return result;
}

auto ReadProfilePackage(const QString &package_path, const QString &staging_dir,
                        const GFBuffer &passphrase)
    -> ProfilePackageReadResult {
  ProfilePackageReadResult result;

  QByteArray bytes;
  if (!ReadWholeFile(package_path, 0, bytes)) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "this file could not be read";
    return result;
  }

  const auto view = ParseProfilePackageHeader(bytes);
  if (!view.Ok()) {
    result.status = view.status == ProfilePackageHeaderStatus::kTOO_NEW
                        ? ProfilePackageReadStatus::kTOO_NEW
                    : view.status == ProfilePackageHeaderStatus::kNOT_A_PACKAGE
                        ? ProfilePackageReadStatus::kNOT_A_PACKAGE
                        : ProfilePackageReadStatus::kMALFORMED;
    result.detail = view.detail;
    return result;
  }
  result.header = view.header;

  QByteArray payload;
  if (view.header.protection == ProfilePackageProtection::kPIN) {
    const auto decrypted = AESCryptoHelper::Decrypt(
        passphrase, GFBuffer(bytes.mid(view.body_offset)));
    if (!decrypted) {
      result.status = ProfilePackageReadStatus::kBAD_PASSPHRASE;
      result.detail = "the passphrase does not open this package";
      return result;
    }
    payload = decrypted->ConvertToQByteArray();
  } else {
    payload = bytes.mid(view.body_offset);
  }

  // Derived from what is actually in hand rather than from anything the file
  // claims about itself: an archive that says it is small and is not dies
  // early instead of filling the disk.
  auto policy = ArchiveExtractPolicy::Strict(
      qMax(static_cast<qint64>(64) * 1024 * 1024,
           static_cast<qint64>(payload.size()) * 100),
      200000);

  auto exchanger = CreateStandardGFDataExchanger();
  std::thread feeder([&]() {
    exchanger->Write(reinterpret_cast<const std::byte *>(payload.constData()),
                     payload.size());
    exchanger->CloseWrite();
  });

  const auto error = ArchiveFileOperator::ExtractArchiveFromDataExchangerSync(
      exchanger, staging_dir, policy);
  feeder.join();

  if (error < 0) {
    RemoveDirectoryQuietly(staging_dir);
    result.status = ProfilePackageReadStatus::kMALFORMED;
    result.detail = "the package's contents could not be unpacked";
    return result;
  }

  QByteArray manifest_bytes;
  if (!ReadWholeFile(staging_dir + "/manifest.json", kMaxHeaderLength * 16,
                     manifest_bytes)) {
    RemoveDirectoryQuietly(staging_dir);
    result.status = ProfilePackageReadStatus::kMALFORMED;
    result.detail = "the package does not describe itself";
    return result;
  }

  const auto manifest = ParseProfilePackageManifest(manifest_bytes);
  if (!manifest.has_value()) {
    RemoveDirectoryQuietly(staging_dir);
    result.status = ProfilePackageReadStatus::kMALFORMED;
    result.detail = "the package does not describe itself";
    return result;
  }

  // The header is plaintext and anyone can edit it; the manifest is sealed and
  // nobody can. A disagreement is tampering, and says so.
  const auto mismatch = CheckPackageHeaderAgainstManifest(
      view.header, view.header_bytes, *manifest);
  if (!mismatch.isEmpty()) {
    RemoveDirectoryQuietly(staging_dir);
    result.status = ProfilePackageReadStatus::kTAMPERED;
    result.detail = mismatch;
    return result;
  }

  if (!QFileInfo::exists(staging_dir + "/" + kProfilePackageTreePrefix)) {
    RemoveDirectoryQuietly(staging_dir);
    result.status = ProfilePackageReadStatus::kMALFORMED;
    result.detail = "the package carries no profile";
    return result;
  }

  result.manifest = *manifest;
  return result;
}

auto ExportProfilePackage(const ProfileExportRequest &request)
    -> ProfilePackageWriteResult {
  ProfilePackageWriteResult result;

  const auto staging =
      MakeProfilePackageScratchDir(request.profiles_root, "staging");
  if (staging.isEmpty()) {
    result.error = "a temporary folder could not be made";
    return result;
  }

  const ScratchGuard guard{staging};

  const auto staged = StageProfileTree(request.profile_root, staging,
                                       request.include_workspace);
  if (!staged.ok) {
    result.error = staged.error;
    return result;
  }

  if (!WriteStagedSettings(staging, request.settings)) {
    result.error = "the settings could not be written into the package";
    return result;
  }

  if (!WriteStagedAppKey(staging, request.app_key)) {
    result.error = "the application key could not be written into the package";
    return result;
  }

  ProfilePackageHeader header;
  header.writer = GetProjectVersion();
  header.writer_stable = IsStableBuild();
  header.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  header.protection = request.protection;

  auto manifest = request.manifest;
  manifest.format_version = header.format_version;
  manifest.min_reader = header.min_reader;
  manifest.protection = ProfilePackageProtectionToString(header.protection);
  manifest.header_digest =
      ProfilePackageHeaderDigest(EncodeProfilePackageHeader(header));
  manifest.writer_version = header.writer;
  manifest.created = header.created;
  manifest.workspace_included = request.include_workspace;
  manifest.app_key_protection = "none";
  if (manifest.package_id.isEmpty()) {
    manifest.package_id =
        QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
  }

  if (!WriteStagedManifest(staging, manifest)) {
    result.error = "the package could not describe itself";
    return result;
  }

  return WriteProfilePackage(staging, request.dest_path, header,
                             request.passphrase);
}

auto AdoptExtractedProfile(const QString &staging_dir,
                           const QString &profile_root, const QString &id,
                           const QString &display_name,
                           const ProfilePackageManifest &manifest) -> QString {
  const auto tree = staging_dir + "/" + kProfilePackageTreePrefix;
  if (!QFileInfo::exists(tree)) return "the package carries no profile";

  if (!QDir().mkpath(profile_root)) {
    return "the profile folder could not be created";
  }

  // Moved entry by entry rather than renamed wholesale, because the
  // destination may already exist and be locked: a session root is created and
  // locked before anything is extracted into it, and the lock file living
  // there is exactly what must survive.
  QDir source(tree);
  for (const auto &entry : source.entryInfoList(
           QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
    const auto destination = profile_root + "/" + entry.fileName();
    if (QFileInfo::exists(destination)) {
      return QString("%1 is already there").arg(entry.fileName());
    }
    if (!QDir().rename(entry.absoluteFilePath(), destination)) {
      return QString("%1 could not be unpacked").arg(entry.fileName());
    }
  }

  auto marker = ReadProfileMarker(ProfileMarkerPathFor(profile_root))
                    .value_or(ProfileMarker{});

  if (marker.schema_version == 0) {
    marker.schema_version = manifest.schema_version;
    marker.min_reader_version = manifest.min_reader_version;
    marker.profile = manifest.app_profile;
  }

  // A new identity, deliberately, and the same one the directory is named by:
  // the copy must not share an identity with the profile it came from, or the
  // two roots would fight over one credential-store entry.
  marker.profile_uuid = id;
  marker.credential_account.clear();
  marker.profile_id = id;
  marker.display_name = display_name;
  marker.kind = ProfileKindToString(ProfileKind::kPERSIST);
  marker.package_id = manifest.package_id;
  marker.self_contained = manifest.self_contained;
  marker.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  marker.created_by_version = GetProjectVersion();

  if (!WriteProfileMarker(ProfileMarkerPathFor(profile_root), marker)) {
    return "the profile could not be recorded";
  }

  // The application key inside a package is unprotected — the package's own
  // passphrase is what protected it — and the machine that wrote it may well
  // have recorded "keychain", which here would only strand the profile.
  const auto settings_path = profile_root + "/config/config.ini";
  QSettings settings(settings_path, QSettings::IniFormat);
  settings.setValue("advanced/app_key_protection", "none");
  settings.sync();

  return {};
}

auto ProfileSessionRoot(const QString &profiles_root,
                        const QString &package_path) -> QString {
  if (profiles_root.isEmpty() || package_path.isEmpty()) return {};

  // Canonical where possible so that a package reached through a symlink or a
  // relative path is recognised as the same package, and absolute otherwise so
  // that a destination which does not exist yet still resolves.
  const QFileInfo info(package_path);
  const auto canonical = info.canonicalFilePath();
  const auto path = canonical.isEmpty() ? info.absoluteFilePath() : canonical;

  return QString("%1/.%2").arg(
      profiles_root, ProfilePackageHeaderDigest(path.toUtf8()).left(32));
}

auto SweepTransientProfileRoots(const QString &profiles_root,
                                const QString &keep_root) -> int {
  QDir root(profiles_root);
  if (!root.exists()) return 0;

  const auto keep =
      keep_root.isEmpty() ? QString{} : QFileInfo(keep_root).absoluteFilePath();

  int removed = 0;
  const auto entries =
      root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);

  for (const auto &entry : entries) {
    if (!entry.fileName().startsWith('.')) continue;

    const auto path = entry.absoluteFilePath();
    if (!keep.isEmpty() && path == keep) continue;

    // Only an adopted session, never scratch: staging and extraction trees are
    // dot-prefixed too and never hold a lock, so sweeping one would delete an
    // export that another window is running right now.
    if (!QFileInfo::exists(ProfileMarkerPathFor(path))) continue;

    if (!ProfileLock::Probe(path).Ok()) continue;

    if (QDir(path).removeRecursively()) {
      LOG_I() << "removed a session left behind by a process that is gone:"
              << path;
      ++removed;
    }
  }

  return removed;
}

auto OpenPackageSession(const QString &package_path,
                        const QString &profiles_root,
                        const GFBuffer &passphrase, int this_schema_version)
    -> ProfileSessionOpenResult {
  ProfileSessionOpenResult result;

  const auto session_root = ProfileSessionRoot(profiles_root, package_path);
  if (session_root.isEmpty()) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "there is nowhere to open this package";
    return result;
  }

  // Anything already there belonged to a process that is gone: whoever calls
  // this holds the lock on the session root, so a live session could not have
  // got this far. The lock file itself stays — it is the caller's, and it is
  // what tells the next process this root is in use.
  ClearSessionRootContents(session_root);

  const auto staging = MakeProfilePackageScratchDir(profiles_root, "extract");
  if (staging.isEmpty()) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "a temporary folder could not be made";
    return result;
  }
  const ScratchGuard guard{staging};

  const auto read = ReadProfilePackage(package_path, staging, passphrase);
  if (!read.Ok()) {
    result.status = read.status;
    result.detail = read.detail;
    return result;
  }

  // Before anything is adopted, never after: a package this build must not
  // touch has to leave no trace of having been opened.
  ProfileMarker as_marker;
  as_marker.schema_version = read.manifest.schema_version;
  as_marker.min_reader_version = read.manifest.min_reader_version;
  as_marker.profile = read.manifest.app_profile;
  as_marker.last_writer_version = read.manifest.writer_version;

  if (CheckProfileCompatibility(as_marker, true, this_schema_version) ==
      ProfileCompatibility::kTOO_NEW) {
    result.status = ProfilePackageReadStatus::kTOO_NEW;
    result.detail = read.manifest.writer_version;
    return result;
  }

  const auto display_name = read.manifest.display_name.isEmpty()
                                ? read.manifest.profile_id
                                : read.manifest.display_name;

  const auto error = AdoptExtractedProfile(staging, session_root,
                                           QFileInfo(session_root).fileName(),
                                           display_name, read.manifest);
  if (!error.isEmpty()) {
    RemoveDirectoryQuietly(session_root);
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = error;
    return result;
  }

  result.session_root = session_root;
  result.manifest = read.manifest;
  return result;
}

}  // namespace GpgFrontend
