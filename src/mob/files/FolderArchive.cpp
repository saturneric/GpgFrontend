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

#include "files/FolderArchive.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>

#include "core/function/ArchiveFileOperator.h"
#include "files/FileOperations.h"
#include "files/StagingArea.h"

namespace GpgFrontend::Mobile {

namespace {

auto Tr(const char* text) -> QString {
  return QCoreApplication::translate("FolderArchive", text);
}

/// @p name as one archive path component.
auto SafeComponent(const QString& name) -> QString {
  QString safe;
  safe.reserve(name.size());
  for (const auto c : name) {
    if (c.unicode() < 0x20 || c.unicode() == 0x7f) continue;
    // A backslash is a separator to the extractor, as on Windows.
    safe.append(
        c == QLatin1Char('/') || c == QLatin1Char('\\') ? QLatin1Char('_') : c);
  }
  if (safe.isEmpty() || safe == QStringLiteral(".") ||
      safe == QStringLiteral("..")) {
    return QStringLiteral("_");
  }
  return safe;
}

/// "a (2).txt" for a file, "a (2)" for a folder.
auto Numbered(const QString& name, int n, bool folder) -> QString {
  const auto dot = folder ? -1 : name.lastIndexOf(QLatin1Char('.'));
  if (dot <= 0) return QStringLiteral("%1 (%2)").arg(name).arg(n);
  return QStringLiteral("%1 (%2)%3")
      .arg(name.left(dot))
      .arg(n)
      .arg(name.mid(dot));
}

auto Join(const QString& parent, const QString& name) -> QString {
  return parent.isEmpty() ? name : parent + QLatin1Char('/') + name;
}

/// How much of a file is copied at a time, and reported.
constexpr qint64 kChunk = 1024 * 1024;

/// Every entry an extracted archive may have, at most.
constexpr int kMaxExtractedEntries = 100000;

}  // namespace

auto ArchivePaths(const QList<FolderNode>& nodes)
    -> std::optional<QStringList> {
  QStringList paths;
  paths.reserve(nodes.size());
  QHash<QString, QString> folders;  // a folder's path -> its archive path
  QSet<QString> taken;              // archive paths, lower-cased
  auto policy = ArchiveExtractPolicy::Permissive();
  policy.max_depth = kMaxFolderTreeDepth + 1;
  for (const auto& node : nodes) {
    const auto cut = node.path.lastIndexOf(QLatin1Char('/'));
    const auto parent = cut < 0 ? QString() : node.path.left(cut);
    QString under;
    if (!parent.isEmpty()) {
      const auto found = folders.constFind(parent);
      // A folder always comes before what is in it.
      if (found == folders.constEnd()) return std::nullopt;
      under = *found;
    }
    const auto safe =
        SafeComponent(cut < 0 ? node.path : node.path.mid(cut + 1));
    auto path = Join(under, safe);
    for (int n = 2; taken.contains(path.toLower()); ++n) {
      path = Join(under, Numbered(safe, n, node.folder));
    }
    QString normalised;
    if (ValidateArchiveEntryPath(path, policy, normalised) !=
            ArchiveEntryVerdict::kACCEPT ||
        normalised != path) {
      return std::nullopt;
    }
    taken.insert(path.toLower());
    if (node.folder) folders.insert(node.path, path);
    paths << path;
  }
  return paths;
}

auto PackFolderTree(const FolderTree& tree, const QString& to, qint64 limit,
                    const std::atomic_bool& stop,
                    const ArchiveProgress& progress) -> FolderPackResult {
  FolderPackResult result;
  const auto paths = ArchivePaths(tree.nodes);
  QFile out(to);
  if (!paths || tree.truncated ||
      !out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    result.too_large = tree.truncated;
    return result;
  }

  // Roughly what it comes to: the data, and a header block for each entry.
  qint64 total = 0;
  for (const auto& node : tree.nodes) total += node.size + 512;

  qsizetype next = 0;
  QString unreadable;
  const auto provide = [&](ArchiveMemberEntry& member) -> bool {
    if (stop.load() || next >= tree.nodes.size()) return false;
    const auto& node = tree.nodes.at(next);
    member.relative_path = paths->at(next);
    ++next;
    if (node.folder) {
      member.directory = true;
      return true;
    }
    // One that cannot be read fails the archive: left out, the archive
    // would hold less than the folder and say nothing of it.
    const auto source = LocalOrContent(node.url);
    if (QFile probe(source); !probe.open(QIODevice::ReadOnly)) {
      unreadable = node.path;
      return false;
    }
    member.source_file = source;
    return true;
  };

  qint64 written = 0;
  qint64 reported = 0;
  bool kept = true;
  const auto keep = [&](const char* data, qint64 size) -> bool {
    if (stop.load()) return false;
    written += size;
    if (written > limit) {
      result.too_large = true;
      return false;
    }
    if (out.write(data, size) != size) {
      kept = false;
      return false;
    }
    if (progress && written - reported >= kChunk) {
      reported = written;
      progress(written, std::max(total, written));
    }
    return true;
  };

  const auto status = ArchiveFileOperator::NewArchiveFromMembersToSink(
      provide, keep, ArchiveCompression::kNONE);
  result.cancelled = stop.load();
  result.unreadable = unreadable;
  result.ok = status == 0 && kept && !result.cancelled &&
              unreadable.isEmpty() && !result.too_large &&
              next == tree.nodes.size() && out.flush();
  out.close();
  if (!result.ok) QFile::remove(to);
  return result;
}

auto LooksLikeTarArchive(const QString& path) -> bool {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return false;
  const auto header = file.read(512);
  if (header.size() < 512) return false;
  // A block of zeros ends an archive: one that starts with it holds nothing.
  if (header.count('\0') == header.size()) return false;
  // The checksum: every byte of the header, its own field read as spaces.
  const auto field = header.mid(148, 8);
  bool parsed = false;
  const auto stored = QString::fromLatin1(field)
                          .remove(QLatin1Char('\0'))
                          .trimmed()
                          .toLongLong(&parsed, 8);
  if (!parsed) return false;
  qint64 sum_unsigned = 0;
  qint64 sum_signed = 0;
  for (int i = 0; i < 512; ++i) {
    const auto byte = (i >= 148 && i < 156) ? ' ' : header.at(i);
    sum_unsigned += static_cast<unsigned char>(byte);
    sum_signed += static_cast<signed char>(byte);
  }
  return stored == sum_unsigned || stored == sum_signed;
}

auto ExtractArchiveToFolder(const QString& archive, const QUrl& destination,
                            const QString& name, const QString& staging_dir,
                            qint64 limit, const std::atomic_bool& stop,
                            const ArchiveProgress& progress)
    -> FolderExtractResult {
  FolderExtractResult result;
  const auto clean = [&staging_dir]() {
    QDir(staging_dir).removeRecursively();
  };

  // First into a directory of our own, every entry checked: nothing that
  // leaves it, no links or devices, no name twice.
  auto policy = ArchiveExtractPolicy::Strict(limit, kMaxExtractedEntries);
  policy.allow_symlinks = false;
  policy.allow_hardlinks = false;
  policy.reject_duplicate_paths = true;
  policy.reject_case_colliding_paths = true;
  policy.max_depth = kMaxFolderTreeDepth + 1;
  policy.max_path_length = 4096;
  QString refused;
  if (ArchiveFileOperator::ExtractArchiveFromFileSync(
          archive, staging_dir, policy, {}, {}, {}, &refused) != 0) {
    clean();
    result.reason = refused.isEmpty()
                        ? Tr("It is not an archive that can be read.")
                        : Tr("The archive was refused: %1").arg(refused);
    return result;
  }
  if (stop.load()) {
    clean();
    result.cancelled = true;
    return result;
  }

  // What it holds, a folder before what is in it.
  struct Item {
    QString path;
    bool folder;
    qint64 size;
  };
  QList<Item> items;
  qint64 total = 0;
  const QDir root(staging_dir);
  QDirIterator walk(
      staging_dir,
      QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
      QDirIterator::Subdirectories);
  while (walk.hasNext()) {
    const auto info = walk.nextFileInfo();
    if (info.isSymLink()) {
      clean();
      result.reason = Tr("The archive holds a link, which is not extracted.");
      return result;
    }
    items.append({root.relativeFilePath(info.absoluteFilePath()), info.isDir(),
                  info.isDir() ? 0 : info.size()});
    total += items.last().size;
  }
  std::sort(items.begin(), items.end(),
            [](const Item& a, const Item& b) { return a.path < b.path; });

  const auto made = CreateFolderChild(destination, name);
  if (made.isEmpty()) {
    clean();
    result.reason = Tr("No folder could be made there.");
    return result;
  }
  const auto undo = [&](const QString& reason) {
    DeleteFolderDocument(made);
    clean();
    result.reason = reason;
    return result;
  };

  QHash<QString, QUrl> folders;
  qint64 copied = 0;
  qint64 reported = 0;
  QByteArray chunk;
  for (const auto& item : items) {
    if (stop.load()) {
      result.cancelled = true;
      return undo({});
    }
    const auto cut = item.path.lastIndexOf(QLatin1Char('/'));
    const auto parent =
        cut < 0 ? made : folders.value(item.path.left(cut), QUrl());
    const auto leaf = cut < 0 ? item.path : item.path.mid(cut + 1);
    if (parent.isEmpty()) return undo(Tr("A folder could not be made there."));
    if (item.folder) {
      const auto folder = CreateFolderChild(parent, leaf);
      if (folder.isEmpty()) {
        return undo(Tr("A folder could not be made there."));
      }
      folders.insert(item.path, folder);
      continue;
    }
    const auto document = CreateFolderDocument(
        parent, leaf, QStringLiteral("application/octet-stream"));
    QFile in(root.filePath(item.path));
    QFile out(LocalOrContent(document));
    if (document.isEmpty() || !in.open(QIODevice::ReadOnly) ||
        !out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      return undo(Tr("A file could not be written there."));
    }
    while (!in.atEnd()) {
      if (stop.load()) {
        result.cancelled = true;
        out.close();
        return undo({});
      }
      chunk = in.read(kChunk);
      if (chunk.isEmpty() && in.error() != QFileDevice::NoError) {
        return undo(Tr("A file could not be written there."));
      }
      if (out.write(chunk) != chunk.size()) {
        return undo(Tr("A file could not be written there."));
      }
      copied += chunk.size();
      if (progress && copied - reported >= kChunk) {
        reported = copied;
        progress(copied, total);
      }
    }
    if (!out.flush()) return undo(Tr("A file could not be written there."));
    ++result.files;
  }

  clean();
  result.ok = true;
  result.folder_name = DisplayName(made);
  return result;
}

}  // namespace GpgFrontend::Mobile
