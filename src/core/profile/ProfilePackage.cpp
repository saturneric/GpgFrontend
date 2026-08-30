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
#include <QScopeGuard>
#include <QUuid>
#include <optional>
#include <thread>

#include "core/function/AESCryptoHelper.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/function/DataObjectOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/model/GFDataExchanger.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileAreaTraits.h"
#include "core/profile/ProfileLock.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileMember.h"
#include "core/profile/ProfilePackageStream.h"
#include "core/profile/ProtectedFsProfileAccessor.h"
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

/// Headroom below which an unpacking failure is read as "the storage filled up"
/// rather than "the package is damaged". Deliberately generous: the last write
/// before ENOSPC often leaves a little back, and guessing wrong the other way
/// blames a file that is fine.
constexpr qint64 kStorageExhaustedSlack = 1024 * 1024;

/// Absolute floor and ceiling for the one-shot payload cap. The floor keeps a
/// container with a tiny locked-memory allowance from refusing every export;
/// the ceiling keeps a machine with none from trying to hold a DVD in memory.
constexpr qint64 kPayloadCapFloor = 16LL * 1024 * 1024;
constexpr qint64 kPayloadCapCeiling = 256LL * 1024 * 1024;

auto DirectorySize(const QString &path) -> qint64 {
  if (path.isEmpty() || !QFileInfo::exists(path)) return 0;

  qint64 total = 0;
  QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    total += it.fileInfo().size();
  }
  return total;
}

/**
 * @brief How much the settings this package would carry actually weigh.
 *
 * The config area and the settings file are not the same thing. An installed
 * profile on Windows keeps its INI in AppConfigLocation and on POSIX writes
 * through Qt's native store; in neither case is there anything under the
 * profile's `config/` to measure, and the export dialog said "0 B" beside
 * "Settings" for every such profile.
 *
 * What travels is the file regenerated from the live settings, so the live
 * file is what this is about. The area is measured too, for whatever else was
 * put there, and the file is added only when it is somewhere else -- counting
 * the ordinary case twice would be a different wrong number.
 *
 * @param storage the session's storage
 * @return bytes the settings occupy
 */
auto SettingsBytes(const ProfileAccessor &storage) -> qint64 {
  const auto dir = storage.PathOf(ProfileArea::kConfig);
  auto total = DirectorySize(dir);

  const auto file = storage.Settings().fileName();
  if (file.isEmpty()) return total;

  const auto path = QFileInfo(file).absoluteFilePath();
  if (dir.isEmpty() || !path.startsWith(QDir(dir).absolutePath() + "/")) {
    total += QFileInfo(path).size();
  }
  return total;
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

auto RenderSettingsIni(const QMap<QString, QVariant> &settings)
    -> std::optional<GFBuffer> {
  QTemporaryDir scratch;
  if (!scratch.isValid()) return {};

  const auto path = scratch.path() + "/config.ini";
  {
    QSettings staged(path, QSettings::IniFormat);
    staged.clear();
    for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
      staged.setValue(it.key(), it.value());
    }
    staged.sync();
    if (staged.status() != QSettings::NoError) return {};
  }

  QByteArray bytes;
  if (!ReadWholeFile(path, 0, bytes)) return {};
  return GFBuffer(bytes);
}

auto WriteProfilePackage(const ArchiveMemberProvider &next,
                         const QString &dest_path,
                         const ProfilePackageHeader &header,
                         const GFBuffer &passphrase)
    -> ProfilePackageWriteResult {
  ProfilePackageWriteResult result;

  auto exchanger = CreateStandardGFDataExchanger();
  GFError archive_error = 0;

  // The archive is produced on another thread because the exchanger is a
  // pipe: it holds a few megabytes and then blocks its writer until someone
  // reads.
  std::thread producer([&]() {
    archive_error = ArchiveFileOperator::NewArchiveFromMembersSync(
        next, exchanger, ArchiveCompression::kGZIP);
  });

  // Joining the producer while it is blocked writing into a full pipe is a
  // deadlock, so nothing may join it without closing the pipe first. Written as
  // a guard rather than as care at each call site because care is exactly what
  // the early returns below were missing, and a fourth one would miss it too.
  //
  // The drain path below joins explicitly, once it has emptied the pipe and can
  // read `archive_error`; this then finds the thread already joined and does
  // nothing.
  struct ProducerGuard {
    std::thread &thread;
    GFDataExchanger &pipe;

    ~ProducerGuard() {
      if (!thread.joinable()) return;
      pipe.CloseWrite();
      thread.join();
    }
  } const producer_guard{producer, *exchanger};

  const auto header_bytes = EncodeProfilePackageHeader(header);

  // Written beside the destination, so the final step is a rename inside
  // one filesystem rather than a copy that could fail halfway. The
  // destination is never opened for writing: a failed save that had
  // truncated it would destroy keys, settings and workspace at once, and
  // this file is the backup.
  const auto temporary_path =
      QString("%1/.%2.tmp-%3")
          .arg(QFileInfo(dest_path).absolutePath(),
               QFileInfo(dest_path).fileName(),
               QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));

  const auto abandon = [&](const QString &why) {
    QFile::remove(temporary_path);
    result.error = why;
    return result;
  };

  qint64 payload_bytes = 0;
  {
    QFile temporary(temporary_path);
    if (!temporary.open(QIODevice::WriteOnly)) {
      result.error = "the package file could not be created";
      return result;
    }

    if (temporary.write(header_bytes) != header_bytes.size()) {
      temporary.close();
      return abandon("the package could not be written");
    }

    // The whole reason for the shape. The archive is drained a chunk at a
    // time and each chunk is sealed and appended as it arrives, so neither the
    // payload nor its ciphertext is ever held whole -- which is what used to
    // cap a package at a couple of hundred megabytes and put several full
    // copies of the plaintext, including the application key, on the ordinary
    // heap for the length of the export.
    ProfilePackageStreamWriter writer(
        [&temporary](const char *data, qint64 length) {
          return temporary.write(data, length) == length;
        },
        passphrase, header.protection == ProfilePackageProtection::kPIN);

    if (!writer.Begin()) {
      temporary.close();
      return abandon("the package could not be encrypted");
    }

    QByteArray chunk(static_cast<int>(kDrainChunk), Qt::Uninitialized);
    bool sink_failed = false;
    while (true) {
      const auto read = exchanger->Read(
          reinterpret_cast<std::byte *>(chunk.data()), kDrainChunk);
      if (read <= 0) break;

      payload_bytes += read;
      if (!writer.Write(chunk.constData(), read)) {
        // Drain to the end regardless: the producer is blocked on this pipe
        // and joining it while it waits would deadlock.
        sink_failed = true;
        while (exchanger->Read(reinterpret_cast<std::byte *>(chunk.data()),
                               kDrainChunk) > 0) {
        }
        break;
      }
    }

    producer.join();

    if (sink_failed) {
      temporary.close();
      return abandon("the package could not be written");
    }
    // `!= 0`, not `< 0`: GFError is uint32_t, so the signed comparison this
    // used to make was a tautology the compiler folded away, and no error the
    // archive producer reported has ever been noticed here.
    if (archive_error != 0 || payload_bytes == 0) {
      temporary.close();
      return abandon("the profile could not be packed");
    }
    if (!writer.Finish()) {
      temporary.close();
      return abandon("the package could not be written");
    }

    temporary.flush();

    // Durable before it is believed: reading back what only the page cache
    // knows would verify nothing about the file that survives a power cut.
#ifdef Q_OS_UNIX
    ::fsync(static_cast<int>(temporary.handle()));
#endif
    temporary.close();
  }

  // A package that cannot be read back is not a package. Read back as a
  // stream and thrown away chunk by chunk rather than compared against a
  // payload held in memory: every chunk carries its own tag, so walking to the
  // final one proves the file authenticates end to end, which is the property
  // the old byte-for-byte comparison was standing in for.
  {
    QFile written(temporary_path);
    if (!written.open(QIODevice::ReadOnly)) {
      return abandon("the package could not be read back");
    }

    // Closed before the temporary is removed. Windows refuses to unlink an open
    // file, so abandoning from in here left a complete encrypted copy of the
    // profile sitting beside the destination.
    const auto give_up = [&](const QString &why) {
      written.close();
      return abandon(why);
    };

    QByteArray head =
        written.read(kProfilePackageMagicLength + 4 + kMaxHeaderLength);
    const auto view = ParseProfilePackageHeader(head);
    if (!view.Ok() || view.header_bytes != header_bytes) {
      return give_up(
          "the package was written but does not read back correctly");
    }

    if (!written.seek(view.body_offset)) {
      return give_up("the package could not be read back");
    }

    ProfilePackageStreamReader reader(
        [&written](char *out, qint64 length) {
          return written.read(out, length);
        },
        passphrase, header.protection == ProfilePackageProtection::kPIN);

    if (!reader.Begin()) {
      return give_up("the package was written but cannot be decrypted");
    }

    qint64 verified = 0;
    GFBuffer chunk;
    const auto forget_chunk = qScopeGuard([&chunk]() { chunk.Zeroize(); });
    while (true) {
      if (!reader.Next(chunk)) {
        return give_up("the package was written but cannot be decrypted");
      }
      if (chunk.Empty() && !reader.Complete()) break;
      verified += static_cast<qint64>(chunk.Size());
      if (reader.Complete()) break;
    }

    if (verified != payload_bytes) {
      return give_up(
          "the package was written but does not read back correctly");
    }
  }

  if (QFileInfo::exists(dest_path) && !QFile::remove(dest_path)) {
    return abandon("the existing package could not be replaced");
  }
  if (!QFile::rename(temporary_path, dest_path)) {
    return abandon("the package could not be moved into place");
  }

  result.ok = true;
  result.bytes = header_bytes.size() + payload_bytes;
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
  json["uncompressed_bytes"] = static_cast<double>(manifest.uncompressed_bytes);
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

  // Absent on every package written before this field existed, and those keep
  // opening: zero means "no declared size", which is what the budget already
  // falls back from.
  manifest.uncompressed_bytes =
      static_cast<qint64>(json["uncompressed_bytes"].toDouble(0));
  if (manifest.uncompressed_bytes < 0) manifest.uncompressed_bytes = 0;
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
      "uncompressed_bytes", "self_contained",     "key_databases"};
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

    // Not under the profile at all: it stays absolute and is marked external,
    // which is the case this has always handled.
    if (rewritten.isEmpty() || !rewritten.startsWith(kProfilePathToken)) {
      out.push_back(item);
      continue;
    }

    // Under the profile, but somewhere the user put it by hand. The packer will
    // not carry that directory, so carrying the reference would make the
    // package claim contents it does not have -- and the recipient's path
    // resolution creates a missing key database directory rather than
    // complaining, leaving them a keyring with no keys and no error.
    //
    // Dropped outright rather than marked external: keeping it would also put
    // the sender's absolute path, usually including their username, inside a
    // file meant for somebody else.
    const auto relative =
        rewritten.mid(static_cast<int>(qstrlen(kProfilePathToken))).mid(1);
    if (!IsManagedKeyDatabasePath(relative)) {
      LOG_I() << "key database not carried by the package:" << item.name;
      continue;
    }

    item.path = rewritten;
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

auto MakeTravellingKeyDatabaseMember(
    const QContainer<KeyDatabaseItemSO> &databases)
    -> std::optional<ProfileMember> {
  KeyDatabaseListSO travelling;
  travelling.key_databases = databases;

  auto sealed = DataObjectOperator::GetInstance().SealDataObjForPackage(
      kKeyDatabaseListObject, QJsonDocument(travelling.ToJson()));
  if (!sealed) {
    LOG_E() << "the key database list could not be sealed for the package";
    return {};
  }

  ProfileMember member;
  member.path = QString("data_objs/%1").arg(sealed->first);
  member.area = ProfileArea::kDataObjects;
  member.bytes = sealed->second;
  return member;
}

auto MeasureProfileAreas(const ProfileAccessor &storage)
    -> QMap<QString, qint64> {
  // Asked of the storage, not of the tree under the root. Walking the tree was
  // right only for a profile whose every area is a directory, and two are not:
  // a packaged session holds `secure` in memory, where a walk finds nothing,
  // and an installed profile keeps its settings file outside the root on
  // Windows and outside the profile entirely on POSIX. Both reported 0 bytes --
  // the row for the profile's own key, and the row for the settings, in the
  // dialog whose whole job is to say what is about to be handed to somebody.
  QMap<QString, qint64> areas;

  areas["config"] = SettingsBytes(storage);
  areas["data_objs"] = storage.TotalSize(ProfileArea::kDataObjects, "*");
  areas["secure"] = storage.TotalSize(ProfileArea::kSecure, "*");

  // Key databases have no ProfileArea of their own: they are gpg-agent's
  // working directories, which nothing but a filesystem can host, so the tree
  // is the only place to measure them.
  const auto root = storage.PathOf(ProfileArea::kRoot);
  areas["key_databases"] = root.isEmpty() ? 0
                                          : DirectorySize(root + "/db") +
                                                DirectorySize(root + "/dbs");

  areas["workspace"] = DirectorySize(storage.PathOf(ProfileArea::kWorkspace));
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

auto SettingTravelsInPackage(const QString &key) -> bool {
  // Each of these names something that exists on one computer and not on the
  // next one. Carried, they do not merely fail to apply -- they actively point
  // the copy at the wrong thing: a gpgconf that is not there, a workspace
  // folder that is not there, a credential store this platform does not have.
  static const QSet<QString> kStaysHome = {
      // An absolute path to a GnuPG installation, and the flag that turns it
      // on. Startup falls back when it does not resolve, but only after
      // reporting an illegal path and prepending it to PATH.
      "gnupg/custom_gnupg_install_path",
      "gnupg/use_custom_gnupg_install_path",

      // An absolute override for where the profile's workspace lives.
      "workspace/path",

      // Which credential store held the key here. The key inside a package is
      // not sealed by any store -- the package's own passphrase is what
      // protected it -- so the recipient must decide this for themselves.
      "advanced/os_secret_store",
      "advanced/app_key_protection",
  };

  return !kStaysHome.contains(key);
}

auto SnapshotSettings(QSettings &settings) -> QMap<QString, QVariant> {
  settings.sync();

  QMap<QString, QVariant> snapshot;
  for (const auto &key : settings.allKeys()) {
    if (!SettingTravelsInPackage(key)) continue;
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

auto PeekProfilePackageManifest(const QString &package_path,
                                const GFBuffer &passphrase)
    -> ProfilePackageReadResult {
  auto result = InspectProfilePackage(package_path);
  if (!result.Ok()) return result;

  // Only a streamed package is worth looking inside for this. A version 1
  // manifest never carried a size, so reading one would cost a full pass to
  // learn nothing -- and version 1 makes no promise the manifest comes first,
  // so that pass could pull an arbitrary member into memory on the way.
  if (result.header.format_version < kProfilePackageStreamedFrom) return result;

  QFile file(package_path);
  if (!file.open(QIODevice::ReadOnly)) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "this file could not be read";
    return result;
  }

  const auto head =
      file.read(kProfilePackageMagicLength + 4 + kMaxHeaderLength);
  const auto view = ParseProfilePackageHeader(head);
  if (!view.Ok() || !file.seek(view.body_offset)) {
    result.status = ProfilePackageReadStatus::kMALFORMED;
    result.detail = "this package does not describe itself";
    return result;
  }

  // Made before the feeder starts, so a failure here is a plain return rather
  // than something that has to unwind a running thread. Nothing is ever written
  // into it -- every entry is claimed -- but the extractor still insists on a
  // destination, and an invalid one would hand it an empty path.
  QTemporaryDir nowhere;
  if (!nowhere.isValid()) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "a temporary folder could not be made";
    return result;
  }

  auto exchanger = CreateStandardGFDataExchanger();

  // Set by the feeder and read after it is joined. This is the one failure that
  // means "the passphrase does not open this package": every other way of
  // arriving without a manifest is something else entirely, and reporting them
  // all as a wrong passphrase sends a user typing the right one over and over
  // at a file that is damaged.
  bool body_ok = true;

  std::thread feeder([&]() {
    ProfilePackageStreamReader reader(
        [&file](char *out, qint64 length) { return file.read(out, length); },
        passphrase, view.header.protection == ProfilePackageProtection::kPIN);

    if (!reader.Begin()) {
      body_ok = false;
      exchanger->CloseWrite();
      return;
    }

    GFBuffer chunk;
    const auto forget_chunk = qScopeGuard([&chunk]() { chunk.Zeroize(); });
    while (true) {
      if (!reader.Next(chunk)) {
        body_ok = false;
        break;
      }
      if (chunk.Empty()) break;

      // Stops the moment nobody is reading any more, and not a body failure:
      // the extraction below refuses everything after the manifest, and without
      // this the feeder went on decrypting the whole package into a pipe that
      // was closed -- so opening a profile paid for two full passes over it
      // rather than the chunk or two this is documented to need.
      if (exchanger->Write(reinterpret_cast<const std::byte *>(chunk.Data()),
                           static_cast<ssize_t>(chunk.Size())) < 0) {
        break;
      }
      if (reader.Complete()) break;
    }
    exchanger->CloseWrite();
  });

  // Nothing is written: every entry is claimed, and the one that matters is
  // kept. Refusing the entry *after* it is what stops the walk, so a package
  // is opened far enough to describe itself and no further.
  QByteArray manifest_bytes;
  bool have_manifest = false;

  const auto error = ArchiveFileOperator::ExtractArchiveFromDataExchangerSync(
      exchanger, nowhere.path(),
      ArchiveExtractPolicy::Strict(kMaxHeaderLength * 16, 200000),
      [](const QString &) { return true; },
      [&](const QString &path, const GFBuffer &bytes) {
        if (have_manifest) return false;
        if (path == "manifest.json") {
          manifest_bytes = bytes.ConvertToQByteArray();
          have_manifest = true;
        }
        return true;
      });

  exchanger->CloseWrite();
  feeder.join();

  // `error` is expected to be non-zero: refusing the entry after the manifest
  // is how the walk was stopped, and that refusal is indistinguishable from any
  // other. So it says nothing on its own, and what went wrong is read from the
  // two things that do.
  Q_UNUSED(error)

  if (!have_manifest) {
    // The chunks would not authenticate. A wrong passphrase and a damaged body
    // are the same event to the stream, and the passphrase is far likelier.
    if (!body_ok) {
      result.status = ProfilePackageReadStatus::kBAD_PASSPHRASE;
      result.detail = "the passphrase does not open this package";
      return result;
    }

    // The body opened and the manifest still did not arrive, so this is not
    // about the passphrase at all: a package whose first member is something
    // else, or one whose archive could not be walked.
    result.status = ProfilePackageReadStatus::kMALFORMED;
    result.detail = "this package does not describe itself";
    return result;
  }

  auto manifest = ParseProfilePackageManifest(manifest_bytes);
  if (!manifest) {
    result.status = ProfilePackageReadStatus::kMALFORMED;
    result.detail = "this package does not describe itself";
    return result;
  }

  result.manifest = *manifest;
  return result;
}

auto ReadProfilePackage(const QString &package_path, const QString &staging_dir,
                        const GFBuffer &passphrase,
                        const ProfileExtractionRouting &routing)
    -> ProfilePackageReadResult {
  ProfilePackageReadResult result;

  const auto file_bytes = QFileInfo(package_path).size();

  // Only as much as the routing header can occupy. Which shape the body is in
  // decides how much more may be read, so nothing larger than this is touched
  // before that question has an answer.
  QByteArray head;
  {
    QFile file(package_path);
    if (!file.open(QIODevice::ReadOnly)) {
      result.status = ProfilePackageReadStatus::kIO_FAILED;
      result.detail = "this file could not be read";
      return result;
    }
    head = file.read(kProfilePackageMagicLength + 4 + kMaxHeaderLength);
  }

  const auto view = ParseProfilePackageHeader(head);
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

  const auto streamed =
      view.header.format_version >= kProfilePackageStreamedFrom;

  // A version 1 body is one sealed block and has to be opened as one. Version 2
  // is a stream of chunks, each authenticated on its own, and is never held
  // whole -- so the payload buffer below, and the ceiling that guards it, exist
  // only for the legacy shape.
  QByteArray legacy_payload;
  if (!streamed) {
    // Opening a version 1 body holds it whole and then the plaintext beside it,
    // so a file several times larger than memory takes the process down part
    // way through mounting -- with the profile lock already held, which is a
    // worse state to leave behind than a refusal. The file's own size is the
    // only thing checkable before making the allocation it guards.
    //
    // Deliberately *not* applied to a streamed body: capping that would put
    // back the very limit streaming was built to remove.
    const auto cap = ProfilePackagePayloadCap();
    if (file_bytes > cap) {
      result.status = ProfilePackageReadStatus::kTOO_LARGE;
      // Both numbers, because "too large" without them is not something a user
      // can act on and not something a bug report can be written from.
      result.detail =
          QString("%1 / %2").arg(QLocale().formattedDataSize(file_bytes),
                                 QLocale().formattedDataSize(cap));
      return result;
    }

    QByteArray bytes;
    if (!ReadWholeFile(package_path, 0, bytes)) {
      result.status = ProfilePackageReadStatus::kIO_FAILED;
      result.detail = "this file could not be read";
      return result;
    }

    if (view.header.protection == ProfilePackageProtection::kPIN) {
      const auto decrypted = AESCryptoHelper::Decrypt(
          passphrase, GFBuffer(bytes.mid(view.body_offset)));
      if (!decrypted) {
        result.status = ProfilePackageReadStatus::kBAD_PASSPHRASE;
        result.detail = "the passphrase does not open this package";
        return result;
      }
      legacy_payload = decrypted->ConvertToQByteArray();
    } else {
      legacy_payload = bytes.mid(view.body_offset);
    }
  }

  // Derived from what the file weighs rather than from anything it claims about
  // itself: an archive that says it is small and is not dies early instead of
  // filling the disk.
  auto policy = ArchiveExtractPolicy::Strict(
      qMax(static_cast<qint64>(64) * 1024 * 1024, file_bytes * 100), 200000);

  auto exchanger = CreateStandardGFDataExchanger();

  // Set by the feeder and read after it is joined, so a body that stops
  // authenticating part way through is reported as such rather than as a
  // truncated archive.
  bool body_ok = true;

  // The other reason the feeder stops early, and not a body failure at all: the
  // extraction gave up and closed the pipe. Told apart because the two are
  // reported to the user as completely different things -- a wrong passphrase,
  // or a package that could not be unpacked -- and an unpack that failed for a
  // reason of the recipient's platform was being called a wrong passphrase,
  // which the open path then re-prompts on forever.
  bool sink_closed = false;

  std::thread feeder([&]() {
    if (!streamed) {
      exchanger->Write(
          reinterpret_cast<const std::byte *>(legacy_payload.constData()),
          legacy_payload.size());
      exchanger->CloseWrite();
      return;
    }

    QFile file(package_path);
    if (!file.open(QIODevice::ReadOnly) || !file.seek(view.body_offset)) {
      body_ok = false;
      exchanger->CloseWrite();
      return;
    }

    ProfilePackageStreamReader reader(
        [&file](char *out, qint64 length) { return file.read(out, length); },
        passphrase, view.header.protection == ProfilePackageProtection::kPIN);

    if (!reader.Begin()) {
      body_ok = false;
      exchanger->CloseWrite();
      return;
    }

    GFBuffer chunk;
    const auto forget_chunk = qScopeGuard([&chunk]() { chunk.Zeroize(); });
    while (true) {
      if (!reader.Next(chunk)) {
        body_ok = false;
        break;
      }
      if (chunk.Empty()) break;

      // A closed pipe means the extraction has already given up, so there is
      // nothing left to decrypt for.
      if (exchanger->Write(reinterpret_cast<const std::byte *>(chunk.Data()),
                           static_cast<ssize_t>(chunk.Size())) < 0) {
        sink_closed = true;
        break;
      }
      if (reader.Complete()) break;
    }

    // A stream that ran out without its final tag was cut short, which a
    // length-prefixed format cannot otherwise tell from a smaller profile.
    // Unless nobody was reading any more: then the stream did not run out, it
    // was abandoned, and what went wrong is the extraction's to report.
    if (body_ok && !sink_closed && !reader.Complete()) body_ok = false;

    exchanger->CloseWrite();
  });

  // An area the driver holds in memory must not be written down on the way
  // there. Entries under it are read straight out of the archive and handed to
  // the storage, so the profile's own key never exists as a file at all --
  // which is the difference between this and unpacking it and deleting it
  // afterwards, since deleting does not erase.
  ArchiveEntryFilter divert;
  ArchiveEntrySink sink;
  if (routing.Active()) {
    const auto prefixes = routing.resident_dirs;
    divert = [prefixes](const QString &path) {
      // Archive-relative here: "profile/secure/app.key".
      const auto parts = path.split('/', Qt::SkipEmptyParts);
      if (parts.size() < 2 || parts.first() != kProfilePackageTreePrefix) {
        return false;
      }
      return prefixes.contains(parts.at(1));
    };
    sink = [&routing](const QString &path, const GFBuffer &bytes) {
      // Handed on profile-relative, the one namespace members speak.
      return routing.store(path.section('/', 1), bytes);
    };
  }

  QString unpack_reason;
  const auto error = ArchiveFileOperator::ExtractArchiveFromDataExchangerSync(
      exchanger, staging_dir, policy, divert, sink, &unpack_reason);

  // Before the join, always. The exchanger is a bounded ring and its writer
  // blocks when it fills, so a feeder still pushing when the extraction gives
  // up -- a malformed archive, a refused entry, a sink that said no -- would
  // wait for a reader that is never coming back, and join() would hang the
  // process with the profile lock held.
  //
  // Closing here is what wakes it: Write() returns -1 once close_ is set, and
  // the feeder unwinds. Cheap when extraction succeeded, since the feeder has
  // already finished and closed.
  exchanger->CloseWrite();
  feeder.join();

  if (!body_ok) {
    RemoveDirectoryQuietly(staging_dir);
    // The wrong passphrase and a damaged chunk are the same event to the
    // stream, and the passphrase is overwhelmingly the likelier of the two.
    result.status = ProfilePackageReadStatus::kBAD_PASSPHRASE;
    result.detail = "the passphrase does not open this package";
    return result;
  }

  // `!= 0`, not `< 0`: GFError is unsigned, so the comparison this used to make
  // was always false and an extraction that gave up part way through was
  // adopted as if it had finished.
  if (error != 0) {
    RemoveDirectoryQuietly(staging_dir);
    result.status = ProfilePackageReadStatus::kMALFORMED;
    // The reason, whenever the walk knows one. A package that will not open is
    // exactly the moment a user needs to be told which entry stopped it and
    // why -- the alternative is a sentence they can do nothing with and a log
    // file they have no reason to know exists.
    result.detail =
        unpack_reason.isEmpty()
            ? QString("the package's contents could not be unpacked")
            : QString(
                  "the package's contents could not be "
                  "unpacked: %1")
                  .arg(unpack_reason);
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

auto ExportProfilePackage(ProfileExportRequest request)
    -> ProfilePackageWriteResult {
  // Taken by value so this can be done at all, and done on every exit. The
  // members carry the profile's own key in the clear -- they are the only way
  // it reaches a package now -- and an export that merely dropped them left it
  // in memory for as long as the task object that captured the request lived.
  const auto forget_secure_members = qScopeGuard([&request]() {
    for (auto &member : request.secure_members) member.bytes.Zeroize();
  });

  ProfilePackageWriteResult result;

  // A package cannot declare a passphrase it does not have. The reader picks
  // its framing from the header, so writing this would produce a file whose
  // header says "protected" over a body that is plaintext -- the whole profile
  // and its application key in the clear, in a file the user was told was
  // sealed. Refused here rather than deep in the writer, where the destination
  // has already been chosen.
  if (request.protection == ProfilePackageProtection::kPIN &&
      request.passphrase.Empty()) {
    result.error = "a protected package cannot be written without a passphrase";
    return result;
  }

  // Everything the package carries, resolved before anything is written. The
  // secure area is the reason it has to be: a packaged session holds it in
  // memory, so there is nothing on the storage to copy, and the two kinds of
  // key in it are sourced differently -- see ResolveSecureAreaMembers().
  QList<ProfileMember> synthesised = request.secure_members;
  if (synthesised.isEmpty()) {
    result.error = "the application key could not be written into the package";
    return result;
  }

  auto settings_ini = RenderSettingsIni(request.settings);
  if (!settings_ini) {
    result.error = "the settings could not be written into the package";
    return result;
  }

  ProfileMember config;
  config.path = "config/config.ini";
  config.area = ProfileArea::kConfig;
  config.bytes = *settings_ini;
  synthesised.append(config);

  // The data objects that travel rewritten rather than copied. Added to
  // `synthesised` before the superseded set is taken from it, so the walk below
  // neither yields nor measures the copy on disk and the recipient reads
  // exactly one version of each.
  for (const auto &member : request.data_object_members) {
    if (member.path.isEmpty()) continue;
    synthesised.append(member);
  }

  // Walked before a byte is written, and before the manifest is built: the
  // destination may already hold the only copy of this profile, and the
  // manifest has to declare how big the tree is.
  //
  // The synthesised names are handed over so the walk neither yields nor
  // measures them. `config/config.ini` is the live case: the area table is
  // explicit that the file which travels is regenerated from the live settings
  // rather than copied, but `config` is packed from the tree as well, so the
  // walk offered the on-disk copy too. Both went into the archive under one
  // name and the later one won -- the stale one -- and its bytes were counted
  // twice into the size the recipient provisions from.
  QSet<QString> synthesised_paths;
  for (const auto &member : synthesised) synthesised_paths.insert(member.path);

  TreeMemberSource tree(request.profile_root, request.include_workspace, {},
                        synthesised_paths);
  if (const auto error = tree.Prepare(); !error.isEmpty()) {
    result.error = error;
    return result;
  }
  const auto tree_bytes = tree.Bytes();

  ProfilePackageHeader header;
  header.writer = GetProjectVersion();
  header.writer_stable = IsStableBuild();
  header.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  header.protection = request.protection;

  auto manifest = request.manifest;

  // What the recipient will need room for. Known exactly here and nowhere else:
  // the walk has already measured the tree, and the members above are bytes in
  // hand. Recording it is what stops the other machine sizing its storage from
  // the compressed size and coming up short on a profile that compressed well.
  //
  // The manifest's own bytes are not counted, and cannot be -- the figure is
  // inside them. A few hundred bytes against a whole profile, and the budget
  // built from this is deliberately generous anyway.
  qint64 unpacked = tree_bytes;
  for (const auto &member : synthesised) {
    unpacked += static_cast<qint64>(member.bytes.Size());
  }
  manifest.uncompressed_bytes = unpacked;

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

  // No staging tree. The archive is fed from the live profile and from the
  // members above, so the full plaintext copy that used to be written out
  // first -- an unprotected application key in it -- never exists at all.
  //
  // That copy was also why a session needed twice its own size in free space
  // to save itself.
  ListMemberSource extra(synthesised);

  // The manifest goes first, by construction: it is what a reader needs before
  // it can decide anything about the rest, and it sits beside the tree rather
  // than inside it.
  bool manifest_written = false;
  bool extras_done = false;
  const auto manifest_bytes = EncodeProfilePackageManifest(manifest);

  const auto next = [&](ArchiveMemberEntry &out) -> bool {
    if (!manifest_written) {
      manifest_written = true;
      out.relative_path = "manifest.json";
      out.bytes = GFBuffer(manifest_bytes);
      return true;
    }

    ProfileMember member;
    if (!extras_done) {
      if (extra.Next(member)) {
        out.relative_path =
            QString(kProfilePackageTreePrefix) + "/" + member.path;
        out.bytes = member.bytes;
        return true;
      }
      extras_done = true;
    }

    if (!tree.Next(member)) return false;

    out.relative_path = QString(kProfilePackageTreePrefix) + "/" + member.path;
    out.directory = member.directory;
    // Carried as a path, so a workspace larger than memory is not a reason a
    // profile cannot be packaged.
    out.source_file = member.source_path;
    return true;
  };

  auto written =
      WriteProfilePackage(next, request.dest_path, header, request.passphrase);

  // Reported rather than merely done. Failing closed silently is its own trap:
  // the sender should find out here, not by hearing that the copy on another
  // machine was missing something.
  if (written.ok) written.skipped = tree.Skipped();
  return written;
}

namespace {

/// Copy a tree wholesale, links and all skipped, for the case where a rename
/// cannot cross from where the staging is to where the profile goes.
auto CopyTreeAcross(const QString &source, const QString &destination) -> bool {
  const QFileInfo info(source);

  if (info.isSymLink()) return false;  // a package carries no links, by design

  if (info.isDir()) {
    if (!QDir().mkpath(destination)) return false;
    QDir dir(source);
    for (const auto &entry : dir.entryInfoList(
             QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
      if (!CopyTreeAcross(entry.absoluteFilePath(),
                          destination + "/" + entry.fileName())) {
        return false;
      }
    }
    return true;
  }

  return QFile::copy(source, destination);
}

}  // namespace

auto MoveTreeAcrossFilesystems(const QString &source,
                               const QString &destination) -> bool {
  // Refused before anything is attempted, because the cleanup below removes the
  // destination and must only ever remove what this call put there. Without
  // this, a failed copy onto an existing file deletes the file that was already
  // there — which is the profile the caller was trying not to overwrite.
  if (QFileInfo::exists(destination)) return false;

  // The cheap path, and the only one taken when staging and destination share a
  // filesystem — which a session's own staging always does, because it is an
  // area under the same root.
  if (QDir().rename(source, destination)) return true;

  // An import is the case that genuinely crosses: the staging is wherever the
  // session storage was provisioned, and the copy being made is a permanent
  // profile in the profiles folder. QDir::rename cannot cross a mount point, so
  // without this an import from protected storage fails with a message about
  // the package being unreadable.
  if (!CopyTreeAcross(source, destination)) {
    // Half a copy is worse than none: it would be adopted as a profile with
    // pieces missing. Safe to remove wholesale only because nothing was there
    // before this call — checked above.
    RemoveDirectoryQuietly(destination);
    QFile::remove(destination);
    return false;
  }

  // The source is staging and is about to be swept anyway, so failing to remove
  // it is untidy rather than wrong.
  if (QFileInfo(source).isDir()) {
    RemoveDirectoryQuietly(source);
  } else {
    QFile::remove(source);
  }
  return true;
}

auto AdoptExtractedProfile(const QString &staging_dir,
                           const QString &profile_root, const QString &id,
                           const QString &display_name,
                           const ProfilePackageManifest &manifest,
                           const ProfileTreeMover &mover) -> QString {
  const auto &move =
      mover ? mover : ProfileTreeMover(MoveTreeAcrossFilesystems);

  const auto tree = staging_dir + "/" + kProfilePackageTreePrefix;
  if (!QFileInfo::exists(tree)) return "the package carries no profile";

  if (!QDir().mkpath(profile_root)) {
    return "the profile folder could not be created";
  }

  // Moved entry by entry rather than renamed wholesale, because the destination
  // already exists: it is provisioned before anything is extracted into it, and
  // for the disk driver the profile lock lives there too.
  QDir source(tree);
  for (const auto &entry : source.entryInfoList(
           QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
    const auto destination = profile_root + "/" + entry.fileName();
    if (QFileInfo::exists(destination)) {
      return QString("%1 is already there").arg(entry.fileName());
    }
    if (!move(entry.absoluteFilePath(), destination)) {
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

auto SessionPointerPathFor(const QString &anchor) -> QString {
  return anchor + "/session.json";
}

auto WriteSessionPointer(const QString &anchor, const QJsonObject &state)
    -> bool {
  if (anchor.isEmpty()) return false;
  if (!QDir().mkpath(anchor)) return false;

  QFile file(SessionPointerPathFor(anchor));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

  file.write(QJsonDocument(state).toJson(QJsonDocument::Compact));
  file.close();
  return true;
}

auto ReadSessionPointer(const QString &anchor) -> QJsonObject {
  QFile file(SessionPointerPathFor(anchor));
  if (!file.open(QIODevice::ReadOnly)) return {};

  const auto document = QJsonDocument::fromJson(file.readAll());
  return document.isObject() ? document.object() : QJsonObject{};
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

    // An anchor names storage that may not be here at all, and a session root
    // from an older build is the tree itself. Either is a session; a staging or
    // extraction tree is neither, is dot-prefixed too, and never holds a lock —
    // sweeping one would delete an export another window is running right now.
    const auto pointer = ReadSessionPointer(path);
    const bool is_anchor = !pointer.isEmpty();
    const bool is_legacy_session =
        QFileInfo::exists(ProfileMarkerPathFor(path));

    if (!is_anchor && !is_legacy_session) continue;

    if (!ProfileLock::Probe(path).Ok()) continue;

    // The storage first, because the anchor is the only thing that knows where
    // it went. Losing the pointer before following it would strand a tree full
    // of somebody else's key material somewhere nothing thinks to look -- and,
    // for an encrypted driver, a key in the kernel that would stay readable to
    // this user until the machine is rebooted.
    //
    // Which is also why the answer is not ignored. A key cannot be found again
    // by looking around: no filesystem names it, so the pointer is the only
    // record there is. When the release did not finish, the anchor stays and
    // the next sweep tries again -- and a release that can never finish reports
    // itself done rather than false, so nothing is kept for a retry that would
    // reach the same dead end.
    if (is_anchor && !ReleaseStrandedSessionStorage(pointer, path)) {
      LOG_W() << "keeping the anchor of a session that could not be released:"
              << path;
      continue;
    }

    if (QDir(path).removeRecursively()) {
      LOG_I() << "removed a session left behind by a process that is gone:"
              << path;
      ++removed;
    }
  }

  return removed;
}

auto OpenPackageSession(const QString &package_path, ProfileAccessor &storage,
                        const GFBuffer &passphrase, int this_schema_version)
    -> ProfileSessionOpenResult {
  ProfileSessionOpenResult result;

  const auto session_root = storage.PathOf(ProfileArea::kRoot);
  if (session_root.isEmpty()) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "there is nowhere to open this package";
    return result;
  }

  // Anything already there belonged to a process that is gone: whoever calls
  // this holds the lock on this package, so a live session could not have got
  // this far. The lock file, where it lives here at all, is the caller's and is
  // what tells the next process this package is in use.
  ClearSessionRootContents(session_root);

  // Staged inside the storage rather than beside it: the staging tree is a full
  // plaintext copy of somebody else's profile, so it belongs wherever the
  // driver decided that may be kept. It also keeps the move below on one
  // filesystem, which a rename needs.
  if (!storage.Ensure(ProfileArea::kScratch)) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "a temporary folder could not be made";
    return result;
  }

  const auto staging = MakeProfilePackageScratchDir(
      storage.PathOf(ProfileArea::kScratch), "extract");
  if (staging.isEmpty()) {
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = "a temporary folder could not be made";
    return result;
  }
  const ScratchGuard guard{staging};

  // Whatever the driver holds in memory, the extraction routes there instead
  // of onto the storage. Read off the accessor rather than hardcoded, so an
  // area becoming resident later needs no change here.
  ProfileExtractionRouting routing;
  for (const auto &row : ProfileAreaTable()) {
    if (!row.area.has_value()) continue;
    if (!storage.IsAreaResident(*row.area)) continue;
    routing.resident_dirs << QString(row.dir);
  }
  // What the routing put into the storage, so a failure can take it back out
  // again. The staging tree is removed by its guard and the session root is
  // cleared on the way in, but a resident area is neither: objects already
  // handed to the driver would otherwise outlive the failed open and leave
  // another profile's key material in this session until the next mount.
  QList<QPair<ProfileArea, QString>> routed;
  auto unroute = [&storage, &routed]() {
    for (const auto &object : routed)
      storage.Remove(object.first, object.second);
    routed.clear();
  };

  if (!routing.resident_dirs.isEmpty()) {
    routing.store = [&storage, &routed](const QString &relative,
                                        const GFBuffer &bytes) {
      const auto *traits = TraitsForTopLevel(relative.section('/', 0, 0));
      if (traits == nullptr || !traits->area.has_value()) return false;

      // An empty top-level segment resolves to the root row, whose directory
      // name is the empty string. Nothing reaches here with one today, but this
      // decides where somebody else's bytes land.
      if (relative.section('/', 0, 0).isEmpty()) return false;

      const auto name = relative.section('/', 1);
      if (name.isEmpty() || name.contains('/')) return false;
      if (!storage.Write(*traits->area, name, bytes)) return false;

      routed.append({*traits->area, name});
      return true;
    };
  }

  const auto read =
      ReadProfilePackage(package_path, staging, passphrase, routing);
  if (!read.Ok()) {
    result.status = read.status;
    result.detail = read.detail;

    // Asked only once it has already failed, and only to tell the user
    // something they can act on: an unpacking failure with the storage full is
    // not a damaged package, and reporting it as one sends them looking for a
    // problem that is not there.
    if (read.status == ProfilePackageReadStatus::kIO_FAILED) {
      const auto free_bytes = storage.FreeBytes();
      if (free_bytes >= 0 && free_bytes < kStorageExhaustedSlack) {
        result.status = ProfilePackageReadStatus::kNO_SPACE;
        result.detail = storage.Label();
      }
    }
    unroute();
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
    unroute();
    return result;
  }

  const auto display_name = read.manifest.display_name.isEmpty()
                                ? read.manifest.profile_id
                                : read.manifest.display_name;

  const auto error = AdoptExtractedProfile(staging, session_root,
                                           QFileInfo(session_root).fileName(),
                                           display_name, read.manifest);
  if (!error.isEmpty()) {
    // Emptied, not removed. The lock file in here is the caller's, and it is
    // what tells the next process this package is in use -- removing the root
    // wholesale takes it with it and hands the package to a second window while
    // this one is still holding it. ClearSessionRootContents() is careful about
    // exactly this on the way in; the failure path has to be too.
    ClearSessionRootContents(session_root);
    unroute();
    result.status = ProfilePackageReadStatus::kIO_FAILED;
    result.detail = error;
    return result;
  }

  result.session_root = session_root;
  result.manifest = read.manifest;
  return result;
}

}  // namespace GpgFrontend
