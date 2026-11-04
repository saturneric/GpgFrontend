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

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "plat/common/PlatformFolder.h"

// A local directory, as a desktop picker returns one. Android has its own.
#if !defined(Q_OS_ANDROID)

namespace GpgFrontend {

namespace {

auto DirectoryOf(const QUrl& folder) -> QString {
  return folder.isLocalFile() ? folder.toLocalFile() : QString();
}

}  // namespace

auto FolderDocumentNames(const QUrl& folder) -> std::optional<QStringList> {
  const QDir dir(DirectoryOf(folder));
  if (DirectoryOf(folder).isEmpty() || !dir.exists()) return std::nullopt;
  return dir.entryList(QDir::Files | QDir::Hidden | QDir::System);
}

auto FindFolderDocument(const QUrl& folder, const QString& name) -> QUrl {
  const auto directory = DirectoryOf(folder);
  if (directory.isEmpty() || name.contains(QLatin1Char('/'))) return {};
  const auto path = QDir(directory).filePath(name);
  return QFileInfo(path).isFile() ? QUrl::fromLocalFile(path) : QUrl();
}

auto CreateFolderDocument(const QUrl& folder, const QString& name,
                          const QString&) -> QUrl {
  const auto directory = DirectoryOf(folder);
  if (directory.isEmpty() || name.isEmpty() ||
      name.contains(QLatin1Char('/'))) {
    return {};
  }
  // Numbered as Android's providers number a name that is taken.
  const auto dot = name.lastIndexOf(QLatin1Char('.'));
  const auto base = dot > 0 ? name.left(dot) : name;
  const auto extension = dot > 0 ? name.mid(dot) : QString();
  auto path = QDir(directory).filePath(name);
  for (int n = 1; QFileInfo::exists(path); ++n) {
    path = QDir(directory).filePath(
        QStringLiteral("%1 (%2)%3").arg(base).arg(n).arg(extension));
  }
  QFile file(path);
  if (!file.open(QIODevice::NewOnly)) return {};
  return QUrl::fromLocalFile(path);
}

}  // namespace GpgFrontend

#endif
