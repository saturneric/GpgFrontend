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
#include <QList>
#include <QPair>

#include "plat/common/PlatformFolder.h"

// A local directory, as a desktop picker returns one. Android has its own.
#if !defined(Q_OS_ANDROID)

namespace GpgFrontend {

namespace {

auto DirectoryOf(const QUrl& folder) -> QString {
  return folder.isLocalFile() ? folder.toLocalFile() : QString();
}

}  // namespace

auto PickDocuments(bool, DocumentsPicked) -> bool { return false; }

auto FolderDocumentNames(const QUrl& folder) -> std::optional<QStringList> {
  const QDir dir(DirectoryOf(folder));
  if (DirectoryOf(folder).isEmpty() || !dir.exists()) return std::nullopt;
  return dir.entryList(QDir::Files | QDir::Hidden | QDir::System);
}

auto FolderTreeOf(const QUrl& folder) -> std::optional<FolderTree> {
  const auto root = DirectoryOf(folder);
  const QDir dir(root);
  if (root.isEmpty() || !dir.exists() || !dir.isReadable()) {
    return std::nullopt;
  }
  FolderTree tree;
  tree.name = QFileInfo(dir.absolutePath()).fileName();
  // Breadth first, each folder by name: a folder before what is in it.
  QList<QPair<QString, int>> pending{{QString(), 0}};
  while (!pending.isEmpty()) {
    const auto [under, depth] = pending.takeFirst();
    const QDir here(under.isEmpty() ? root : dir.filePath(under));
    if (!here.isReadable()) return std::nullopt;
    for (const auto& info :
         here.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot |
                                QDir::Hidden | QDir::System | QDir::NoSymLinks,
                            QDir::Name)) {
      if (tree.nodes.size() >= kMaxFolderTreeNodes) {
        tree.truncated = true;
        return tree;
      }
      FolderNode node;
      node.path = under.isEmpty() ? info.fileName()
                                  : under + QLatin1Char('/') + info.fileName();
      node.url = QUrl::fromLocalFile(info.absoluteFilePath());
      node.folder = info.isDir();
      node.size = node.folder ? 0 : info.size();
      tree.nodes.append(node);
      if (node.folder && depth + 1 < kMaxFolderTreeDepth) {
        pending.append({node.path, depth + 1});
      } else if (node.folder) {
        tree.truncated = true;
      }
    }
  }
  return tree;
}

auto CreateFolderChild(const QUrl& folder, const QString& name) -> QUrl {
  const auto directory = DirectoryOf(folder);
  if (directory.isEmpty() || name.isEmpty() ||
      name.contains(QLatin1Char('/')) || name == QStringLiteral(".") ||
      name == QStringLiteral("..")) {
    return {};
  }
  auto path = QDir(directory).filePath(name);
  for (int n = 1; QFileInfo::exists(path); ++n) {
    path = QDir(directory).filePath(QStringLiteral("%1 (%2)").arg(name).arg(n));
  }
  if (!QDir().mkdir(path)) return {};
  return QUrl::fromLocalFile(path);
}

auto DeleteFolderDocument(const QUrl& document) -> bool {
  const auto path = DirectoryOf(document);
  if (path.isEmpty()) return false;
  const QFileInfo info(path);
  if (info.isDir()) return QDir(path).removeRecursively();
  return QFile::remove(path);
}

auto RenameFolderDocument(const QUrl& document, const QString& name) -> QUrl {
  const auto path = DirectoryOf(document);
  if (path.isEmpty() || name.isEmpty() || name.contains(QLatin1Char('/')) ||
      name == QStringLiteral(".") || name == QStringLiteral("..")) {
    return {};
  }
  const auto renamed = QFileInfo(path).dir().filePath(name);
  if (QFileInfo::exists(renamed) || !QFile::rename(path, renamed)) return {};
  return QUrl::fromLocalFile(renamed);
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
