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
#include <QSharedPointer>
#include <QTemporaryDir>
#include <thread>

#ifndef Q_OS_WINDOWS
#include <unistd.h>
#endif

#include "core/function/ArchiveFileOperator.h"

namespace {

void CreateTestFile(const QString& dir, const QString& name,
                    const QByteArray& content) {
  QFile f(dir + "/" + name);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write(content);
  f.close();
}

constexpr int kTarBlock = 512;

void WriteOctalField(char* field, int width, quint64 value) {
  auto s = QByteArray::number(static_cast<qulonglong>(value), 8);
  while (s.size() < width - 1) s.prepend('0');
  memcpy(field, s.constData(), width - 1);
  field[width - 1] = '\0';
}

/**
 * @brief Build one ustar header block by hand.
 *
 * Hand-rolled rather than produced with libarchive because these tests exist to
 * feed the extractor entries libarchive would refuse to write: names that
 * escape the destination, absolute names, link types the format never emits.
 */
auto TarHeader(const QByteArray& name, char typeflag, qint64 size,
               const QByteArray& linkname = {}) -> QByteArray {
  QByteArray h(kTarBlock, '\0');
  char* p = h.data();

  memcpy(p, name.constData(), std::min<qsizetype>(name.size(), 100));
  WriteOctalField(p + 100, 8, 0644);
  WriteOctalField(p + 108, 8, 0);
  WriteOctalField(p + 116, 8, 0);
  WriteOctalField(p + 124, 12, static_cast<quint64>(size));
  WriteOctalField(p + 136, 12, 0);
  memset(p + 148, ' ', 8);  // checksum is computed over spaces
  p[156] = typeflag;
  if (!linkname.isEmpty()) {
    memcpy(p + 157, linkname.constData(),
           std::min<qsizetype>(linkname.size(), 100));
  }
  memcpy(p + 257, "ustar", 5);
  memcpy(p + 263, "00", 2);

  unsigned sum = 0;
  for (int i = 0; i < kTarBlock; ++i) {
    sum += static_cast<unsigned char>(h[i]);
  }
  WriteOctalField(p + 148, 7, sum);
  p[155] = ' ';

  return h;
}

auto TarPad(const QByteArray& data) -> QByteArray {
  QByteArray out = data;
  const auto rem = out.size() % kTarBlock;
  if (rem != 0) out.append(kTarBlock - rem, '\0');
  return out;
}

auto TarEnd() -> QByteArray { return QByteArray(kTarBlock * 2, '\0'); }

/// Push a whole archive into an exchanger and close it. Safe without a reader:
/// the standard exchanger buffers 4 MB and every archive here is a few KB.
auto ExchangerWith(const QByteArray& bytes)
    -> QSharedPointer<GpgFrontend::GFDataExchanger> {
  auto ex = GpgFrontend::CreateStandardGFDataExchanger();
  ex->Write(reinterpret_cast<const std::byte*>(bytes.constData()),
            bytes.size());
  ex->CloseWrite();
  return ex;
}

auto ExtractSync(const QSharedPointer<GpgFrontend::GFDataExchanger>& ex,
                 const QString& dest,
                 const GpgFrontend::ArchiveExtractPolicy& policy) -> int {
  int result = 0;
  bool finished = false;
  GpgFrontend::ArchiveFileOperator::ExtractArchiveFromDataExchanger(
      ex, dest,
      [&](GpgFrontend::GFError err, GpgFrontend::DataObjectPtr) {
        result = static_cast<int>(err);
        finished = true;
      },
      policy);
  WAIT_FOR_TRUE(finished, 3000);
  return result;
}

}  // namespace

namespace GpgFrontend::Test {

// ---------------------------------------------------------------- pure rules

TEST(ArchiveEntryPathTest, AcceptsOrdinaryRelativePaths) {
  QString out;
  auto policy = ArchiveExtractPolicy::Permissive();

  EXPECT_EQ(ValidateArchiveEntryPath("a.txt", policy, out),
            ArchiveEntryVerdict::kACCEPT);
  EXPECT_EQ(out, QString("a.txt"));

  EXPECT_EQ(ValidateArchiveEntryPath("dir/sub/a.txt", policy, out),
            ArchiveEntryVerdict::kACCEPT);
  EXPECT_EQ(out, QString("dir/sub/a.txt"));

  // "." components are noise, not an escape
  EXPECT_EQ(ValidateArchiveEntryPath("./dir/./a.txt", policy, out),
            ArchiveEntryVerdict::kACCEPT);
  EXPECT_EQ(out, QString("dir/a.txt"));
}

TEST(ArchiveEntryPathTest, RejectsEscapes) {
  QString out;
  auto policy = ArchiveExtractPolicy::Permissive();

  EXPECT_EQ(ValidateArchiveEntryPath("../escape.txt", policy, out),
            ArchiveEntryVerdict::kREJECT_DOTDOT);
  EXPECT_TRUE(out.isEmpty());

  EXPECT_EQ(ValidateArchiveEntryPath("a/../../escape.txt", policy, out),
            ArchiveEntryVerdict::kREJECT_DOTDOT);

  // a backslash separates on Windows, so it must not smuggle an escape past a
  // forward-slash-only check when the archive is opened on Linux
  EXPECT_EQ(ValidateArchiveEntryPath("..\\escape.txt", policy, out),
            ArchiveEntryVerdict::kREJECT_DOTDOT);
}

TEST(ArchiveEntryPathTest, RejectsAbsolutePaths) {
  QString out;
  auto policy = ArchiveExtractPolicy::Permissive();

  EXPECT_EQ(ValidateArchiveEntryPath("/etc/passwd", policy, out),
            ArchiveEntryVerdict::kREJECT_ABSOLUTE);
  EXPECT_EQ(ValidateArchiveEntryPath("C:/Windows/x", policy, out),
            ArchiveEntryVerdict::kREJECT_ABSOLUTE);
  EXPECT_EQ(ValidateArchiveEntryPath("C:\\Windows\\x", policy, out),
            ArchiveEntryVerdict::kREJECT_ABSOLUTE);
  EXPECT_EQ(ValidateArchiveEntryPath("\\\\host\\share\\x", policy, out),
            ArchiveEntryVerdict::kREJECT_ABSOLUTE);
}

TEST(ArchiveEntryPathTest, RejectsEmptyAndOverLongAndTooDeep) {
  QString out;
  auto policy = ArchiveExtractPolicy::Permissive();

  EXPECT_EQ(ValidateArchiveEntryPath("", policy, out),
            ArchiveEntryVerdict::kREJECT_EMPTY);
  EXPECT_EQ(ValidateArchiveEntryPath(".", policy, out),
            ArchiveEntryVerdict::kREJECT_EMPTY);
  EXPECT_EQ(ValidateArchiveEntryPath("./././", policy, out),
            ArchiveEntryVerdict::kREJECT_EMPTY);
  // a run of separators is still rooted, so it is refused as absolute
  EXPECT_EQ(ValidateArchiveEntryPath("///", policy, out),
            ArchiveEntryVerdict::kREJECT_ABSOLUTE);

  policy.max_path_length = 8;
  EXPECT_EQ(ValidateArchiveEntryPath("0123456789", policy, out),
            ArchiveEntryVerdict::kREJECT_PATH_TOO_LONG);

  policy = ArchiveExtractPolicy::Permissive();
  policy.max_depth = 3;
  EXPECT_EQ(ValidateArchiveEntryPath("a/b/c", policy, out),
            ArchiveEntryVerdict::kACCEPT);
  EXPECT_EQ(ValidateArchiveEntryPath("a/b/c/d", policy, out),
            ArchiveEntryVerdict::kREJECT_TOO_DEEP);
}

TEST(ArchiveEntryPathTest, StrictPolicyRefusesLinksAndDemandsEmptyDest) {
  const auto strict = ArchiveExtractPolicy::Strict(1024, 8);
  EXPECT_FALSE(strict.allow_symlinks);
  EXPECT_FALSE(strict.allow_hardlinks);
  EXPECT_TRUE(strict.require_empty_destination);
  EXPECT_EQ(strict.max_total_bytes, 1024);
  EXPECT_EQ(strict.max_entries, 8);

  const auto permissive = ArchiveExtractPolicy::Permissive();
  EXPECT_TRUE(permissive.allow_symlinks);
  EXPECT_TRUE(permissive.allow_hardlinks);
  EXPECT_FALSE(permissive.require_empty_destination);
  EXPECT_EQ(permissive.max_total_bytes, -1);
}

// ------------------------------------------------------------- round trips

// The synchronous pair exists for callers that both produce and consume the
// stream themselves — the profile packer — and would otherwise be waiting on a
// completion callback addressed to a thread that is busy waiting.
TEST(ArchiveFileOperatorTest, TheSynchronousPairRoundTrips) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto src_dir = temp_dir.path() + "/src";
  QDir().mkpath(src_dir + "/nested");

  CreateTestFile(src_dir, "test1.txt", "hello world 1");
  CreateTestFile(src_dir + "/nested", "test2.txt", "hello world 2");

  auto exchanger = GpgFrontend::CreateStandardGFDataExchanger();
  GpgFrontend::GFError archive_error = 0;

  QByteArray payload;
  std::thread producer([&]() {
    archive_error =
        GpgFrontend::ArchiveFileOperator::NewArchive2DataExchangerSync(
            src_dir, exchanger, GpgFrontend::ArchiveCompression::kGZIP);
  });

  QByteArray chunk(64 * 1024, Qt::Uninitialized);
  while (true) {
    const auto read = exchanger->Read(
        reinterpret_cast<std::byte*>(chunk.data()), chunk.size());
    if (read <= 0) break;
    payload.append(chunk.constData(), static_cast<int>(read));
  }
  producer.join();

  ASSERT_EQ(archive_error, 0);
  ASSERT_FALSE(payload.isEmpty());
  EXPECT_EQ(payload.left(2), QByteArray("\x1f\x8b", 2));  // gzip

  QTemporaryDir out_dir;
  ASSERT_TRUE(out_dir.isValid());

  auto back = GpgFrontend::CreateStandardGFDataExchanger();
  std::thread feeder([&]() {
    back->Write(reinterpret_cast<const std::byte*>(payload.constData()),
                payload.size());
    back->CloseWrite();
  });
  const auto extract_error =
      GpgFrontend::ArchiveFileOperator::ExtractArchiveFromDataExchangerSync(
          back, out_dir.path() + "/out",
          GpgFrontend::ArchiveExtractPolicy::Strict());
  feeder.join();

  ASSERT_EQ(extract_error, 0);
  QFile f(out_dir.path() + "/out/nested/test2.txt");
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));
  EXPECT_EQ(f.readAll(), QByteArray("hello world 2"));
}

TEST(ArchiveFileOperatorTest, ArchiveAndExtract) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  QString src_dir = temp_dir.path() + "/src";
  QDir().mkpath(src_dir);

  CreateTestFile(src_dir, "test1.txt", "hello world 1");
  CreateTestFile(src_dir, "test2.txt", "hello world 2");

  auto exchanger = CreateStandardGFDataExchanger();
  bool archive_finished = false;

  GpgFrontend::ArchiveFileOperator::NewArchive2DataExchanger(
      src_dir, exchanger, [&](GFError err, DataObjectPtr) {
        EXPECT_EQ(err, 0);
        archive_finished = true;
      });

  WAIT_FOR_TRUE(archive_finished, 3000);

  QTemporaryDir extract_dir;
  ASSERT_TRUE(extract_dir.isValid());
  ASSERT_EQ(ExtractSync(exchanger, extract_dir.path(),
                        ArchiveExtractPolicy::Permissive()),
            0);

  QFile f1(extract_dir.path() + "/test1.txt");
  QFile f2(extract_dir.path() + "/test2.txt");
  ASSERT_TRUE(f1.open(QIODevice::ReadOnly));
  ASSERT_TRUE(f2.open(QIODevice::ReadOnly));
  EXPECT_EQ(f1.readAll(), QByteArray("hello world 1"));
  EXPECT_EQ(f2.readAll(), QByteArray("hello world 2"));
}

TEST(ArchiveFileOperatorTest, DirectoriesIncludingEmptyOnesSurvive) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto src_dir = temp_dir.path() + "/src";

  // an empty directory used to vanish: an entry was written only when the path
  // opened for reading, which never succeeds on a directory
  ASSERT_TRUE(QDir().mkpath(src_dir + "/empty_dir"));
  ASSERT_TRUE(QDir().mkpath(src_dir + "/nested/deeper"));
  CreateTestFile(src_dir + "/nested/deeper", "leaf.txt", "leaf");

  auto exchanger = CreateStandardGFDataExchanger();
  bool archive_finished = false;
  GpgFrontend::ArchiveFileOperator::NewArchive2DataExchanger(
      src_dir, exchanger, [&](GFError err, DataObjectPtr) {
        EXPECT_EQ(err, 0);
        archive_finished = true;
      });
  WAIT_FOR_TRUE(archive_finished, 3000);

  QTemporaryDir extract_dir;
  ASSERT_TRUE(extract_dir.isValid());
  ASSERT_EQ(ExtractSync(exchanger, extract_dir.path(),
                        ArchiveExtractPolicy::Permissive()),
            0);

  EXPECT_TRUE(QFileInfo(extract_dir.path() + "/empty_dir").isDir());
  EXPECT_TRUE(QFileInfo(extract_dir.path() + "/nested/deeper").isDir());
  QFile leaf(extract_dir.path() + "/nested/deeper/leaf.txt");
  ASSERT_TRUE(leaf.open(QIODevice::ReadOnly));
  EXPECT_EQ(leaf.readAll(), QByteArray("leaf"));
}

TEST(ArchiveFileOperatorTest, GzipRoundTrip) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto src_dir = temp_dir.path() + "/src";
  ASSERT_TRUE(QDir().mkpath(src_dir));
  CreateTestFile(src_dir, "compressible.txt", QByteArray(4096, 'a'));

  auto exchanger = CreateStandardGFDataExchanger();
  bool archive_finished = false;
  GpgFrontend::ArchiveFileOperator::NewArchive2DataExchanger(
      src_dir, exchanger,
      [&](GFError err, DataObjectPtr) {
        EXPECT_EQ(err, 0);
        archive_finished = true;
      },
      ArchiveCompression::kGZIP);
  WAIT_FOR_TRUE(archive_finished, 3000);

  QTemporaryDir extract_dir;
  ASSERT_TRUE(extract_dir.isValid());
  ASSERT_EQ(ExtractSync(exchanger, extract_dir.path(),
                        ArchiveExtractPolicy::Permissive()),
            0);

  QFile f(extract_dir.path() + "/compressible.txt");
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));
  EXPECT_EQ(f.readAll(), QByteArray(4096, 'a'));
}

TEST(ArchiveFileOperatorTest, FilterPrunesWholeSubtree) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto src_dir = temp_dir.path() + "/src";
  ASSERT_TRUE(QDir().mkpath(src_dir + "/keep"));
  ASSERT_TRUE(QDir().mkpath(src_dir + "/drop/inner"));
  CreateTestFile(src_dir + "/keep", "kept.txt", "kept");
  CreateTestFile(src_dir + "/drop/inner", "dropped.txt", "dropped");

  auto exchanger = CreateStandardGFDataExchanger();
  bool archive_finished = false;
  GpgFrontend::ArchiveFileOperator::NewArchive2DataExchanger(
      src_dir, exchanger,
      [&](GFError err, DataObjectPtr) {
        EXPECT_EQ(err, 0);
        archive_finished = true;
      },
      ArchiveCompression::kNONE,
      [](const QString& rel) { return rel != "drop"; });
  WAIT_FOR_TRUE(archive_finished, 3000);

  QTemporaryDir extract_dir;
  ASSERT_TRUE(extract_dir.isValid());
  ASSERT_EQ(ExtractSync(exchanger, extract_dir.path(),
                        ArchiveExtractPolicy::Permissive()),
            0);

  EXPECT_TRUE(QFileInfo::exists(extract_dir.path() + "/keep/kept.txt"));
  EXPECT_FALSE(QFileInfo::exists(extract_dir.path() + "/drop"));
}

// ------------------------------------------------------------ hostile input

TEST(ArchiveFileOperatorTest, RefusesTraversalAndWritesNothingOutside) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto dest = temp_dir.path() + "/dest";
  ASSERT_TRUE(QDir().mkpath(dest));

  QByteArray tar;
  tar.append(TarHeader("../escape.txt", '0', 5));
  tar.append(TarPad("owned"));
  tar.append(TarEnd());

  EXPECT_NE(
      ExtractSync(ExchangerWith(tar), dest, ArchiveExtractPolicy::Permissive()),
      0);

  // the point of the whole guard: the sibling of the destination is untouched
  EXPECT_FALSE(QFileInfo::exists(temp_dir.path() + "/escape.txt"));
  EXPECT_FALSE(QFileInfo::exists(dest + "/escape.txt"));
}

TEST(ArchiveFileOperatorTest, RefusesAbsoluteEntryName) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto dest = temp_dir.path() + "/dest";
  ASSERT_TRUE(QDir().mkpath(dest));

  const auto victim = temp_dir.path() + "/victim.txt";

  QByteArray tar;
  tar.append(TarHeader(victim.toUtf8(), '0', 5));
  tar.append(TarPad("owned"));
  tar.append(TarEnd());

  EXPECT_NE(
      ExtractSync(ExchangerWith(tar), dest, ArchiveExtractPolicy::Permissive()),
      0);
  EXPECT_FALSE(QFileInfo::exists(victim));
}

TEST(ArchiveFileOperatorTest, StrictRefusesSymlinkPermissiveAllowsIt) {
  QByteArray tar;
  tar.append(TarHeader("link", '2', 0, "/etc/passwd"));
  tar.append(TarEnd());

  {
    QTemporaryDir temp_dir;
    ASSERT_TRUE(temp_dir.isValid());
    EXPECT_NE(ExtractSync(ExchangerWith(tar), temp_dir.path() + "/dest",
                          ArchiveExtractPolicy::Strict()),
              0);
  }
  {
    QTemporaryDir temp_dir;
    ASSERT_TRUE(temp_dir.isValid());
    const auto dest = temp_dir.path() + "/dest";
    ASSERT_TRUE(QDir().mkpath(dest));
    EXPECT_EQ(ExtractSync(ExchangerWith(tar), dest,
                          ArchiveExtractPolicy::Permissive()),
              0);
  }
}

TEST(ArchiveFileOperatorTest, StrictRefusesHardlink) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  QByteArray tar;
  tar.append(TarHeader("a.txt", '0', 3));
  tar.append(TarPad("abc"));
  tar.append(TarHeader("b.txt", '1', 0, "a.txt"));
  tar.append(TarEnd());

  EXPECT_NE(ExtractSync(ExchangerWith(tar), temp_dir.path() + "/dest",
                        ArchiveExtractPolicy::Strict()),
            0);
}

TEST(ArchiveFileOperatorTest, StrictRefusesDeviceAndFifoEntries) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  QByteArray tar;
  tar.append(TarHeader("pipe", '6', 0));  // FIFO
  tar.append(TarEnd());

  EXPECT_NE(ExtractSync(ExchangerWith(tar), temp_dir.path() + "/dest",
                        ArchiveExtractPolicy::Strict()),
            0);
}

TEST(ArchiveFileOperatorTest, EnforcesEntryCountLimit) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  QByteArray tar;
  for (int i = 0; i < 5; ++i) {
    tar.append(TarHeader(QByteArray("f") + QByteArray::number(i), '0', 1));
    tar.append(TarPad("x"));
  }
  tar.append(TarEnd());

  auto policy = ArchiveExtractPolicy::Strict();
  policy.max_entries = 3;

  EXPECT_NE(ExtractSync(ExchangerWith(tar), temp_dir.path() + "/dest", policy),
            0);
}

TEST(ArchiveFileOperatorTest, EnforcesPerEntryAndTotalSizeLimits) {
  {
    QTemporaryDir temp_dir;
    ASSERT_TRUE(temp_dir.isValid());
    QByteArray tar;
    tar.append(TarHeader("big.bin", '0', 4096));
    tar.append(TarPad(QByteArray(4096, 'z')));
    tar.append(TarEnd());

    auto policy = ArchiveExtractPolicy::Strict();
    policy.max_entry_bytes = 1024;
    policy.max_total_bytes = -1;
    EXPECT_NE(
        ExtractSync(ExchangerWith(tar), temp_dir.path() + "/dest", policy), 0);
  }
  {
    QTemporaryDir temp_dir;
    ASSERT_TRUE(temp_dir.isValid());
    QByteArray tar;
    for (int i = 0; i < 4; ++i) {
      tar.append(TarHeader(QByteArray("f") + QByteArray::number(i), '0', 1024));
      tar.append(TarPad(QByteArray(1024, 'z')));
    }
    tar.append(TarEnd());

    auto policy = ArchiveExtractPolicy::Strict();
    policy.max_entry_bytes = 2048;
    policy.max_total_bytes = 2048;
    EXPECT_NE(
        ExtractSync(ExchangerWith(tar), temp_dir.path() + "/dest", policy), 0);
  }
}

TEST(ArchiveFileOperatorTest, StrictRemovesDestinationOnAbort) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto dest = temp_dir.path() + "/dest";

  QByteArray tar;
  tar.append(TarHeader("good.txt", '0', 4));
  tar.append(TarPad("good"));
  tar.append(TarHeader("../escape.txt", '0', 5));
  tar.append(TarPad("owned"));
  tar.append(TarEnd());

  EXPECT_NE(
      ExtractSync(ExchangerWith(tar), dest, ArchiveExtractPolicy::Strict()), 0);

  // a half-extracted tree left behind would later be adopted as a real profile
  EXPECT_FALSE(QFileInfo::exists(dest));
  EXPECT_FALSE(QFileInfo::exists(temp_dir.path() + "/escape.txt"));
}

TEST(ArchiveFileOperatorTest, StrictRefusesNonEmptyDestination) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto dest = temp_dir.path() + "/dest";
  ASSERT_TRUE(QDir().mkpath(dest));
  CreateTestFile(dest, "already_here.txt", "precious");

  QByteArray tar;
  tar.append(TarHeader("a.txt", '0', 1));
  tar.append(TarPad("x"));
  tar.append(TarEnd());

  EXPECT_NE(
      ExtractSync(ExchangerWith(tar), dest, ArchiveExtractPolicy::Strict()), 0);

  // refusing must not destroy what was already there
  EXPECT_TRUE(QFileInfo::exists(dest + "/already_here.txt"));
}

TEST(ArchiveFileOperatorTest, StrictCreatesMissingDestination) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const auto dest = temp_dir.path() + "/fresh";

  QByteArray tar;
  tar.append(TarHeader("a.txt", '0', 3));
  tar.append(TarPad("abc"));
  tar.append(TarEnd());

  EXPECT_EQ(
      ExtractSync(ExchangerWith(tar), dest, ArchiveExtractPolicy::Strict()), 0);
  QFile f(dest + "/a.txt");
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));
  EXPECT_EQ(f.readAll(), QByteArray("abc"));
}

// ------------------------------------------------------------------- legacy

TEST(ArchiveFileOperatorTest, ListArchive) {
  GpgFrontend::ArchiveFileOperator::ListArchive("/tmp/archive.tar");
}

TEST(ArchiveFileOperatorTest, HandleInvalidInput) {
  auto exchanger = CreateStandardGFDataExchanger();
  bool archive_finished = false;
  GpgFrontend::ArchiveFileOperator::NewArchive2DataExchanger(
      "/not/exist", exchanger, [&](int err, DataObjectPtr) {
        EXPECT_NE(err, 0);
        archive_finished = true;
      });
  WAIT_FOR_TRUE(archive_finished, 3000);

  QTemporaryDir temp_dir;
  EXPECT_EQ(ExtractSync(exchanger, temp_dir.path(),
                        ArchiveExtractPolicy::Permissive()),
            0);
}
// ------------------------------------------------- failures must stay visible

#ifndef Q_OS_WINDOWS
TEST(ArchiveFileOperatorTest, AnEntryTheDestinationRefusesFailsTheExtraction) {
  if (geteuid() == 0) {
    GTEST_SKIP() << "root ignores the permission this test relies on";
  }

  QTemporaryDir out_dir;
  ASSERT_TRUE(out_dir.isValid());
  const auto dest = out_dir.path() + "/dest";
  ASSERT_TRUE(QDir().mkpath(dest));

  // Read-only, so creating the entry's file inside it cannot succeed. This is
  // the shape a full disk takes as far as the extractor is concerned: the entry
  // was refused, and it used to be skipped and the extraction called a success,
  // which mounts a profile with files missing.
  ASSERT_TRUE(QFile::setPermissions(
      dest, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

  QByteArray tar;
  tar.append(TarHeader("wanted.txt", '0', 5));
  tar.append(TarPad("hello"));
  tar.append(TarEnd());

  const auto ret =
      GpgFrontend::ArchiveFileOperator::ExtractArchiveFromDataExchangerSync(
          ExchangerWith(tar), dest,
          GpgFrontend::ArchiveExtractPolicy::Permissive());

  // Restored before the assertion, so a failure still leaves a removable dir.
  QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner);

  EXPECT_NE(ret, 0U) << "an entry the destination refused was dropped silently";
  EXPECT_FALSE(QFileInfo::exists(dest + "/wanted.txt"));
}
#endif

TEST(ArchiveFileOperatorTest, ASinkThatRefusesEveryWriteIsNotASuccessfulPack) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  CreateTestFile(temp_dir.path(), "a.txt", "hello");

  // Closed before a byte is asked of it, so every write the archive attempts
  // fails -- including the last one, which a gzip filter only makes at close.
  // That final flush is the one this used to return success in spite of.
  auto exchanger = GpgFrontend::CreateStandardGFDataExchanger();
  exchanger->CloseWrite();

  bool done = false;
  const auto next = [&](GpgFrontend::ArchiveMemberEntry& out) {
    if (done) return false;
    done = true;
    out.relative_path = "a.txt";
    out.source_file = temp_dir.path() + "/a.txt";
    return true;
  };

  EXPECT_NE(GpgFrontend::ArchiveFileOperator::NewArchiveFromMembersSync(
                next, exchanger, GpgFrontend::ArchiveCompression::kGZIP),
            0U)
      << "an archive nothing could be written to was reported as written";
}

TEST(ArchiveFileOperatorTest, AMemberShorterThanItsDeclaredSizeFailsThePack) {
  // A file whose stat size is larger than what reading it yields. sysfs
  // attributes are the reliable example: 4096 declared, a handful of bytes
  // actually there. The header is written from the declared size, so packing
  // one used to emit a null-padded member and report success -- which is what a
  // keyring file hitting a read error would have done.
  const QString probe = "/sys/devices/system/cpu/online";
  const QFileInfo info(probe);
  if (!info.isReadable() || info.size() <= 0) {
    GTEST_SKIP() << "no file with an over-declared size available here";
  }
  {
    QFile f(probe);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    if (f.readAll().size() >= info.size()) {
      GTEST_SKIP() << probe.toStdString() << " does not over-declare its size";
    }
  }

  auto exchanger = GpgFrontend::CreateStandardGFDataExchanger();
  GpgFrontend::GFError ret = 0;
  bool done = false;
  const auto next = [&](GpgFrontend::ArchiveMemberEntry& out) {
    if (done) return false;
    done = true;
    out.relative_path = "short.txt";
    out.source_file = probe;
    return true;
  };

  std::thread producer([&]() {
    ret = GpgFrontend::ArchiveFileOperator::NewArchiveFromMembersSync(
        next, exchanger, GpgFrontend::ArchiveCompression::kNONE);
  });

  QByteArray chunk(64 * 1024, Qt::Uninitialized);
  while (exchanger->Read(reinterpret_cast<std::byte*>(chunk.data()),
                         chunk.size()) > 0) {
  }
  producer.join();

  EXPECT_NE(ret, 0U) << "a member that came up short was padded and passed off "
                        "as complete";
}
}  // namespace GpgFrontend::Test
