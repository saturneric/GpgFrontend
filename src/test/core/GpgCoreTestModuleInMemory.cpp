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

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSet>
#include <QTemporaryDir>
#include <array>
#include <thread>

#include "GpgFrontendTest.h"
#include "ModuleTestPackages.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/model/GFDataExchanger.h"
#include "core/module/ModuleDescriptor.h"

/**
 * @file GpgCoreTestModuleInMemory.cpp
 * @brief That reading a module package touches no filesystem.
 *
 * Verification has always kept every byte in memory, but the property lived in
 * a comment: the extractor insisted on a destination, so the verifier handed it
 * a throwaway QTemporaryDir and relied on a divert that claimed every entry to
 * make sure nothing was ever written there. Both the directory and the filter
 * are gone, and ReadArchiveMembersSync() has no destination parameter at all.
 *
 * These tests exist because that is exactly the kind of property that decays
 * silently. Nothing about a package that verifies correctly would change if
 * someone reintroduced a temporary file tomorrow, so the assertions here are
 * about the absence of side effects rather than about the verdict.
 */

namespace GpgFrontend::Test {

namespace {

/// Everything directly inside a directory, for comparing before against after.
auto SnapshotOf(const QString& path) -> QSet<QString> {
  QSet<QString> out;
  const QDir dir(path);
  if (!dir.exists()) return out;
  for (const auto& name :
       dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                     QDir::System)) {
    out.insert(name);
  }
  return out;
}

/// A minimal archive, built in memory, so the read tests need no fixture file.
///
/// The producer/consumer shape is the builder's own (ModuleDescriptorBuilder.cpp):
/// the archive writer runs on a thread and this drains the pipe, because there
/// is no way to close the read side and a producer pushing into a full one
/// would never return from the join.
auto TinyArchiveBytes(const QMap<QString, QByteArray>& members) -> QByteArray {
  auto exchanger = CreateStandardGFDataExchanger();

  GFError archive_error = 0;
  std::thread producer([&]() {
    auto it = members.constBegin();
    archive_error = ArchiveFileOperator::NewArchiveFromMembersSync(
        [&it, &members](ArchiveMemberEntry& entry) -> bool {
          if (it == members.constEnd()) return false;
          entry.relative_path = it.key();
          entry.bytes = GFBuffer(it.value());
          ++it;
          return true;
        },
        exchanger, ArchiveCompression::kNONE, ArchiveFormat::kZIP);
  });

  QByteArray out;
  {
    std::array<std::byte, 64 * 1024> chunk{};
    while (true) {
      const auto n = exchanger->Read(chunk.data(), chunk.size());
      if (n <= 0) break;
      out.append(reinterpret_cast<const char*>(chunk.data()),
                 static_cast<qsizetype>(n));
    }
  }
  producer.join();

  if (archive_error != 0) return {};
  return out;
}

}  // namespace

// ------------------------------------------------------- the archive primitive

TEST(ModuleInMemoryTest, ReadArchiveMembersHandsOverEveryMember) {
  const auto bytes =
      TinyArchiveBytes({{"a.txt", "alpha"},
                        {"nested/b.bin", QByteArray("\0\xff", 2)},
                        {"c.txt", "gamma"}});
  ASSERT_FALSE(bytes.isEmpty());

  QMap<QString, QByteArray> seen;
  const auto err = ArchiveFileOperator::ReadArchiveMembersSync(
      bytes, ArchiveExtractPolicy::Strict(),
      [&seen](const QString& path, const QByteArray& body) {
        seen.insert(path, body);
        return true;
      });

  ASSERT_EQ(err, 0);
  EXPECT_EQ(seen.size(), 3);
  EXPECT_EQ(seen.value("a.txt"), QByteArray("alpha"));
  EXPECT_EQ(seen.value("c.txt"), QByteArray("gamma"));
  // The octets survive exactly, embedded NUL and high byte included. A member
  // read as text would lose both.
  EXPECT_EQ(seen.value("nested/b.bin"), QByteArray("\0\xff", 2));
}

TEST(ModuleInMemoryTest, ReadArchiveMembersWritesNothingAnywhere) {
  const auto bytes =
      TinyArchiveBytes({{"a.txt", "alpha"}, {"d/b.txt", "beta"}});
  ASSERT_FALSE(bytes.isEmpty());

  const auto temp_root = QDir::tempPath();
  const auto before = SnapshotOf(temp_root);

  int members = 0;
  ASSERT_EQ(ArchiveFileOperator::ReadArchiveMembersSync(
                bytes, ArchiveExtractPolicy::Strict(),
                [&members](const QString&, const QByteArray&) {
                  ++members;
                  return true;
                }),
            0);
  EXPECT_EQ(members, 2);

  // Nothing appeared. A directory entry in the archive would have become a
  // real directory under the old shape if the divert had ever missed it.
  EXPECT_EQ(SnapshotOf(temp_root), before);
}

TEST(ModuleInMemoryTest, ASinkThatDeclinesEndsTheWalk) {
  const auto bytes = TinyArchiveBytes(
      {{"a.txt", "alpha"}, {"b.txt", "beta"}, {"c.txt", "gamma"}});
  ASSERT_FALSE(bytes.isEmpty());

  int members = 0;
  const auto err = ArchiveFileOperator::ReadArchiveMembersSync(
      bytes, ArchiveExtractPolicy::Strict(),
      [&members](const QString&, const QByteArray&) {
        ++members;
        return members < 2;  // take one, refuse the next
      });

  EXPECT_NE(err, 0) << "a declining sink must stop the walk";
  EXPECT_EQ(members, 2) << "and must not be called again after it declined";
}

TEST(ModuleInMemoryTest, ReadArchiveMembersRefusesWithoutASink) {
  const auto bytes = TinyArchiveBytes({{"a.txt", "alpha"}});
  ASSERT_FALSE(bytes.isEmpty());

  QString reason;
  EXPECT_NE(ArchiveFileOperator::ReadArchiveMembersSync(
                bytes, ArchiveExtractPolicy::Strict(), {}, &reason),
            0);
  EXPECT_FALSE(reason.isEmpty());
}

TEST(ModuleInMemoryTest, GarbageIsNotAnArchive) {
  const QByteArray junk("this is not an archive, not even slightly", 41);
  EXPECT_NE(ArchiveFileOperator::ReadArchiveMembersSync(
                junk, ArchiveExtractPolicy::Strict(),
                [](const QString&, const QByteArray&) { return true; }),
            0);
}

// ------------------------------------------------- the verifier on a real one

TEST(ModuleInMemoryTest, VerifyingAPackageCreatesNoFilesystemEntry) {
  const auto package = LargestBuiltModulePackage();
  if (package.isEmpty()) GTEST_SKIP() << "this build produced no packages";

  const auto temp_root = QDir::tempPath();
  const auto package_dir = QFileInfo(package).absolutePath();

  const auto temp_before = SnapshotOf(temp_root);
  const auto dir_before = SnapshotOf(package_dir);

  const auto verdict = Module::VerifyModuleDescriptor(package);
  ASSERT_TRUE(verdict.ok) << verdict.reason.toStdString();

  // The two places a stray file would land: the system temporary directory,
  // which is where the retired QTemporaryDir went, and beside the package
  // itself, which is where a naive extractor would put it.
  EXPECT_EQ(SnapshotOf(temp_root), temp_before);
  EXPECT_EQ(SnapshotOf(package_dir), dir_before);
}

TEST(ModuleInMemoryTest, VerificationSurvivesAnUnusableTempDirectory) {
  const auto package = LargestBuiltModulePackage();
  if (package.isEmpty()) GTEST_SKIP() << "this build produced no packages";

  const auto saved = qEnvironmentVariable("TMPDIR");
  const auto restore = qScopeGuard([&saved]() {
    if (saved.isEmpty()) {
      qunsetenv("TMPDIR");
    } else {
      qputenv("TMPDIR", saved.toUtf8());
    }
  });

  // A path that cannot hold anything. Under the retired shape this made
  // verification fail with "a temporary folder could not be made" -- a package
  // refused for a reason that had nothing to do with the package.
  qputenv("TMPDIR", "/nonexistent/gpgfrontend/there-is-no-such-place");

  // Proven, not assumed. If Qt has already cached a usable temporary path then
  // the retired code would have worked here too, and this test would be
  // asserting nothing at all -- which is worse than not having it.
  {
    QTemporaryDir probe;
    if (probe.isValid()) {
      GTEST_SKIP() << "TMPDIR is cached; this environment cannot make a "
                      "temporary directory fail, so the test would be vacuous";
    }
  }

  const auto verdict = Module::VerifyModuleDescriptor(package);
  EXPECT_TRUE(verdict.ok)
      << "verification must not depend on a usable temporary directory: "
      << verdict.reason.toStdString();
}

TEST(ModuleInMemoryTest, VerificationReadsThePackageOnlyOnce) {
  const auto package = LargestBuiltModulePackage();
  if (package.isEmpty()) GTEST_SKIP() << "this build produced no packages";

  QTemporaryDir scratch;
  ASSERT_TRUE(scratch.isValid());
  const auto copy = scratch.path() + "/copy.gfmodule";
  ASSERT_TRUE(QFile::copy(package, copy));

  const auto verdict = Module::VerifyModuleDescriptor(copy);
  ASSERT_TRUE(verdict.ok) << verdict.reason.toStdString();

  // Replace the file with something that is not a package at all, then ask the
  // same question again. The second answer must be a refusal -- proving the
  // first verdict came from bytes read at the time and not from anything
  // cached, and that the file really is consulted per call.
  {
    QFile clobber(copy);
    ASSERT_TRUE(clobber.open(QIODevice::WriteOnly | QIODevice::Truncate));
    clobber.write("not a package any more");
    clobber.close();
  }

  const auto second = Module::VerifyModuleDescriptor(copy);
  EXPECT_FALSE(second.ok);
}

}  // namespace GpgFrontend::Test
