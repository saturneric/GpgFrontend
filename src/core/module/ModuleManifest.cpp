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

#include "ModuleManifest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

namespace GpgFrontend::Module {

namespace {

/// Every refusal below returns through here, so none of them can forget to
/// clear `ok` or to say why.
auto Refuse(ModuleManifestStatus status, const QString& reason)
    -> ModuleManifestParseResult {
  ModuleManifestParseResult r;
  r.ok = false;
  r.status = status;
  r.reason = reason;
  return r;
}

auto Malformed(const QString& reason) -> ModuleManifestParseResult {
  return Refuse(ModuleManifestStatus::kMALFORMED, reason);
}

/// A required string, present and actually a string. An absent field and a
/// field holding the wrong type are reported apart, because they are different
/// mistakes for whoever has to fix the manifest.
auto TakeString(const QJsonObject& o, const QString& key, QString& out,
                QString& error) -> bool {
  const auto v = o.value(key);
  if (v.isUndefined()) {
    error = QString("\"%1\" is missing").arg(key);
    return false;
  }
  if (!v.isString()) {
    error = QString("\"%1\" is not a string").arg(key);
    return false;
  }
  out = v.toString();
  if (out.isEmpty()) {
    error = QString("\"%1\" is empty").arg(key);
    return false;
  }
  return true;
}

/// A required integer. QJsonValue::isDouble() is true for every number, so a
/// non-integral one is caught separately: a fractional `sdk_abi` is not a
/// version this build should quietly truncate into one.
auto TakeInt(const QJsonObject& o, const QString& key, int& out, QString& error)
    -> bool {
  const auto v = o.value(key);
  if (v.isUndefined()) {
    error = QString("\"%1\" is missing").arg(key);
    return false;
  }
  if (!v.isDouble()) {
    error = QString("\"%1\" is not a number").arg(key);
    return false;
  }
  const auto d = v.toDouble();
  if (d != static_cast<double>(static_cast<int>(d))) {
    error = QString("\"%1\" is not a whole number").arg(key);
    return false;
  }
  out = static_cast<int>(d);
  return true;
}

/// A required object.
auto TakeObject(const QJsonObject& o, const QString& key, QJsonObject& out,
                QString& error) -> bool {
  const auto v = o.value(key);
  if (v.isUndefined()) {
    error = QString("\"%1\" is missing").arg(key);
    return false;
  }
  if (!v.isObject()) {
    error = QString("\"%1\" is not an object").arg(key);
    return false;
  }
  out = v.toObject();
  return true;
}

auto IsHexDigest(const QString& s) -> bool {
  if (s.size() != 64) return false;
  for (const auto c : s) {
    const auto ch = c.unicode();
    const auto is_digit = ch >= u'0' && ch <= u'9';
    const auto is_lower_hex = ch >= u'a' && ch <= u'f';
    if (!is_digit && !is_lower_hex) return false;
  }
  return true;
}

}  // namespace

auto ParseModuleManifest(const QByteArray& bytes) -> ModuleManifestParseResult {
  QJsonParseError parse_error{};
  const auto doc = QJsonDocument::fromJson(bytes, &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    return Malformed(
        QString("it is not valid JSON: %1").arg(parse_error.errorString()));
  }
  if (!doc.isObject()) return Malformed("it is not a JSON object");

  const auto root = doc.object();
  QString error;

  // Version first, and on its own: a manifest from the future must be reported
  // as such rather than as a pile of missing fields, because the two call for
  // opposite things from whoever reads the message.
  int schema_version = 0;
  if (!TakeInt(root, "schema_version", schema_version, error)) {
    return Malformed(error);
  }
  if (schema_version > kModuleManifestSchemaVersion) {
    return Refuse(ModuleManifestStatus::kTOO_NEW,
                  QString("it uses manifest schema %1, and this version of "
                          "GpgFrontend understands up to %2")
                      .arg(schema_version)
                      .arg(kModuleManifestSchemaVersion));
  }
  if (schema_version < 1) {
    return Malformed(QString("\"schema_version\" is %1").arg(schema_version));
  }

  ModuleManifest m;
  m.schema_version = schema_version;

  if (!TakeString(root, "id", m.id, error)) return Malformed(error);
  if (!TakeString(root, "version", m.version, error)) return Malformed(error);
  if (!TakeInt(root, "sdk_abi", m.sdk_abi, error)) return Malformed(error);
  if (!TakeString(root, "min_host_version", m.min_host_version, error)) {
    return Malformed(error);
  }
  if (!TakeInt(root, "security_epoch", m.security_epoch, error)) {
    return Malformed(error);
  }
  if (m.sdk_abi < 0 || m.security_epoch < 0) {
    return Malformed("a version number in it is negative");
  }

  // capabilities: required, an array, and every element a string. An empty
  // array is legitimate -- a module that asks for nothing.
  {
    const auto v = root.value("capabilities");
    if (v.isUndefined()) return Malformed("\"capabilities\" is missing");
    if (!v.isArray()) return Malformed("\"capabilities\" is not an array");
    for (const auto& c : v.toArray()) {
      if (!c.isString()) {
        return Malformed("a value in \"capabilities\" is not a string");
      }
      m.capabilities.append(c.toString());
    }
  }

  // metadata: required object, every value a string. This is what the module
  // used to answer by being loaded and asked -- which meant the only way to
  // learn what a module claimed to be was to run its initialisers first.
  {
    QJsonObject meta;
    if (!TakeObject(root, "metadata", meta, error)) return Malformed(error);
    for (auto it = meta.constBegin(); it != meta.constEnd(); ++it) {
      if (!it.value().isString()) {
        return Malformed(
            QString("\"metadata.%1\" is not a string").arg(it.key()));
      }
      m.metadata.insert(it.key(), it.value().toString());
    }
  }

  {
    QJsonObject build;
    if (!TakeObject(root, "build", build, error)) return Malformed(error);
    if (!TakeString(build, "id", m.build_id, error) ||
        !TakeString(build, "timestamp", m.build_timestamp, error) ||
        !TakeString(build, "source_commit", m.build_source_commit, error)) {
      return Malformed(QString("build.%1").arg(error));
    }
  }

  {
    QJsonObject platform;
    if (!TakeObject(root, "platform", platform, error)) return Malformed(error);
    if (!TakeString(platform, "os", m.platform_os, error) ||
        !TakeString(platform, "arch", m.platform_arch, error) ||
        !TakeString(platform, "qt", m.platform_qt, error)) {
      return Malformed(QString("platform.%1").arg(error));
    }
  }

  // files: required, non-empty, and every entry a path and a digest. A package
  // whose manifest covers nothing would verify trivially, which is the one
  // outcome this whole format exists to prevent.
  {
    const auto v = root.value("files");
    if (v.isUndefined()) return Malformed("\"files\" is missing");
    if (!v.isArray()) return Malformed("\"files\" is not an array");
    const auto files = v.toArray();
    if (files.isEmpty()) return Malformed("\"files\" is empty");

    for (const auto& f : files) {
      if (!f.isObject())
        return Malformed("a value in \"files\" is not an object");
      const auto fo = f.toObject();
      ModuleManifestFile entry;
      if (!TakeString(fo, "path", entry.path, error) ||
          !TakeString(fo, "sha256", entry.sha256, error)) {
        return Malformed(QString("files.%1").arg(error));
      }
      if (!IsHexDigest(entry.sha256)) {
        return Malformed(QString("the digest of \"%1\" is not 64 lower-case "
                                 "hexadecimal characters")
                             .arg(entry.path));
      }
      m.files.append(entry);
    }
  }

  ModuleManifestParseResult result;
  result.ok = true;
  result.status = ModuleManifestStatus::kOK;
  result.manifest = m;
  return result;
}

}  // namespace GpgFrontend::Module
