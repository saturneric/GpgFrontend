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

#include "files/KeptStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QUuid>

#include "files/StagingArea.h"

namespace GpgFrontend::Mobile {

namespace {

constexpr auto kIndexName = "index.json";
constexpr int kIndexVersion = 1;

/// @p path under @p root, or empty when it would leave it.
auto Inside(const QString& root, const QString& path) -> QString {
  if (path.isEmpty() || QDir::isAbsolutePath(path)) return {};
  const auto full = QDir::cleanPath(root + QLatin1Char('/') + path);
  if (!full.startsWith(QDir::cleanPath(root) + QLatin1Char('/'))) return {};
  return full;
}

auto ToJson(const KeptRecord& record) -> QJsonObject {
  return {
      {QStringLiteral("path"), record.path},
      {QStringLiteral("name"), record.name},
      {QStringLiteral("origin"), record.origin},
      {QStringLiteral("operation"), record.operation},
      {QStringLiteral("made_by"), record.made_by},
      {QStringLiteral("plaintext"), record.plaintext},
      {QStringLiteral("saved_name"), record.saved_name},
      {QStringLiteral("added"), record.added.toString(Qt::ISODateWithMs)},
      {QStringLiteral("result"), QJsonObject::fromVariantMap(record.result)},
  };
}

auto FromJson(const QJsonObject& object) -> KeptRecord {
  KeptRecord record;
  record.path = object.value(QStringLiteral("path")).toString();
  record.name = object.value(QStringLiteral("name")).toString();
  record.origin = object.value(QStringLiteral("origin")).toString();
  record.operation = object.value(QStringLiteral("operation")).toString();
  record.made_by = object.value(QStringLiteral("made_by")).toString();
  record.plaintext = object.value(QStringLiteral("plaintext")).toBool();
  record.saved_name = object.value(QStringLiteral("saved_name")).toString();
  record.added = QDateTime::fromString(
      object.value(QStringLiteral("added")).toString(), Qt::ISODateWithMs);
  record.result =
      object.value(QStringLiteral("result")).toObject().toVariantMap();
  return record;
}

}  // namespace

KeptStore::KeptStore(QString root) : root_(QDir::cleanPath(root)) {}

auto KeptStore::Root() const -> QString { return root_; }

auto KeptStore::NewPath(const QString& name) -> QString {
  const auto dir = root_ + QLatin1Char('/') +
                   QUuid::createUuid().toString(QUuid::Id128).left(16);
  QDir().mkpath(dir);
  return dir + QLatin1Char('/') + SafeFileName(name);
}

auto KeptStore::Adopt(const QString& from, const QString& name) -> QString {
  if (from.isEmpty() || !QFileInfo::exists(from)) return {};
  const auto to = NewPath(name);
  // Both are in the application's own storage, usually one filesystem.
  if (QFile::rename(from, to)) {
    QDir().rmdir(QFileInfo(from).absolutePath());
    return to;
  }
  if (QFile::copy(from, to)) {
    QFile::remove(from);
    QDir().rmdir(QFileInfo(from).absolutePath());
    return to;
  }
  QFile::remove(to);
  QDir().rmdir(QFileInfo(to).absolutePath());
  return {};
}

auto KeptStore::Load() -> QList<KeptRecord> {
  QList<KeptRecord> records;
  QSet<QString> listed;

  QFile index(root_ + QLatin1Char('/') + QLatin1String(kIndexName));
  if (index.open(QIODevice::ReadOnly)) {
    const auto object = QJsonDocument::fromJson(index.readAll()).object();
    for (const auto& value : object.value(QStringLiteral("files")).toArray()) {
      auto record = FromJson(value.toObject());
      const auto full = Inside(root_, record.path);
      // Its file gone (or a path that would leave the store): forgotten.
      if (full.isEmpty() || record.name.isEmpty() ||
          !QFileInfo(full).isFile()) {
        continue;
      }
      listed.insert(QFileInfo(full).absolutePath());
      records << record;
    }
  }

  // What the index does not name is left from a run that ended before it
  // wrote the index: nobody's.
  const QDir root(root_);
  for (const auto& entry :
       root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
    if (!listed.contains(entry.absoluteFilePath())) {
      QDir(entry.absoluteFilePath()).removeRecursively();
    }
  }
  return records;
}

auto KeptStore::Save(const QList<KeptRecord>& records) -> bool {
  if (!QDir().mkpath(root_)) return false;
  QJsonArray files;
  for (const auto& record : records) files.append(ToJson(record));
  const QJsonObject object{{QStringLiteral("version"), kIndexVersion},
                           {QStringLiteral("files"), files}};
  QSaveFile index(root_ + QLatin1Char('/') + QLatin1String(kIndexName));
  if (!index.open(QIODevice::WriteOnly)) return false;
  index.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
  return index.commit();
}

void KeptStore::Clear() { QDir(root_).removeRecursively(); }

}  // namespace GpgFrontend::Mobile
