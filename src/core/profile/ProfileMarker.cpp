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

#include "core/profile/ProfileMarker.h"

#include <optional>

namespace GpgFrontend {

auto ProfileMarkerPathFor(const QString& profile_root) -> QString {
  return profile_root + "/profile.json";
}

auto CheckProfileCompatibility(const ProfileMarker& marker, bool marker_present,
                               int this_schema_version)
    -> ProfileCompatibility {
  if (!marker_present) return ProfileCompatibility::kMISSING;

  if (marker.min_reader_version > this_schema_version) {
    return ProfileCompatibility::kTOO_NEW;
  }

  return ProfileCompatibility::kOK;
}

auto ReadProfileMarker(const QString& path) -> std::optional<ProfileMarker> {
  QFile file(path);
  if (!file.exists()) return {};

  if (!file.open(QIODevice::ReadOnly)) {
    LOG_W() << "cannot open profile marker:" << path;
    return {};
  }

  QJsonParseError error{};
  const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject()) {
    LOG_W() << "profile marker is not valid json:" << path
            << error.errorString();
    return {};
  }

  const auto obj = doc.object();

  ProfileMarker marker;
  marker.schema_version = obj.value("schema_version").toInt();
  marker.min_reader_version = obj.value("min_reader_version").toInt();
  marker.profile = obj.value("profile").toString();
  marker.last_writer_version = obj.value("last_writer_version").toString();
  marker.last_writer_stable = obj.value("last_writer_stable").toBool();

  marker.profile_uuid = obj.value("profile_uuid").toString();
  marker.profile_id = obj.value("profile_id").toString();
  marker.display_name = obj.value("display_name").toString();
  marker.created = obj.value("created").toString();
  marker.created_by_version = obj.value("created_by_version").toString();
  marker.kind = obj.value("kind").toString();
  marker.last_opened = obj.value("last_opened").toString();
  marker.package_id = obj.value("package_id").toString();
  marker.credential_account = obj.value("credential_account").toString();

  marker.self_contained =
      obj.value("policy").toObject().value("self_contained").toBool();

  // Absent keys must stay absent rather than becoming default-constructed
  // QVariants: every consumer reads a missing key as "this layer has no
  // opinion", and a value of the wrong emptiness would stop the ladder.
  marker.deployment = obj.value("deployment").toObject().toVariantMap();

  for (const auto& entry : obj.value("migrations").toArray()) {
    const auto e = entry.toObject();
    ProfileMigrationRecord record;
    record.from = e.value("from").toInt();
    record.to = e.value("to").toInt();
    record.name = e.value("name").toString();
    record.at = e.value("at").toString();
    record.by = e.value("by").toString();
    record.skipped = e.value("skipped").toBool();
    record.reason = e.value("reason").toString();
    marker.migrations.append(record);
  }

  // Anything this build does not know is carried through untouched, so opening
  // a profile with an older version never silently discards what a newer one
  // depends on.
  static const QSet<QString> kKnown = {"schema_version",
                                       "min_reader_version",
                                       "profile",
                                       "last_writer_version",
                                       "last_writer_stable",
                                       "profile_uuid",
                                       "profile_id",
                                       "display_name",
                                       "created",
                                       "created_by_version",
                                       "kind",
                                       "last_opened",
                                       "package_id",
                                       "credential_account",
                                       "policy",
                                       "deployment",
                                       "migrations"};
  for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
    if (!kKnown.contains(it.key()))
      marker.unknown_fields[it.key()] = it.value();
  }

  return marker;
}

auto WriteProfileMarker(const QString& path, const ProfileMarker& marker)
    -> bool {
  // unknown keys first, so a known field always wins a collision rather than
  // being shadowed by a stale copy of itself
  QJsonObject obj = marker.unknown_fields;

  obj["schema_version"] = marker.schema_version;
  obj["min_reader_version"] = marker.min_reader_version;
  obj["profile"] = marker.profile;
  obj["last_writer_version"] = marker.last_writer_version;
  obj["last_writer_stable"] = marker.last_writer_stable;

  const auto put = [&obj](const char* key, const QString& value) {
    if (!value.isEmpty()) obj[QLatin1String(key)] = value;
  };
  put("profile_uuid", marker.profile_uuid);
  put("profile_id", marker.profile_id);
  put("display_name", marker.display_name);
  put("created", marker.created);
  put("created_by_version", marker.created_by_version);
  put("kind", marker.kind);
  put("last_opened", marker.last_opened);
  put("package_id", marker.package_id);
  put("credential_account", marker.credential_account);

  QJsonObject policy;
  policy["self_contained"] = marker.self_contained;
  obj["policy"] = policy;

  // Written only when there is something to write, so a profile that pins
  // nothing keeps a marker with no empty scaffolding in it — and so the absent
  // case round-trips to an absent case rather than to an empty object.
  if (!marker.deployment.isEmpty()) {
    obj["deployment"] = QJsonObject::fromVariantMap(marker.deployment);
  }

  if (!marker.migrations.isEmpty()) {
    QJsonArray migrations;
    for (const auto& record : marker.migrations) {
      QJsonObject e;
      e["from"] = record.from;
      e["to"] = record.to;
      e["name"] = record.name;
      e["at"] = record.at;
      e["by"] = record.by;
      if (record.skipped) {
        e["skipped"] = true;
        e["reason"] = record.reason;
      }
      migrations.append(e);
    }
    obj["migrations"] = migrations;
  }

  const auto payload = QJsonDocument(obj).toJson(QJsonDocument::Indented);

  // QSaveFile rather than a truncating write: this file is the only record of
  // how far a migration got, and a half-written one after a power cut would
  // make the ladder restart against already-migrated data.
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    LOG_W() << "cannot write profile marker:" << path;
    return false;
  }
  if (file.write(payload) != payload.size()) {
    file.cancelWriting();
    LOG_W() << "short write on profile marker:" << path;
    return false;
  }
  return file.commit();
}

}  // namespace GpgFrontend
