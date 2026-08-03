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

#include "ProfileRegistry.h"

#include <QLockFile>

#include "core/function/GlobalSettingStation.h"
#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend {

namespace {

constexpr int kRegistrySchemaVersion = 1;
constexpr int kRegistryLockTimeoutMs = 5000;

auto RegistryPath(const QString& profiles_root) -> QString {
  return profiles_root + "/profiles.json";
}

auto RegistryLockPath(const QString& profiles_root) -> QString {
  return profiles_root + "/profiles.lock";
}

auto EntryToJson(const ProfileRegistryEntry& e) -> QJsonObject {
  QJsonObject obj;
  obj["id"] = e.id;
  obj["root"] = e.root;
  obj["name"] = e.name;
  obj["kind"] = ProfileRootKindToString(e.kind);
  if (!e.last_opened.isEmpty()) obj["last_opened"] = e.last_opened;
  if (!e.package_id.isEmpty()) obj["package_id"] = e.package_id;
  if (!e.source_package.isEmpty()) obj["source_package"] = e.source_package;
  if (!e.source_bookmark.isEmpty()) obj["source_bookmark"] = e.source_bookmark;
  return obj;
}

auto EntryFromJson(const QJsonObject& obj) -> ProfileRegistryEntry {
  ProfileRegistryEntry e;
  e.id = obj.value("id").toString();
  e.root = obj.value("root").toString();
  e.name = obj.value("name").toString();
  e.kind = ProfileRootKindFromString(obj.value("kind").toString());
  e.last_opened = obj.value("last_opened").toString();
  e.package_id = obj.value("package_id").toString();
  e.source_package = obj.value("source_package").toString();
  e.source_bookmark = obj.value("source_bookmark").toString();
  return e;
}

/// Move a registry that will not parse aside rather than deleting it. It is the
/// only record of where this machine's profiles are; a user may well want to
/// pick it apart by hand.
void QuarantineRegistry(const QString& path) {
  for (int n = 1; n < 100; ++n) {
    const auto candidate = QString("%1.corrupt-%2").arg(path).arg(n);
    if (QFileInfo::exists(candidate)) continue;
    if (QFile::rename(path, candidate)) {
      LOG_W() << "unparsable profile registry moved aside to" << candidate;
    }
    return;
  }
}

auto ReadRegistryFile(const QString& profiles_root) -> ProfileRegistryData {
  ProfileRegistryData data;
  const auto path = RegistryPath(profiles_root);

  QFile file(path);
  if (!file.exists()) return data;
  if (!file.open(QIODevice::ReadOnly)) {
    LOG_W() << "cannot read profile registry:" << path;
    return data;
  }

  QJsonParseError error{};
  const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
  file.close();

  if (error.error != QJsonParseError::NoError || !doc.isObject()) {
    LOG_W() << "profile registry is not valid json:" << path;
    QuarantineRegistry(path);
    return data;
  }

  const auto obj = doc.object();
  data.schema_version = obj.value("schema_version").toInt(1);
  data.min_reader_version = obj.value("min_reader_version").toInt(1);
  data.last_used = obj.value("last_used").toString();
  data.startup_policy =
      ProfileStartupPolicyFromString(obj.value("startup_policy").toString());
  data.startup_profile = obj.value("startup_profile").toString();
  data.save_on_close = obj.value("save_on_close").toString("ask");

  for (const auto& entry : obj.value("profiles").toArray()) {
    data.profiles.append(EntryFromJson(entry.toObject()));
  }
  return data;
}

}  // namespace

auto ProfileRegistryData::Find(const QString& id) const
    -> std::optional<ProfileRegistryEntry> {
  for (const auto& e : profiles) {
    if (e.id == id) return e;
  }
  return {};
}

auto ScanProfilesRoot(const QString& profiles_root)
    -> QList<ProfileRegistryEntry> {
  QList<ProfileRegistryEntry> out;

  QDir dir(profiles_root);
  if (!dir.exists()) return out;

  const auto names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  for (const auto& name : names) {
    // staging and extraction scratch; adopting a half-extracted tree as a real
    // profile would be worse than having no profile at all
    if (name.startsWith('.')) continue;
    if (!IsValidProfileId(name)) continue;

    const auto root = dir.absoluteFilePath(name);
    if (!QFileInfo::exists(ProfileMarkerPathFor(root))) continue;

    ProfileRegistryEntry e;
    e.id = name;
    e.root = root;
    e.kind = ProfileRootKind::kNAMED;

    // The marker is where the profile's own view of itself lives, so a display
    // name or package identity survives losing the registry entirely.
    if (const auto marker = ReadProfileMarker(ProfileMarkerPathFor(root))) {
      if (!marker->display_name.isEmpty()) e.name = marker->display_name;
      if (!marker->kind.isEmpty()) {
        e.kind = ProfileRootKindFromString(marker->kind);
      }
      e.package_id = marker->package_id;
    }
    if (e.name.isEmpty()) e.name = name;

    out.append(e);
  }
  return out;
}

auto ReconcileProfileRegistry(const QList<ProfileRegistryEntry>& scanned,
                              const ProfileRegistryData& stored)
    -> ProfileRegistryData {
  ProfileRegistryData out;
  out.schema_version = kRegistrySchemaVersion;
  out.min_reader_version = 1;
  out.startup_policy = stored.startup_policy;
  out.startup_profile = stored.startup_profile;
  out.save_on_close = stored.save_on_close;
  out.last_used = stored.last_used;

  QHash<QString, ProfileRegistryEntry> by_id;
  for (const auto& e : stored.profiles) {
    if (e.id.isEmpty()) continue;
    // last stored copy of a duplicated id wins; keeping both would give the
    // user two rows that mean the same directory
    by_id.insert(e.id, e);
  }

  QSet<QString> seen;
  for (const auto& found : scanned) {
    seen.insert(found.id);

    auto entry = found;
    if (const auto it = by_id.constFind(found.id); it != by_id.constEnd()) {
      // metadata the filesystem cannot know is carried over; everything the
      // scan established wins, because the directory is the truth
      entry.last_opened = it->last_opened;
      entry.source_package = it->source_package;
      entry.source_bookmark = it->source_bookmark;
      if (entry.name.isEmpty()) entry.name = it->name;
      if (entry.package_id.isEmpty()) entry.package_id = it->package_id;
    }
    out.profiles.append(entry);
  }

  // A stored entry with no directory is gone. Dropping it silently is right:
  // the user deleted the folder, and an entry pointing nowhere is only ever a
  // dead row in the manager.
  for (const auto& e : stored.profiles) {
    if (!seen.contains(e.id) && !e.implicit) {
      LOG_I() << "dropping profile registry entry with no directory:" << e.id;
    }
  }

  if (!out.last_used.isEmpty() && out.last_used != "classic" &&
      out.last_used != "portable" && !seen.contains(out.last_used)) {
    LOG_I() << "last used profile is gone, falling back:" << out.last_used;
    out.last_used.clear();
  }
  if (!out.startup_profile.isEmpty() && !seen.contains(out.startup_profile)) {
    out.startup_profile.clear();
    if (out.startup_policy == ProfileStartupPolicy::kFIXED) {
      // a fixed startup pointing at nothing would halt every launch
      out.startup_policy = ProfileStartupPolicy::kLAST_USED;
    }
  }

  return out;
}

auto LoadProfileRegistry(const QString& profiles_root,
                         const QString& classic_root,
                         const QString& portable_root) -> ProfileRegistryData {
  auto data = ReconcileProfileRegistry(ScanProfilesRoot(profiles_root),
                                       ReadRegistryFile(profiles_root));

  // Classic exists whether or not the registry has ever heard of it, so it is
  // synthesised rather than stored, and cannot be renamed or deleted.
  QList<ProfileRegistryEntry> implicit;
  {
    ProfileRegistryEntry e;
    e.id = "classic";
    e.root = classic_root;
    e.name = "Default";
    e.kind = ProfileRootKind::kCLASSIC;
    e.implicit = true;
    implicit.append(e);
  }
  if (!portable_root.isEmpty()) {
    ProfileRegistryEntry e;
    e.id = "portable";
    e.root = portable_root;
    e.name = "Portable";
    e.kind = ProfileRootKind::kPORTABLE;
    e.implicit = true;
    implicit.append(e);
  }

  data.profiles = implicit + data.profiles;
  return data;
}

auto SaveProfileRegistry(const QString& profiles_root,
                         const ProfileRegistryData& data) -> bool {
  if (!QDir(profiles_root).exists() && !QDir().mkpath(profiles_root)) {
    LOG_W() << "cannot create profiles root:" << profiles_root;
    return false;
  }

  QJsonObject obj;
  obj["schema_version"] = kRegistrySchemaVersion;
  obj["min_reader_version"] = data.min_reader_version;
  obj["last_used"] = data.last_used;
  obj["startup_policy"] = ProfileStartupPolicyToString(data.startup_policy);
  obj["startup_profile"] = data.startup_profile;
  obj["save_on_close"] = data.save_on_close;

  QJsonArray profiles;
  for (const auto& e : data.profiles) {
    // classic and portable are re-synthesised on every load; storing them would
    // let a stale root outlive the thing that computes it
    if (e.implicit) continue;
    profiles.append(EntryToJson(e));
  }
  obj["profiles"] = profiles;

  const auto payload = QJsonDocument(obj).toJson(QJsonDocument::Indented);

  QSaveFile file(RegistryPath(profiles_root));
  if (!file.open(QIODevice::WriteOnly)) {
    LOG_W() << "cannot write profile registry:" << RegistryPath(profiles_root);
    return false;
  }
  if (file.write(payload) != payload.size()) {
    file.cancelWriting();
    return false;
  }
  return file.commit();
}

auto UpdateProfileRegistry(
    const QString& profiles_root, const QString& classic_root,
    const QString& portable_root,
    const std::function<bool(ProfileRegistryData&)>& mutate) -> bool {
  if (!QDir(profiles_root).exists() && !QDir().mkpath(profiles_root)) {
    LOG_W() << "cannot create profiles root:" << profiles_root;
    return false;
  }

  QLockFile lock(RegistryLockPath(profiles_root));
  lock.setStaleLockTime(30000);
  if (!lock.tryLock(kRegistryLockTimeoutMs)) {
    LOG_W() << "could not take the profile registry lock";
    return false;
  }

  auto data = LoadProfileRegistry(profiles_root, classic_root, portable_root);
  if (!mutate(data)) return true;

  return SaveProfileRegistry(profiles_root, data);
}

auto CreateProfile(const QString& profiles_root, const QString& classic_root,
                   const QString& portable_root, const QString& id,
                   const QString& display_name, bool self_contained)
    -> ProfileCreateResult {
  ProfileCreateResult result;

  if (!IsValidProfileId(id)) {
    result.status = ProfileCreateStatus::kINVALID_ID;
    result.detail = id;
    return result;
  }

  const auto root = profiles_root + "/" + id;
  if (QFileInfo::exists(root)) {
    result.status = ProfileCreateStatus::kALREADY_EXISTS;
    result.detail = root;
    return result;
  }

  if (!QDir().mkpath(root)) {
    result.status = ProfileCreateStatus::kIO_FAILED;
    result.detail = root;
    return result;
  }

  // Enough of a marker for the scan to recognise the directory as a profile,
  // and for the policy to be in effect on the very first start. The key file is
  // deliberately absent: AppSecureKeyManager creates it lazily, so exactly one
  // code path is ever responsible for key material.
  ProfileMarker marker;
  marker.schema_version = GetAppProfileSchemaVersion();
  marker.min_reader_version = GetAppProfileSchemaVersion();
  marker.profile = GetAppProfileName();
  marker.last_writer_version = GetProjectVersion();
  marker.last_writer_stable = IsStableBuild();
  marker.profile_uuid =
      QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
  marker.profile_id = id;
  marker.display_name = display_name.isEmpty() ? id : display_name;
  marker.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  marker.created_by_version = GetProjectVersion();
  marker.kind = ProfileRootKindToString(ProfileRootKind::kNAMED);
  marker.self_contained = self_contained;

  if (!WriteProfileMarker(ProfileMarkerPathFor(root), marker)) {
    QDir(root).removeRecursively();
    result.status = ProfileCreateStatus::kIO_FAILED;
    result.detail = ProfileMarkerPathFor(root);
    return result;
  }

  QDir().mkpath(root + "/config");
  QDir().mkpath(root + "/workspace");

  result.entry.id = id;
  result.entry.root = root;
  result.entry.name = marker.display_name;
  result.entry.kind = ProfileRootKind::kNAMED;

  UpdateProfileRegistry(profiles_root, classic_root, portable_root,
                        [&result](ProfileRegistryData& data) {
                          data.profiles.append(result.entry);
                          return true;
                        });

  return result;
}

auto DeleteProfile(const QString& profiles_root, const QString& classic_root,
                   const QString& portable_root, const QString& id) -> bool {
  if (!IsValidProfileId(id)) return false;

  const auto root = profiles_root + "/" + id;
  if (QFileInfo::exists(root) && !QDir(root).removeRecursively()) {
    LOG_W() << "cannot remove profile directory:" << root;
    return false;
  }

  return UpdateProfileRegistry(
      profiles_root, classic_root, portable_root,
      [&id](ProfileRegistryData& data) {
        data.profiles.removeIf([&id](const ProfileRegistryEntry& e) {
          return e.id == id && !e.implicit;
        });
        if (data.last_used == id) data.last_used.clear();
        if (data.startup_profile == id) {
          data.startup_profile.clear();
          if (data.startup_policy == ProfileStartupPolicy::kFIXED) {
            data.startup_policy = ProfileStartupPolicy::kLAST_USED;
          }
        }
        return true;
      });
}

void TouchProfile(const QString& profiles_root, const QString& classic_root,
                  const QString& portable_root, const QString& id,
                  const QString& now_iso) {
  UpdateProfileRegistry(profiles_root, classic_root, portable_root,
                        [&](ProfileRegistryData& data) {
                          data.last_used = id;
                          for (auto& e : data.profiles) {
                            if (e.id == id) e.last_opened = now_iso;
                          }
                          return true;
                        });
}

}  // namespace GpgFrontend
