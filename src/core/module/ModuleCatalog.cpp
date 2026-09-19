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

#include "ModuleCatalog.h"

#include <sodium.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Module {

namespace {

auto Refuse(ModuleCatalogStatus status, const QString& reason)
    -> ModuleCatalogVerification {
  ModuleCatalogVerification r;
  r.ok = false;
  r.status = status;
  r.reason = reason;
  return r;
}

auto Malformed(const QString& reason) -> ModuleCatalogVerification {
  return Refuse(ModuleCatalogStatus::kMALFORMED, reason);
}

/// Strict, exactly as the manifest is: a field of the wrong type is a refusal
/// rather than a default, because a catalog that quietly reads as "epoch 0,
/// not revoked" is worse than no catalog at all.
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

auto TakeBool(const QJsonObject& o, const QString& key, bool& out,
              QString& error) -> bool {
  const auto v = o.value(key);
  if (v.isUndefined()) {
    error = QString("\"%1\" is missing").arg(key);
    return false;
  }
  if (!v.isBool()) {
    error = QString("\"%1\" is not true or false").arg(key);
    return false;
  }
  out = v.toBool();
  return true;
}

auto IsHex(const QString& s, int length) -> bool {
  if (s.size() != length) return false;
  for (const auto c : s) {
    const auto ch = c.unicode();
    const auto ok = (ch >= u'0' && ch <= u'9') || (ch >= u'a' && ch <= u'f');
    if (!ok) return false;
  }
  return true;
}

}  // namespace

auto ModuleCatalogStatusToString(ModuleCatalogStatus s) -> const char* {
  switch (s) {
    case ModuleCatalogStatus::kOK:
      return "ok";
    case ModuleCatalogStatus::kMALFORMED:
      return "malformed";
    case ModuleCatalogStatus::kTOO_NEW:
      return "written by a newer version";
    case ModuleCatalogStatus::kBAD_SIGNATURE:
      return "it was not signed by the expected key";
  }
  return "unknown";
}

auto ModuleCatalog::FindByDigest(const QString& package_sha256) const
    -> std::optional<ModuleCatalogEntry> {
  for (const auto& entry : entries) {
    if (entry.package_sha256 == package_sha256) return entry;
  }
  return std::nullopt;
}

auto ModuleCatalog::EntriesFor(const QString& module_id) const
    -> QVector<ModuleCatalogEntry> {
  QVector<ModuleCatalogEntry> found;
  for (const auto& entry : entries) {
    if (entry.module_id == module_id) found.append(entry);
  }
  return found;
}

auto VerifyModuleCatalog(const QByteArray& catalog_bytes,
                         const QByteArray& signature,
                         const QByteArray& root_public_key)
    -> ModuleCatalogVerification {
  if (!EnsureSodiumInit()) {
    return Malformed("the cryptography library could not be started");
  }

  // A catalog with no key to check it against is not a catalog, it is a file.
  // Refusing here rather than parsing anyway is what stops this being usable
  // as an unauthenticated source of policy.
  if (root_public_key.size() != crypto_sign_PUBLICKEYBYTES) {
    return Refuse(ModuleCatalogStatus::kBAD_SIGNATURE,
                  "no usable root key was given");
  }
  if (signature.size() != crypto_sign_BYTES) {
    return Refuse(ModuleCatalogStatus::kBAD_SIGNATURE,
                  "its signature is the wrong size");
  }

  // Over the bytes as supplied, and before they are parsed.
  if (crypto_sign_verify_detached(
          reinterpret_cast<const unsigned char*>(signature.constData()),
          reinterpret_cast<const unsigned char*>(catalog_bytes.constData()),
          static_cast<unsigned long long>(catalog_bytes.size()),
          reinterpret_cast<const unsigned char*>(
              root_public_key.constData())) != 0) {
    return Refuse(ModuleCatalogStatus::kBAD_SIGNATURE,
                  "it was not signed by this root key");
  }

  QJsonParseError parse_error{};
  const auto doc = QJsonDocument::fromJson(catalog_bytes, &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    return Malformed(
        QString("it is not valid JSON: %1").arg(parse_error.errorString()));
  }
  if (!doc.isObject()) return Malformed("it is not a JSON object");

  const auto root = doc.object();
  QString error;

  int schema_version = 0;
  if (!TakeInt(root, "schema_version", schema_version, error)) {
    return Malformed(error);
  }
  if (schema_version > kModuleCatalogSchemaVersion) {
    return Refuse(ModuleCatalogStatus::kTOO_NEW,
                  QString("it uses catalog schema %1, and this version of "
                          "GpgFrontend understands up to %2")
                      .arg(schema_version)
                      .arg(kModuleCatalogSchemaVersion));
  }
  if (schema_version < 1) {
    return Malformed(QString("\"schema_version\" is %1").arg(schema_version));
  }

  ModuleCatalog catalog;
  catalog.schema_version = schema_version;
  if (!TakeString(root, "issued_at", catalog.issued_at, error)) {
    return Malformed(error);
  }

  const auto entries_value = root.value("entries");
  if (entries_value.isUndefined()) return Malformed("\"entries\" is missing");
  if (!entries_value.isArray()) return Malformed("\"entries\" is not an array");

  for (const auto& value : entries_value.toArray()) {
    if (!value.isObject()) {
      return Malformed("a value in \"entries\" is not an object");
    }
    const auto o = value.toObject();

    ModuleCatalogEntry entry;
    if (!TakeString(o, "module_id", entry.module_id, error) ||
        !TakeString(o, "version", entry.version, error) ||
        !TakeString(o, "package_sha256", entry.package_sha256, error) ||
        !TakeString(o, "build_public_key", entry.build_public_key, error) ||
        !TakeInt(o, "security_epoch", entry.security_epoch, error) ||
        !TakeBool(o, "revoked", entry.revoked, error)) {
      return Malformed(QString("entries.%1").arg(error));
    }

    if (!IsHex(entry.package_sha256, 64)) {
      return Malformed(QString("the package digest of \"%1\" is not 64 "
                               "lower-case hexadecimal characters")
                           .arg(entry.module_id));
    }
    if (!IsHex(entry.build_public_key, crypto_sign_PUBLICKEYBYTES * 2)) {
      return Malformed(QString("the build key of \"%1\" is not %2 lower-case "
                               "hexadecimal characters")
                           .arg(entry.module_id)
                           .arg(crypto_sign_PUBLICKEYBYTES * 2));
    }
    if (entry.security_epoch < 0) {
      return Malformed(QString("the security epoch of \"%1\" is negative")
                           .arg(entry.module_id));
    }

    // Two entries for one package would be a catalog saying two things about
    // the same bytes -- exactly the split view the package format refuses
    // inside an archive, arriving one level up.
    if (catalog.FindByDigest(entry.package_sha256).has_value()) {
      return Malformed(QString("it lists the package %1 more than once")
                           .arg(entry.package_sha256));
    }

    catalog.entries.append(entry);
  }

  ModuleCatalogVerification result;
  result.ok = true;
  result.status = ModuleCatalogStatus::kOK;
  result.catalog = catalog;
  return result;
}

}  // namespace GpgFrontend::Module
