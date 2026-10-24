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

#ifndef _WIN32
#include <unistd.h>
#endif

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

auto FindField(const QList<QPair<QString, QString>>& fields, const QString& key)
    -> QString {
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
            u"payload.bin"_s);
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
    EXPECT_TRUE(text.contains(u"- "_s + f.first + sep + f.second))
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

TEST(IOUtilsSafeOutputTest, CommitReplacingADirectoryLeavesNoBackup) {
  // An archive decryption stages a directory. Replacing an existing output
  // directory must not leave the old one behind as a hidden backup.
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/archive";
  ASSERT_TRUE(QDir().mkpath(final_path));
  ASSERT_TRUE(WriteFile(final_path + "/old.txt", QByteArray("older")));

  const QString staged = MakeSafeOutputTempPath(final_path);
  ASSERT_TRUE(QDir().mkpath(staged));
  ASSERT_TRUE(WriteFile(staged + "/new.txt", QByteArray("newer")));

  EXPECT_TRUE(CommitSafeOutputFile(staged, final_path));
  EXPECT_TRUE(QFileInfo::exists(final_path + "/new.txt"));
  EXPECT_FALSE(QFileInfo::exists(final_path + "/old.txt"));

  const auto leftovers =
      QDir(temp_dir.path())
          .entryList({"*gpgfrontend.backup*"},
                     QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
  EXPECT_TRUE(leftovers.isEmpty()) << leftovers.join(", ").toStdString();
}

TEST(IOUtilsSafeOutputTest, CommitReplacingAReadOnlyTreeLeavesNoBackup) {
  // A previous archive extraction can contain read-only directories. The
  // replaced tree is the old plaintext: it must not survive as a hidden
  // backup just because one of its directories was not writable.
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/archive";
  ASSERT_TRUE(QDir().mkpath(final_path + "/locked"));
  ASSERT_TRUE(WriteFile(final_path + "/locked/old.txt", QByteArray("older")));
  ASSERT_TRUE(QFile::setPermissions(
      final_path + "/locked", QFileDevice::ReadOwner | QFileDevice::ExeOwner));

  const QString staged = MakeSafeOutputTempPath(final_path);
  ASSERT_TRUE(QDir().mkpath(staged));
  ASSERT_TRUE(WriteFile(staged + "/new.txt", QByteArray("newer")));

  EXPECT_TRUE(CommitSafeOutputFile(staged, final_path));
  EXPECT_TRUE(QFileInfo::exists(final_path + "/new.txt"));

  const auto leftovers =
      QDir(temp_dir.path())
          .entryList({"*gpgfrontend.backup*"},
                     QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
  EXPECT_TRUE(leftovers.isEmpty()) << leftovers.join(", ").toStdString();

  // Let QTemporaryDir clean up whatever a failure left behind.
  for (const auto& leftover : leftovers) {
    QFile::setPermissions(temp_dir.path() + "/" + leftover + "/locked",
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                              QFileDevice::ExeOwner);
  }
}

TEST(IOUtilsSafeOutputTest, DiscardRemovesAStagedTreeWithReadOnlyDirs) {
  // An extraction that is discarded must not leave plaintext behind in a
  // directory the archive marked read-only.
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString staged = MakeSafeOutputTempPath(temp_dir.path() + "/archive");
  ASSERT_TRUE(QDir().mkpath(staged + "/locked"));
  ASSERT_TRUE(WriteFile(staged + "/locked/secret.txt", QByteArray("plain")));
  ASSERT_TRUE(QFile::setPermissions(
      staged + "/locked", QFileDevice::ReadOwner | QFileDevice::ExeOwner));

  EXPECT_TRUE(DiscardSafeOutput(staged));
  EXPECT_FALSE(QFileInfo::exists(staged));

  if (QFileInfo::exists(staged + "/locked")) {
    QFile::setPermissions(staged + "/locked", QFileDevice::ReadOwner |
                                                  QFileDevice::WriteOwner |
                                                  QFileDevice::ExeOwner);
  }
}

TEST(IOUtilsWriteGFBufferTest, WrittenFileIsOwnerOnlyAndRoundTrips) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString path = temp_dir.path() + "/secret.asc";
  ASSERT_TRUE(WriteFileGFBuffer(path, GFBuffer(QString("secret key"))));

  auto [ok, read_back] = ReadFileGFBuffer(path);
  ASSERT_TRUE(ok);
  EXPECT_EQ(read_back.ConvertToQByteArray(), QByteArray("secret key"));

#ifndef _WIN32
  EXPECT_EQ(QFileInfo(path).permissions() &
                (QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                 QFileDevice::ExeGroup | QFileDevice::ReadOther |
                 QFileDevice::WriteOther | QFileDevice::ExeOther),
            QFileDevice::Permissions{});
#endif
}

TEST(IOUtilsWriteGFBufferTest, ExistingFileIsReplacedNotTruncatedInPlace) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString path = temp_dir.path() + "/secret.asc";
  ASSERT_TRUE(WriteFile(path, QByteArray("an older and longer content")));
  ASSERT_TRUE(QFile::setPermissions(
      path, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                QFileDevice::ReadGroup | QFileDevice::ReadOther));

#ifndef _WIN32
  // A second name for the old file. An in-place truncate rewrites the file
  // this name points at; an atomic replace gives the path a new file and
  // leaves the old one whole until it is dropped.
  const QString alias = temp_dir.path() + "/alias.asc";
  ASSERT_EQ(
      ::link(path.toLocal8Bit().constData(), alias.toLocal8Bit().constData()),
      0);
#endif

  ASSERT_TRUE(WriteFileGFBuffer(path, GFBuffer(QString("newer"))));

  QByteArray content;
  ASSERT_TRUE(ReadFile(path, content));
  EXPECT_EQ(content, QByteArray("newer"));

#ifndef _WIN32
  QByteArray alias_content;
  ASSERT_TRUE(ReadFile(alias, alias_content));
  EXPECT_EQ(alias_content, QByteArray("an older and longer content"));

  EXPECT_EQ(QFileInfo(path).permissions() &
                (QFileDevice::ReadGroup | QFileDevice::ReadOther),
            QFileDevice::Permissions{});
#endif
}

TEST(IOUtilsWriteGFBufferTest, ALinkAtTheTargetIsNotFollowed) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString victim = temp_dir.path() + "/victim.txt";
  ASSERT_TRUE(WriteFile(victim, QByteArray("keep")));

  const QString path = temp_dir.path() + "/secret.asc";
  ASSERT_TRUE(QFile::link(victim, path));

  EXPECT_FALSE(WriteFileGFBuffer(path, GFBuffer(QString("secret key"))));

  QByteArray content;
  ASSERT_TRUE(ReadFile(victim, content));
  EXPECT_EQ(content, QByteArray("keep"));
}

TEST(IOUtilsSafeOutputTest, WorkDirsArePrivateAndUnpredictable) {
  // A predictable, world-readable work directory in a shared folder can be
  // created in advance by someone else, who could then plant links in it.
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString final_path = temp_dir.path() + "/report.txt.gpg";
  const QString first = MakeSafeOutputTempPath(final_path);
  const QString second = MakeSafeOutputTempPath(final_path);
  const QString first_dir = QFileInfo(first).absolutePath();
  const QString second_dir = QFileInfo(second).absolutePath();

  EXPECT_NE(first_dir, second_dir);

  // Both live in a fresh `.gpgfrontend.tmp.<random>` directory: the template
  // placeholder was replaced by a real random suffix, not kept literally.
  const QString prefix = u".gpgfrontend.tmp."_s;
  for (const auto& dir : {first_dir, second_dir}) {
    const QString name = QFileInfo(dir).fileName();
    ASSERT_TRUE(name.startsWith(prefix)) << name.toStdString();
    const QString suffix = name.mid(prefix.size());
    EXPECT_EQ(suffix.size(), 6) << name.toStdString();
    EXPECT_NE(suffix, u"XXXXXX"_s);
  }
  EXPECT_NE(QFileInfo(first_dir).fileName().mid(prefix.size()),
            QFileInfo(second_dir).fileName().mid(prefix.size()));

  EXPECT_EQ(QFileInfo(first_dir).permissions() &
                (QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                 QFileDevice::ExeGroup | QFileDevice::ReadOther |
                 QFileDevice::WriteOther | QFileDevice::ExeOther),
            QFileDevice::Permissions{});
}

TEST(IOUtilsSafeOutputTest, DiscardRemovesAStagedDirectoryWithItsContents) {
  // A failed batch must not leave an extracted plaintext tree behind.
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString staged = MakeSafeOutputTempPath(temp_dir.path() + "/archive");
  ASSERT_TRUE(QDir().mkpath(staged + "/nested"));
  ASSERT_TRUE(WriteFile(staged + "/nested/secret.txt", QByteArray("plain")));

  EXPECT_TRUE(DiscardSafeOutput(staged));
  EXPECT_FALSE(QFileInfo::exists(staged));

  RemoveSafeOutputWorkDir(staged);
  EXPECT_FALSE(QFileInfo::exists(QFileInfo(staged).absolutePath()));
}

TEST(IOUtilsSafeOutputTest, DiscardRemovesAStagedFileAndToleratesNone) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString staged = MakeSafeOutputTempPath(temp_dir.path() + "/out.txt");
  EXPECT_TRUE(DiscardSafeOutput(staged));  // nothing written yet
  ASSERT_TRUE(WriteFile(staged, QByteArray("plain")));
  EXPECT_TRUE(DiscardSafeOutput(staged));
  EXPECT_FALSE(QFileInfo::exists(staged));
  EXPECT_TRUE(DiscardSafeOutput({}));
}

TEST(IOUtilsSafeOutputTest, DiscardRemovesALinkWithoutFollowingIt) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());

  const QString victim = temp_dir.path() + "/victim";
  ASSERT_TRUE(QDir().mkpath(victim));
  ASSERT_TRUE(WriteFile(victim + "/keep.txt", QByteArray("keep")));

  const QString staged = MakeSafeOutputTempPath(temp_dir.path() + "/out");
  ASSERT_TRUE(QFile::link(victim, staged));

  EXPECT_TRUE(DiscardSafeOutput(staged));
  EXPECT_FALSE(QFileInfo(staged).isSymLink());
  EXPECT_TRUE(QFileInfo::exists(victim + "/keep.txt"));
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
