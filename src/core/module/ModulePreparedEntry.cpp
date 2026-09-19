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

#include "ModulePreparedEntry.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <cmath>

namespace GpgFrontend::Module {

namespace {

auto ModeFromKey(const QString& key)
    -> std::optional<ModuleEntryVerificationMode> {
  for (const auto mode : {ModuleEntryVerificationMode::kFILE_SHA256,
                          ModuleEntryVerificationMode::kPE_AUTHENTICODE_SHA256,
                          ModuleEntryVerificationMode::kAPPLE_BINDING_ID}) {
    if (ModuleEntryVerificationModeKey(mode) == key) return mode;
  }
  return std::nullopt;
}

/// A required string field, present and actually a string.
auto RequiredString(const QJsonObject& object, const QString& key, QString& out,
                    QString& reason) -> bool {
  const auto value = object.value(key);
  if (value.isUndefined()) {
    reason = QString("it has no \"%1\"").arg(key);
    return false;
  }
  if (!value.isString() || value.toString().isEmpty()) {
    reason = QString("its \"%1\" is not a non-empty string").arg(key);
    return false;
  }
  out = value.toString();
  return true;
}

}  // namespace

auto WritePreparedEntrySeal(const QString& path, const PreparedEntrySeal& seal,
                            QString& reason) -> bool {
  if (!IsModuleHexDigest(seal.value)) {
    reason = "the sealed value is not a 256-bit hex digest";
    return false;
  }

  QJsonObject object;
  object.insert("schema", kPreparedEntrySealSchema);
  object.insert("module_id", seal.module_id);
  object.insert("build_id", seal.build_id);
  object.insert("entry_native_name", seal.entry_native_name);
  object.insert("mode", ModuleEntryVerificationModeKey(seal.mode));
  object.insert("value", seal.value);
  if (seal.size >= 0) object.insert("size", seal.size);

  QFile file(path);
  // Truncating rather than appending, and rewriting rather than skipping an
  // existing file: a stale seal that survives is the one failure this whole
  // mechanism exists to notice.
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    reason = QString("it could not be written: %1").arg(file.errorString());
    return false;
  }
  const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
  if (file.write(bytes) != bytes.size()) {
    reason = QString("it was not fully written: %1").arg(file.errorString());
    file.close();
    return false;
  }
  file.close();
  return true;
}

auto ReadPreparedEntrySeal(const QString& path, PreparedEntrySeal& seal,
                           QString& reason) -> bool {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    reason = QString("it could not be read: %1").arg(file.errorString());
    return false;
  }
  const auto bytes = file.readAll();
  file.close();

  QJsonParseError parse_error{};
  const auto doc = QJsonDocument::fromJson(bytes, &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    reason = QString("it is not valid JSON: %1").arg(parse_error.errorString());
    return false;
  }
  if (!doc.isObject()) {
    reason = "it is not a JSON object";
    return false;
  }
  const auto object = doc.object();

  const auto schema = object.value("schema");
  if (!schema.isDouble() || schema.toInt() != kPreparedEntrySealSchema) {
    reason = QString("its schema is not %1").arg(kPreparedEntrySealSchema);
    return false;
  }

  PreparedEntrySeal read;
  QString mode_key;
  if (!RequiredString(object, "module_id", read.module_id, reason) ||
      !RequiredString(object, "build_id", read.build_id, reason) ||
      !RequiredString(object, "entry_native_name", read.entry_native_name,
                      reason) ||
      !RequiredString(object, "mode", mode_key, reason) ||
      !RequiredString(object, "value", read.value, reason)) {
    return false;
  }

  const auto mode = ModeFromKey(mode_key);
  if (!mode) {
    reason = QString("its mode \"%1\" is not one this build knows")
                 .arg(mode_key);
    return false;
  }
  read.mode = *mode;

  if (!IsModuleHexDigest(read.value)) {
    reason = "its value is not a 256-bit lower-case hex digest";
    return false;
  }

  const auto size = object.value("size");
  if (!size.isUndefined()) {
    // Same rule as the manifest's, for the same reason: Windows and macOS
    // signing both change file size, so a size under either mode is an
    // invariant that legitimately breaks.
    if (read.mode != ModuleEntryVerificationMode::kFILE_SHA256) {
      reason = QString("it carries a size under \"%1\", which may not")
                   .arg(mode_key);
      return false;
    }
    if (!size.isDouble() || size.toDouble() < 0 ||
        size.toDouble() != std::floor(size.toDouble())) {
      reason = "its size is not a non-negative whole number";
      return false;
    }
    read.size = static_cast<qint64>(size.toDouble());
  }

  seal = read;
  return true;
}

}  // namespace GpgFrontend::Module
