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
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <atomic>

#include "MobileTest.h"
#include "core/function/ArchiveFileOperator.h"
#include "files/FolderArchive.h"
#include "plat/common/PlatformFolder.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::ArchivePaths;
using GpgFrontend::Mobile::ExtractArchiveToFolder;
using GpgFrontend::Mobile::LooksLikeTarArchive;
using GpgFrontend::Mobile::PackFolderTree;

namespace {

void Write(const QString& path, const QByteArray& content) {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(content);
}

auto Node(const QString& path, bool folder = false) -> FolderNode {
  return {path, QUrl(), folder, 0};
}

/// A TAR archive at @p path whose one entry is named @p name, as a hostile
/// one would be: written as given, nothing checked.
void HostileArchive(const QString& path, const QString& name) {
  QFile out(path);
  ASSERT_TRUE(out.open(QIODevice::WriteOnly));
  bool given = false;
  const auto status = ArchiveFileOperator::NewArchiveFromMembersToSink(
      [&given, &name](ArchiveMemberEntry& member) {
        if (given) return false;
        given = true;
        member.relative_path = name;
        member.bytes = GFBuffer(QByteArray("escaped"));
        return true;
      },
      [&out](const char* data, qint64 size) {
        return out.write(data, size) == size;
      },
      ArchiveCompression::kNONE);
  ASSERT_EQ(status, 0);
}

}  // namespace

// What a folder's documents are called in its archive: safe components,
// never a name twice (nor twice but for case), a renamed folder's files
// under its new name.
TEST(FolderArchiveTest, EachDocumentHasASafeAndDistinctPath) {
  const auto paths = ArchivePaths({
      Node("Docs", true),
      Node("docs", true),
      Node("docs/a.txt"),
      Node("Docs/A.txt"),
      Node("Docs/a.txt"),
      Node(".."),
      Node("."),
      Node(QStringLiteral("bad\x01name\\x")),
  });
  ASSERT_TRUE(paths.has_value());
  EXPECT_EQ(*paths,
            (QStringList{"Docs", "docs (2)", "docs (2)/a.txt", "Docs/A.txt",
                         "Docs/a (2).txt", "_", "_ (2)", "badname_x"}));
  // A file whose folder never came first: not placed anywhere.
  EXPECT_FALSE(ArchivePaths({Node("nowhere/a.txt")}).has_value());
}

// Packing that is stopped leaves no archive; a folder whose listing was cut
// short is not packed as if it were whole.
TEST(FolderArchiveTest, AStoppedOrPartialPackLeavesNothing) {
  QTemporaryDir dir;
  Write(dir.filePath("in/a.txt"), "a");
  const auto tree = FolderTreeOf(QUrl::fromLocalFile(dir.filePath("in")));
  ASSERT_TRUE(tree.has_value());
  const auto to = dir.filePath("out.tar");

  std::atomic_bool stop{true};
  auto result = PackFolderTree(*tree, to, 1 << 20, stop);
  EXPECT_FALSE(result.ok);
  EXPECT_TRUE(result.cancelled);
  EXPECT_FALSE(QFile::exists(to));

  auto partial = *tree;
  partial.truncated = true;
  stop = false;
  result = PackFolderTree(partial, to, 1 << 20, stop);
  EXPECT_FALSE(result.ok);
  EXPECT_TRUE(result.too_large);
  EXPECT_FALSE(QFile::exists(to));

  // More than it may come to: refused, nothing kept.
  result = PackFolderTree(*tree, to, 100, stop);
  EXPECT_FALSE(result.ok);
  EXPECT_TRUE(result.too_large);
  EXPECT_FALSE(QFile::exists(to));

  result = PackFolderTree(*tree, to, 1 << 20, stop);
  EXPECT_TRUE(result.ok);
  EXPECT_TRUE(LooksLikeTarArchive(to));
}

// An archive that would write outside its folder, or holds a link, is
// refused whole: nothing at all lands in the destination.
TEST(FolderArchiveTest, AnUnsafeArchiveExtractsNothing) {
  QTemporaryDir dir;
  const auto destination = QUrl::fromLocalFile(dir.filePath("dest"));
  ASSERT_TRUE(QDir().mkpath(dir.filePath("dest")));
  std::atomic_bool stop{false};

  for (const auto* name :
       {"../escaped.txt", "/abs/escaped.txt", "a/../../escaped.txt"}) {
    const auto archive = dir.filePath("hostile.tar");
    HostileArchive(archive, QString::fromLatin1(name));
    const auto staging = dir.filePath("staging");
    const auto result = ExtractArchiveToFolder(archive, destination, "out",
                                               staging, 1 << 20, stop);
    EXPECT_FALSE(result.ok) << name;
    EXPECT_FALSE(result.reason.isEmpty()) << name;
    EXPECT_TRUE(QDir(dir.filePath("dest")).isEmpty()) << name;
    EXPECT_FALSE(QFile::exists(dir.filePath("escaped.txt"))) << name;
    EXPECT_FALSE(QFile::exists(staging)) << name;
  }

  // A link, made by the system's tar where there is one.
  const auto tar = QStandardPaths::findExecutable("tar");
  if (tar.isEmpty()) return;
  ASSERT_TRUE(QDir().mkpath(dir.filePath("linked")));
  ASSERT_TRUE(QFile::link("/etc/hostname", dir.filePath("linked/link")));
  QProcess process;
  process.setWorkingDirectory(dir.filePath("linked"));
  process.start(tar, {"-cf", dir.filePath("link.tar"), "link"});
  ASSERT_TRUE(process.waitForFinished(10000));
  const auto result =
      ExtractArchiveToFolder(dir.filePath("link.tar"), destination, "out",
                             dir.filePath("staging"), 1 << 20, stop);
  EXPECT_FALSE(result.ok);
  EXPECT_TRUE(QDir(dir.filePath("dest")).isEmpty());
}

// What is not a TAR archive is not taken for one, whatever it is called.
TEST(FolderArchiveTest, OnlyATarHeaderMakesAnArchive) {
  QTemporaryDir dir;
  Write(dir.filePath("plain.tar"), QByteArray(2048, 'x'));
  Write(dir.filePath("short.tar"), "tiny");
  Write(dir.filePath("zeros.tar"), QByteArray(1024, '\0'));
  EXPECT_FALSE(LooksLikeTarArchive(dir.filePath("plain.tar")));
  EXPECT_FALSE(LooksLikeTarArchive(dir.filePath("short.tar")));
  EXPECT_FALSE(LooksLikeTarArchive(dir.filePath("zeros.tar")));
  EXPECT_FALSE(LooksLikeTarArchive(dir.filePath("missing.tar")));
  // A real one, under any name.
  HostileArchive(dir.filePath("real.bin"), "fine.txt");
  EXPECT_TRUE(LooksLikeTarArchive(dir.filePath("real.bin")));
}

// A folder's tree, as the desktop lists one: a folder before what is in it,
// empty folders too, sizes from the files.
TEST(FolderArchiveTest, AFolderTreeListsAFolderBeforeItsContents) {
  QTemporaryDir dir;
  Write(dir.filePath("Top/b/c/deep.txt"), "deep");
  Write(dir.filePath("Top/a.txt"), "aa");
  ASSERT_TRUE(QDir().mkpath(dir.filePath("Top/empty")));
  const auto tree = FolderTreeOf(QUrl::fromLocalFile(dir.filePath("Top")));
  ASSERT_TRUE(tree.has_value());
  EXPECT_EQ(tree->name, "Top");
  EXPECT_FALSE(tree->truncated);
  QStringList paths;
  for (const auto& node : tree->nodes) {
    paths << node.path;
    if (node.path == "a.txt") EXPECT_EQ(node.size, 2);
  }
  EXPECT_EQ(paths, (QStringList{"a.txt", "b", "empty", "b/c", "b/c/deep.txt"}));
  EXPECT_FALSE(FolderTreeOf(QUrl::fromLocalFile(dir.filePath("none"))));
}

}  // namespace GpgFrontend::Test::Mobile
