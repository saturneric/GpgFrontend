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

#include "screenshot/ScreenshotEnv.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <cmath>

namespace GpgFrontend::Screenshot {

namespace {

constexpr int kRequiredDpr = 2;
constexpr int kRequiredLogicalDpi = 96;

auto Pick(const QJsonObject& from, const QStringList& keys) -> QJsonObject {
  QJsonObject out;
  for (const auto& key : keys) out.insert(key, from.value(key));
  return out;
}

}  // namespace

auto CanonicalJson(const QJsonObject& object) -> QByteArray {
  return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

auto EnvHashSubset(const QJsonObject& env) -> QJsonObject {
  auto subset = Pick(
      env, {"qt_version", "platform_name", "style", "color_scheme", "locale",
            "clock", "font_requested", "font_resolved", "system_font",
            "gnupg_version", "gpgme_version", "gpg_clock", "tz_offset_check"});
  subset.insert("os", Pick(env.value("os").toObject(), {"product", "build"}));
  subset.insert("screen", Pick(env.value("screen").toObject(),
                               {"size", "dpr", "logical_dpi", "physical_dpi"}));
  return subset;
}

auto EnvHashCore(const QJsonObject& env) -> QString {
  return QString::fromLatin1(
      QCryptographicHash::hash(CanonicalJson(EnvHashSubset(env)),
                               QCryptographicHash::Sha256)
          .toHex());
}

auto ParseCheckOptions(const QString& output, const QString& component)
    -> std::optional<bool> {
  for (const auto& line : output.split(QLatin1Char('\n'))) {
    const auto fields = line.trimmed().split(QLatin1Char(':'));
    if (fields.size() < 5 || fields.at(0) != component) continue;
    if (fields.at(4) == QLatin1String("1")) return true;
    if (fields.at(4) == QLatin1String("0")) return false;
    return std::nullopt;
  }
  return std::nullopt;
}

auto ParseDemoData(const QByteArray& json, QString* error)
    -> std::optional<DemoData> {
  QJsonParseError parse_error{};
  const auto doc = QJsonDocument::fromJson(json, &parse_error);
  if (doc.isNull() || !doc.isObject()) {
    if (error != nullptr) *error = parse_error.errorString();
    return std::nullopt;
  }

  const auto root = doc.object();
  if (root.value("schema").toInt() != 1) {
    if (error != nullptr) *error = QStringLiteral("unsupported schema");
    return std::nullopt;
  }

  const auto parse_keys = [error](const QJsonArray& array,
                                  QList<DemoKey>* keys) -> bool {
    for (const auto& value : array) {
      const auto o = value.toObject();
      DemoKey key;
      key.name = o.value("name").toString();
      key.fingerprint = o.value("fingerprint").toString();
      key.created = o.value("created").toInteger();
      if (!o.value("expires").isNull()) {
        key.expires = o.value("expires").toInteger();
      }
      key.revoked = o.value("revoked").toBool();
      if (key.fingerprint.isEmpty()) {
        if (error != nullptr)
          *error = QStringLiteral("key without fingerprint");
        return false;
      }
      keys->append(key);
    }
    return true;
  };

  DemoData data;
  data.newest_timestamp = root.value("newest_timestamp").toInteger(-1);
  if (!parse_keys(root.value("keys").toArray(), &data.keys)) {
    return std::nullopt;
  }

  const auto databases = root.value("databases").toObject();
  for (auto it = databases.constBegin(); it != databases.constEnd(); ++it) {
    const auto o = it.value().toObject();
    DemoDatabase db;
    db.id = it.key();
    db.name = o.value("name").toString();
    db.backend = o.value("backend").toString();
    for (const auto& file : o.value("import").toArray()) {
      db.imports.append(file.toString());
    }
    if (db.name.isEmpty() ||
        (db.backend != QLatin1String("gnupg") &&
         db.backend != QLatin1String("rpgp")) ||
        !parse_keys(o.value("keys").toArray(), &db.keys)) {
      if (error != nullptr && error->isEmpty()) {
        *error = QStringLiteral("invalid database: ") + db.id;
      }
      return std::nullopt;
    }
    data.databases.append(db);
  }

  if (data.keys.isEmpty() || data.newest_timestamp < 0) {
    if (error != nullptr)
      *error = QStringLiteral("no keys or no newest_timestamp");
    return std::nullopt;
  }
  return data;
}

auto DemoData::Database(const QString& id) const -> const DemoDatabase* {
  for (const auto& db : databases) {
    if (db.id == id) return &db;
  }
  return nullptr;
}

auto ExpectedKeyStates(const QList<DemoKey>& keys, const QDateTime& clock)
    -> QList<ExpectedKeyState> {
  const auto now = clock.toSecsSinceEpoch();
  QList<ExpectedKeyState> states;
  for (const auto& key : keys) {
    states.append({key.name, key.fingerprint,
                   key.expires.has_value() && now >= *key.expires,
                   key.revoked});
  }
  return states;
}

auto EvaluateGates(const GateInputs& inputs) -> QStringList {
  QStringList failures;
  if (std::lround(inputs.dpr * 100) != kRequiredDpr * 100) {
    failures.append(QStringLiteral("dpr"));
  }
  if (inputs.logical_dpi != kRequiredLogicalDpi) {
    failures.append(QStringLiteral("logical-dpi"));
  }
  if (inputs.tz_offset_seconds != 0)
    failures.append(QStringLiteral("timezone"));

  for (qsizetype i = 0; i < inputs.requested_families.size(); ++i) {
    const auto& requested = inputs.requested_families.at(i);
    const auto resolved = i < inputs.resolved_families.size()
                              ? inputs.resolved_families.at(i)
                              : QString();
    if (resolved.compare(requested, Qt::CaseInsensitive) != 0) {
      failures.append(QStringLiteral("font:") + requested);
    }
  }
  if (inputs.requested_families.isEmpty()) {
    failures.append(QStringLiteral("font:none-requested"));
  }
  return failures;
}

auto SizeGateFailure(const QSize& requested, const QSize& actual)
    -> std::optional<QString> {
  if (requested == actual) return std::nullopt;
  return QStringLiteral("size:%1x%2!=%3x%4")
      .arg(actual.width())
      .arg(actual.height())
      .arg(requested.width())
      .arg(requested.height());
}

}  // namespace GpgFrontend::Screenshot
