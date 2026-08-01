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

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryDir>

#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

namespace {

auto WriteTempFile(const QString& dir, const QString& name,
                   const QByteArray& content) -> QString {
  const QString path = dir + "/" + name;
  QFile f(path);
  EXPECT_TRUE(f.open(QIODevice::WriteOnly));
  f.write(content);
  f.close();
  return path;
}

auto FindField(const QContainer<QPair<QString, QString>>& fields,
               const QString& key) -> QString {
  for (const auto& f : fields) {
    if (f.first == key) return f.second;
  }
  return {};
}

}  // namespace

TEST(IOUtilsHashTest, CalculateFileHashInfoProducesStructuredFields) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QByteArray content = "GpgFrontend file hash test payload";
  const QString path = WriteTempFile(temp_dir.path(), "payload.bin", content);

  const auto fields = CalculateFileHashInfo(path);

  // The card was rendering empty because the fields were dropped; the contract
  // here is that the structured data is never empty for a readable file.
  ASSERT_FALSE(fields.isEmpty());

  EXPECT_EQ(
      FindField(fields, "MD5"),
      QString::fromLatin1(
          QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex()));
  EXPECT_EQ(
      FindField(fields, "SHA1"),
      QString::fromLatin1(
          QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex()));
  EXPECT_EQ(FindField(fields, "SHA256"),
            QString::fromLatin1(
                QCryptographicHash::hash(content, QCryptographicHash::Sha256)
                    .toHex()));
  EXPECT_EQ(FindField(fields, QCoreApplication::tr("Filename")),
            QStringLiteral("payload.bin"));
}

TEST(IOUtilsHashTest, FormatFileHashInfoRendersEveryField) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString path =
      WriteTempFile(temp_dir.path(), "doc.txt", "some bytes here");

  const auto fields = CalculateFileHashInfo(path);
  const QString text = FormatFileHashInfo(fields);

  EXPECT_TRUE(text.contains(QCoreApplication::tr("File Hash Information")));

  // Every structured field must appear as a "- <key><sep><value>" line, using
  // the localized separator so the rendered report stays consistent with the
  // data it was built from.
  const QString sep = QCoreApplication::tr(": ");
  for (const auto& f : fields) {
    EXPECT_TRUE(text.contains(QStringLiteral("- ") + f.first + sep + f.second))
        << "missing field: " << f.first.toStdString();
  }
}

TEST(IOUtilsHashTest, CalculateHashOnUnreadablePathReportsError) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString missing = temp_dir.path() + "/does-not-exist.bin";

  EXPECT_TRUE(CalculateFileHashInfo(missing).isEmpty());

  const QString text = CalculateHash(missing);
  EXPECT_TRUE(
      text.contains(QCoreApplication::tr("Error: cannot read target file")));
}

TEST(IOUtilsSafeOutputTest, TempPathIsStagedOutsideTheOutputDirectory) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/report.txt.gpg";
  const QString staged = MakeSafeOutputTempPath(final_path);

  ASSERT_FALSE(staged.isEmpty());

  // The staged file must not sit next to the final one: that directory is the
  // one being watched by the file browser.
  const QFileInfo staged_info(staged);
  EXPECT_NE(staged_info.absolutePath(), QDir(temp_dir.path()).absolutePath());
  EXPECT_EQ(QFileInfo(staged_info.absolutePath()).absolutePath(),
            QDir(temp_dir.path()).absolutePath());
  EXPECT_TRUE(QFileInfo(staged_info.absolutePath()).isDir());
  EXPECT_FALSE(QFileInfo::exists(staged));
}

TEST(IOUtilsSafeOutputTest, TempPathsForTheSameOutputDoNotCollide) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/report.txt.gpg";

  const QString first = MakeSafeOutputTempPath(final_path);
  ASSERT_TRUE(WriteFile(first, QByteArray("first")));

  const QString second = MakeSafeOutputTempPath(final_path);
  EXPECT_NE(first, second);
}

TEST(IOUtilsSafeOutputTest, CommitMovesStagedOutputIntoPlace) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/report.txt.gpg";
  const QString staged = MakeSafeOutputTempPath(final_path);
  ASSERT_TRUE(WriteFile(staged, QByteArray("ciphertext")));

  EXPECT_TRUE(CommitSafeOutputFile(staged, final_path));

  EXPECT_FALSE(QFileInfo::exists(staged));
  QByteArray content;
  ASSERT_TRUE(ReadFile(final_path, content));
  EXPECT_EQ(content, QByteArray("ciphertext"));
}

TEST(IOUtilsSafeOutputTest, CommitReplacesAnExistingOutput) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/report.txt.gpg";
  ASSERT_TRUE(WriteFile(final_path, QByteArray("older")));

  const QString staged = MakeSafeOutputTempPath(final_path);
  ASSERT_TRUE(WriteFile(staged, QByteArray("newer")));

  EXPECT_TRUE(CommitSafeOutputFile(staged, final_path));

  QByteArray content;
  ASSERT_TRUE(ReadFile(final_path, content));
  EXPECT_EQ(content, QByteArray("newer"));

  // The backup taken while replacing must not survive the commit.
  const auto leftovers =
      QDir(temp_dir.path())
          .entryList({"*gpgfrontend.backup*"}, QDir::Files | QDir::Hidden);
  EXPECT_TRUE(leftovers.isEmpty());
}

TEST(IOUtilsSafeOutputTest, CommitOfAMissingStagedFileKeepsTheOriginal) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/report.txt.gpg";
  ASSERT_TRUE(WriteFile(final_path, QByteArray("original")));

  const QString staged = MakeSafeOutputTempPath(final_path);
  EXPECT_FALSE(CommitSafeOutputFile(staged, final_path));

  QByteArray content;
  ASSERT_TRUE(ReadFile(final_path, content));
  EXPECT_EQ(content, QByteArray("original"));
}

TEST(IOUtilsSafeOutputTest, WorkDirIsRemovedOnlyOnceItIsEmpty) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/report.txt.gpg";
  const QString staged = MakeSafeOutputTempPath(final_path);
  const QString work_dir = QFileInfo(staged).absolutePath();
  ASSERT_TRUE(WriteFile(staged, QByteArray("ciphertext")));

  // A staged file still in the directory means the operation left something
  // behind — keep it rather than deleting the evidence.
  RemoveSafeOutputWorkDir(staged);
  EXPECT_TRUE(QFileInfo::exists(work_dir));

  ASSERT_TRUE(QFile::remove(staged));
  RemoveSafeOutputWorkDir(staged);
  EXPECT_FALSE(QFileInfo::exists(work_dir));
}

TEST(IOUtilsSafeOutputTest, RemoveWorkDirIgnoresPathsItDidNotStage) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString foreign = temp_dir.path() + "/plain.txt";
  ASSERT_TRUE(WriteFile(foreign, QByteArray("data")));

  RemoveSafeOutputWorkDir(foreign);
  RemoveSafeOutputWorkDir({});

  EXPECT_TRUE(QFileInfo::exists(temp_dir.path()));
  EXPECT_TRUE(QFileInfo::exists(foreign));
}

}  // namespace GpgFrontend::Test
