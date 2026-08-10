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

#include <optional>

#include "core/profile/Profile.h"
#include "core/profile/ProfileMarker.h"
#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend {

namespace {

/// Fill in what a root's own marker says about it, when it has one. The two
/// roots are synthesised rather than discovered, but they are still profiles
/// and still keep a marker, so their name and last-opened time come from the
/// same place as everybody else's.
auto ImplicitEntry(const QString& id, const QString& root,
                   const QString& fallback_name, ProfileKind kind)
    -> ProfileRegistryEntry {
  ProfileRegistryEntry e;
  e.id = id;
  e.root = root;
  e.name = fallback_name;
  e.kind = kind;
  e.implicit = true;

  if (const auto marker = ReadProfileMarker(ProfileMarkerPathFor(root))) {
    if (!marker->display_name.isEmpty()) e.name = marker->display_name;
    e.last_opened = marker->last_opened;
  }
  return e;
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
    e.kind = ProfileKind::kPERSIST;

    // The marker is the profile's own view of itself, and now the only one:
    // everything the list shows comes from here or from the directory name.
    if (const auto marker = ReadProfileMarker(ProfileMarkerPathFor(root))) {
      if (!marker->display_name.isEmpty()) e.name = marker->display_name;
      if (!marker->kind.isEmpty()) {
        e.kind = ProfileKindFromString(marker->kind);
      }
      e.last_opened = marker->last_opened;
      e.package_id = marker->package_id;
    }
    if (e.name.isEmpty()) e.name = name;

    out.append(e);
  }
  return out;
}

auto LoadProfileRegistry(const QString& profiles_root,
                         const QString& classic_root,
                         const QString& portable_root) -> ProfileRegistryData {
  ProfileRegistryData data;

  // The installed root exists whether or not the profiles directory does, so it
  // is synthesised rather than found, and cannot be renamed or deleted.
  data.profiles.append(ImplicitEntry("classic", classic_root, "Default",
                                     ProfileKind::kINSTALLED_ROOT));
  if (!portable_root.isEmpty()) {
    data.profiles.append(ImplicitEntry("portable", portable_root, "Portable",
                                       ProfileKind::kPORTABLE_ROOT));
  }

  data.profiles.append(ScanProfilesRoot(profiles_root));
  return data;
}

auto MintProfileDirectoryId(const QString& profiles_root) -> QString {
  // Six hex digits collide about once in 16.7M, so a handful of attempts is
  // already far more than the birthday bound needs for a realistic profile
  // count. Giving up empty-handed is better than returning a name that
  // CreateProfile would only reject as kALREADY_EXISTS.
  constexpr int kMaxAttempts = 16;
  constexpr int kIdLength = 6;

  for (int i = 0; i < kMaxAttempts; ++i) {
    const auto id = QUuid::createUuid()
                        .toString(QUuid::WithoutBraces)
                        .remove('-')
                        .left(kIdLength);

    if (!IsValidProfileId(id)) continue;
    if (QFileInfo::exists(profiles_root + "/" + id)) continue;

    return id;
  }

  LOG_W() << "could not mint an unused profile directory id after"
          << kMaxAttempts << "attempts";
  return {};
}

auto CreateProfile(const QString& profiles_root, const QString& id,
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
  // deliberately absent: ProfileSecureKeyManager creates it lazily, so exactly
  // one code path is ever responsible for key material.
  ProfileMarker marker;
  marker.schema_version = GetAppProfileSchemaVersion();
  // The floor, not this build's schema. A brand new profile is not inherently
  // unreadable to an older build just because a newer one made it, and saying
  // so here would lock a user out of a profile they just created the moment
  // they went back a version.
  marker.min_reader_version = GetAppProfileMinReaderSchema();
  marker.profile = GetAppProfileName();
  marker.last_writer_version = GetProjectVersion();
  marker.last_writer_stable = IsStableBuild();
  // The directory name is the identity: it is already unique, already opaque,
  // and one of the two is one too many to keep in step.
  marker.profile_uuid = id;
  marker.profile_id = id;
  marker.display_name = display_name.isEmpty() ? id : display_name;
  marker.created = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  marker.created_by_version = GetProjectVersion();
  marker.kind = ProfileKindToString(ProfileKind::kPERSIST);
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
  result.entry.kind = ProfileKind::kPERSIST;
  return result;
}

auto DeleteProfile(const QString& profiles_root, const QString& id) -> bool {
  if (!IsValidProfileId(id)) return false;

  const auto root = profiles_root + "/" + id;
  if (QFileInfo::exists(root) && !QDir(root).removeRecursively()) {
    LOG_W() << "cannot remove profile directory:" << root;
    return false;
  }

  // Nothing else to undo: the directory was the registration.
  return true;
}

}  // namespace GpgFrontend
