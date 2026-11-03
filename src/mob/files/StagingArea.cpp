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

#include "files/StagingArea.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

namespace GpgFrontend::Mobile {

namespace {

constexpr auto kLockName = "session.lock";

}  // namespace

StagingArea::StagingArea(const QString& root)
    : dir_(QDir::cleanPath(root) + QLatin1Char('/') +
           QUuid::createUuid().toString(QUuid::WithoutBraces)) {
  QDir().mkpath(dir_);
  lock_ = std::make_unique<QLockFile>(dir_ + QLatin1Char('/') +
                                      QLatin1String(kLockName));
  // A lock that never goes stale: a session lives as long as its process.
  lock_->setStaleLockTime(0);
  lock_->tryLock(0);
}

StagingArea::~StagingArea() {
  lock_->unlock();
  QDir(dir_).removeRecursively();
}

auto StagingArea::Directory() const -> QString { return dir_; }

auto LocalOrContent(const QUrl& url) -> QString {
  return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

auto SafeFileName(const QString& name) -> QString {
  auto safe = QFileInfo(name).fileName();
  safe.remove(QLatin1Char('/'));
  safe.remove(QLatin1Char('\\'));
  if (safe.isEmpty() || safe == QStringLiteral(".") ||
      safe == QStringLiteral("..")) {
    return QStringLiteral("file");
  }
  return safe;
}

auto StagingArea::NewPath(const QString& name) -> QString {
  const auto safe = SafeFileName(name);
  // A directory per file keeps its own name, which a share sheet shows.
  const auto dir = dir_ + QLatin1Char('/') + QString::number(next_++);
  QDir().mkpath(dir);
  return dir + QLatin1Char('/') + safe;
}

void StagingArea::RemoveOrphans(const QString& root) {
  QDir base(root);
  for (const auto& session :
       base.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
    const auto path = base.filePath(session);
    QLockFile lock(path + QLatin1Char('/') + QLatin1String(kLockName));
    lock.setStaleLockTime(0);
    // Taken: no live process holds the session (a dead one's lock is
    // reclaimed). Held by this process or another: left alone.
    if (!lock.tryLock(0)) continue;
    lock.unlock();
    QDir(path).removeRecursively();
  }
}

StagedFile::StagedFile(QString path) : path_(std::move(path)) {}

StagedFile::~StagedFile() { Reset(); }

StagedFile::StagedFile(StagedFile&& other) noexcept : path_(other.Release()) {}

auto StagedFile::operator=(StagedFile&& other) noexcept -> StagedFile& {
  if (this != &other) {
    Reset();
    path_ = other.Release();
  }
  return *this;
}

auto StagedFile::Path() const -> QString { return path_; }

auto StagedFile::Empty() const -> bool { return path_.isEmpty(); }

auto StagedFile::Release() -> QString {
  auto path = path_;
  path_.clear();
  return path;
}

void StagedFile::Reset() {
  if (path_.isEmpty()) return;
  QFile::remove(path_);
  // The file's own directory (StagingArea::NewPath), if now empty.
  QDir().rmdir(QFileInfo(path_).absolutePath());
  path_.clear();
}

}  // namespace GpgFrontend::Mobile
