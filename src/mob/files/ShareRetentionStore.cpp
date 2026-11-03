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

#include "files/ShareRetentionStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

#include "files/StagingArea.h"

namespace GpgFrontend::Mobile {

namespace {

constexpr int kIndexVersion = 1;

auto MoveFile(const QString& from, const QString& to) -> bool {
  if (QFile::rename(from, to)) return true;
  // Another filesystem: copy, then remove the original.
  if (!QFile::copy(from, to)) return false;
  if (QFile::remove(from)) return true;
  // Two copies of what may be plaintext is worse than none moved.
  QFile::remove(to);
  return false;
}

}  // namespace

ShareRetentionStore::ShareRetentionStore(QString root, QString index_file,
                                         Policy policy, Revoke revoke)
    : root_(QDir::cleanPath(std::move(root))),
      index_file_(std::move(index_file)),
      policy_(policy),
      revoke_(std::move(revoke)) {
  load();
}

auto ShareRetentionStore::Root() const -> QString { return root_; }

auto ShareRetentionStore::Entries() const -> QList<RetainedFile> {
  return entries_;
}

auto ShareRetentionStore::Retain(const QString& staged, const QString& name,
                                 bool plaintext, const QDateTime& now)
    -> std::optional<RetainedFile> {
  RetainedFile entry;
  entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  const auto dir = root_ + QLatin1Char('/') + entry.id;
  if (!QDir().mkpath(dir)) return std::nullopt;

  entry.path = dir + QLatin1Char('/') + SafeFileName(name);
  if (!MoveFile(staged, entry.path)) {
    QDir(dir).removeRecursively();
    return std::nullopt;
  }

  entry.plaintext = plaintext;
  entry.handed_off = now.toUTC();
  entry.eligible = entry.handed_off.addSecs(policy_.keep.count());
  entry.size = QFileInfo(entry.path).size();
  entries_.append(entry);

  apply_caps(entry.id);
  save();
  return entry;
}

auto ShareRetentionStore::Withdraw(const QString& id, const QString& to)
    -> bool {
  for (auto it = entries_.begin(); it != entries_.end(); ++it) {
    if (it->id != id) continue;
    if (revoke_) revoke_(it->path);
    if (!inside_root(it->path) || !MoveFile(it->path, to)) return false;
    QDir(QFileInfo(it->path).absolutePath()).removeRecursively();
    entries_.erase(it);
    save();
    return true;
  }
  return false;
}

void ShareRetentionStore::Cleanup(const QDateTime& now) {
  const auto utc = now.toUTC();
  // Due later than one keep period from now means the clock was wrong when
  // it was shared (or the record was edited): it is due now, not in years.
  const auto latest = utc.addSecs(policy_.keep.count());
  QList<RetainedFile> kept;
  for (const auto& entry : entries_) {
    if (entry.eligible <= utc || entry.eligible > latest) {
      drop(entry);
    } else {
      kept.append(entry);
    }
  }
  entries_ = kept;
  apply_caps({});
  drop_orphans();
  save();
}

void ShareRetentionStore::Clear() {
  for (const auto& entry : entries_) drop(entry);
  entries_.clear();
  drop_orphans();
  save();
}

void ShareRetentionStore::drop(const RetainedFile& entry) {
  if (revoke_) revoke_(entry.path);
  if (inside_root(entry.path)) {
    QDir(QFileInfo(entry.path).absolutePath()).removeRecursively();
  }
}

void ShareRetentionStore::apply_caps(const QString& keep_id) {
  // Oldest first.
  std::stable_sort(entries_.begin(), entries_.end(),
                   [](const RetainedFile& a, const RetainedFile& b) {
                     return a.handed_off < b.handed_off;
                   });

  auto total = [this]() {
    qint64 bytes = 0;
    for (const auto& e : entries_) bytes += e.size;
    return bytes;
  };

  for (int i = 0; i < entries_.size() && (entries_.size() > policy_.max_files ||
                                          total() > policy_.max_bytes);) {
    if (entries_.at(i).id == keep_id) {
      ++i;
      continue;
    }
    drop(entries_.at(i));
    entries_.removeAt(i);
  }
}

void ShareRetentionStore::drop_orphans() {
  // Records whose file is gone.
  QList<RetainedFile> kept;
  for (const auto& entry : entries_) {
    if (QFileInfo::exists(entry.path)) {
      kept.append(entry);
    } else {
      drop(entry);
    }
  }
  entries_ = kept;

  // Files nobody recorded.
  QDir root(root_);
  for (const auto& dir :
       root.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
    const bool known =
        std::any_of(entries_.cbegin(), entries_.cend(),
                    [&dir](const RetainedFile& e) { return e.id == dir; });
    if (known) continue;
    const auto path = root.filePath(dir);
    for (const auto& file : QDir(path).entryList(QDir::Files | QDir::Hidden)) {
      if (revoke_) revoke_(path + QLatin1Char('/') + file);
    }
    QDir(path).removeRecursively();
  }
  for (const auto& file : root.entryList(QDir::Files | QDir::Hidden)) {
    QFile::remove(root.filePath(file));
  }
}

auto ShareRetentionStore::inside_root(const QString& path) const -> bool {
  const auto clean = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
  return clean.startsWith(root_ + QLatin1Char('/'));
}

void ShareRetentionStore::load() {
  entries_.clear();
  QFile file(index_file_);
  if (!file.open(QIODevice::ReadOnly)) return;

  const auto doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isObject() ||
      doc.object().value(QStringLiteral("version")).toInt() != kIndexVersion) {
    return;
  }

  for (const auto& value :
       doc.object().value(QStringLiteral("files")).toArray()) {
    const auto obj = value.toObject();
    RetainedFile entry;
    entry.id = obj.value(QStringLiteral("id")).toString();
    const auto name = obj.value(QStringLiteral("name")).toString();
    // Only a file of its own directory under the root, whatever the record
    // says.
    if (entry.id.isEmpty() || entry.id != SafeFileName(entry.id) ||
        name != SafeFileName(name)) {
      continue;
    }
    entry.path = root_ + QLatin1Char('/') + entry.id + QLatin1Char('/') + name;
    entry.plaintext = obj.value(QStringLiteral("plaintext")).toBool();
    entry.handed_off = QDateTime::fromString(
        obj.value(QStringLiteral("handed_off")).toString(), Qt::ISODate);
    entry.eligible = QDateTime::fromString(
        obj.value(QStringLiteral("eligible")).toString(), Qt::ISODate);
    entry.size = obj.value(QStringLiteral("size")).toInteger();
    // A record that cannot say when it may go, may go now.
    if (!entry.eligible.isValid())
      entry.eligible = QDateTime::fromSecsSinceEpoch(0);
    if (!entry.handed_off.isValid()) entry.handed_off = entry.eligible;
    entries_.append(entry);
  }
}

void ShareRetentionStore::save() const {
  QJsonArray files;
  for (const auto& entry : entries_) {
    files.append(QJsonObject{
        {QStringLiteral("id"), entry.id},
        {QStringLiteral("name"), QFileInfo(entry.path).fileName()},
        {QStringLiteral("plaintext"), entry.plaintext},
        {QStringLiteral("handed_off"),
         entry.handed_off.toString(Qt::ISODateWithMs)},
        {QStringLiteral("eligible"),
         entry.eligible.toString(Qt::ISODateWithMs)},
        {QStringLiteral("size"), entry.size},
    });
  }

  QDir().mkpath(QFileInfo(index_file_).absolutePath());
  QSaveFile file(index_file_);
  if (!file.open(QIODevice::WriteOnly)) return;
  file.write(
      QJsonDocument(QJsonObject{{QStringLiteral("version"), kIndexVersion},
                                {QStringLiteral("files"), files}})
          .toJson(QJsonDocument::Compact));
  file.commit();
}

}  // namespace GpgFrontend::Mobile
