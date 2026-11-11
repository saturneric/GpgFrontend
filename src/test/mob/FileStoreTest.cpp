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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "MobileTest.h"
#include "files/ShareRetentionStore.h"
#include "files/StagingArea.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::RetainedFile;
using GpgFrontend::Mobile::ShareRetentionStore;
using GpgFrontend::Mobile::StagedFile;
using GpgFrontend::Mobile::StagingArea;

namespace {

auto Write(const QString& path, const QByteArray& content) -> bool {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  return file.open(QIODevice::WriteOnly) &&
         file.write(content) == content.size();
}

auto Read(const QString& path) -> QByteArray {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

auto T0() -> QDateTime {
  return QDateTime(QDate(2026, 11, 2), QTime(12, 0), QTimeZone::UTC);
}

/// A store over a temporary directory, recording what it revoked.
class RetentionTest : public ::testing::Test {
 protected:
  QTemporaryDir dir_;
  QStringList revoked_;

  auto Root() const -> QString { return dir_.path() + "/share"; }
  auto Index() const -> QString { return dir_.path() + "/index.json"; }

  auto Make(ShareRetentionStore::Policy policy = {})
      -> std::unique_ptr<ShareRetentionStore> {
    return std::make_unique<ShareRetentionStore>(
        Root(), Index(), policy,
        [this](const QString& path) { revoked_.append(path); });
  }

  /// A staged file of @p size bytes, ready to retain.
  auto Staged(const QString& name, int size = 4) -> QString {
    const auto path = dir_.path() + "/staging/" + name;
    Write(path, QByteArray(size, 'x'));
    return path;
  }
};

}  // namespace

TEST_F(RetentionTest, ASharedFileIsMovedInAndRecorded) {
  auto store = Make();
  const auto staged = Staged("report.pdf.gpg");
  const auto entry = store->Retain(staged, "report.pdf.gpg", false, T0());
  ASSERT_TRUE(entry.has_value());

  EXPECT_FALSE(QFile::exists(staged));
  EXPECT_TRUE(QFile::exists(entry->path));
  EXPECT_EQ(QFileInfo(entry->path).fileName(), "report.pdf.gpg");
  EXPECT_TRUE(entry->path.startsWith(Root() + "/"));
  EXPECT_EQ(entry->eligible, T0().addSecs(24 * 3600));
  EXPECT_EQ(store->Entries().size(), 1);
}

TEST_F(RetentionTest, AFileStaysUntilItIsEligibleThenGoesAtTheNextChance) {
  auto store = Make();
  // Not decrypted: kept the day (a decrypted one, an hour).
  const auto entry = store->Retain(Staged("a.txt"), "a.txt", false, T0());
  ASSERT_TRUE(entry.has_value());

  // A minute short of a day: kept.
  store->Cleanup(T0().addSecs(24 * 3600 - 60));
  EXPECT_TRUE(QFile::exists(entry->path));
  EXPECT_TRUE(revoked_.isEmpty());

  // Past it: revoked, then deleted with its directory.
  store->Cleanup(T0().addSecs(24 * 3600 + 1));
  EXPECT_FALSE(QFile::exists(entry->path));
  EXPECT_FALSE(QFileInfo::exists(QFileInfo(entry->path).absolutePath()));
  EXPECT_EQ(revoked_, QStringList{entry->path});
  EXPECT_TRUE(store->Entries().isEmpty());
}

TEST_F(RetentionTest, AFileSharedUnderAWrongClockStillGoes) {
  // Shared while the clock read a year ahead, then the clock was put right:
  // due "in a year" is not a reason to keep plaintext for one.
  auto store = Make();
  const auto entry =
      store->Retain(Staged("a.txt"), "a.txt", true, T0().addYears(1));
  ASSERT_TRUE(entry.has_value());

  store->Cleanup(T0());
  EXPECT_FALSE(QFile::exists(entry->path));
  EXPECT_TRUE(store->Entries().isEmpty());
}

TEST_F(RetentionTest, AShareNobodySawIsTakenBack) {
  auto store = Make();
  const auto entry = store->Retain(Staged("a.txt"), "a.txt", true, T0());
  ASSERT_TRUE(entry.has_value());

  const auto back = dir_.path() + "/staging/back/a.txt";
  QDir().mkpath(QFileInfo(back).absolutePath());
  ASSERT_TRUE(store->Withdraw(entry->id, back));
  EXPECT_EQ(Read(back), QByteArray(4, 'x'));
  EXPECT_FALSE(QFile::exists(entry->path));
  EXPECT_EQ(revoked_, QStringList{entry->path});
  EXPECT_TRUE(store->Entries().isEmpty());

  EXPECT_FALSE(store->Withdraw(entry->id, back));
}

TEST_F(RetentionTest, TheRecordSurvivesARestart) {
  RetainedFile kept;
  {
    auto store = Make();
    kept = *store->Retain(Staged("b.txt"), "b.txt", true, T0());
  }
  auto reopened = Make();
  ASSERT_EQ(reopened->Entries().size(), 1);
  const auto entry = reopened->Entries().front();
  EXPECT_EQ(entry.path, kept.path);
  EXPECT_TRUE(entry.plaintext);
  EXPECT_EQ(entry.eligible, kept.eligible);

  // And is still cleaned on time after it.
  reopened->Cleanup(T0().addDays(2));
  EXPECT_FALSE(QFile::exists(kept.path));
}

TEST_F(RetentionTest, TheCapsEvictTheOldestButNeverTheNewFile) {
  ShareRetentionStore::Policy policy;
  policy.max_files = 2;
  auto store = Make(policy);
  const auto first = *store->Retain(Staged("1"), "1", false, T0());
  const auto second = *store->Retain(Staged("2"), "2", false, T0().addSecs(10));
  const auto third = *store->Retain(Staged("3"), "3", false, T0().addSecs(20));

  EXPECT_FALSE(QFile::exists(first.path));
  EXPECT_TRUE(QFile::exists(second.path));
  EXPECT_TRUE(QFile::exists(third.path));
  EXPECT_EQ(revoked_, QStringList{first.path});

  // A file larger than the byte cap on its own is still kept: it is the one
  // just shared.
  ShareRetentionStore::Policy tight;
  tight.max_bytes = 8;
  auto small = Make(tight);
  const auto big = small->Retain(Staged("big", 64), "big", false, T0());
  ASSERT_TRUE(big.has_value());
  EXPECT_TRUE(QFile::exists(big->path));
}

TEST_F(RetentionTest, ADecryptedFileIsKeptOnlyAnHour) {
  auto store = Make();
  const auto plain = *store->Retain(Staged("p"), "p", true, T0());
  const auto sealed = *store->Retain(Staged("s"), "s", false, T0());
  EXPECT_EQ(plain.eligible, T0().addSecs(3600));
  store->Cleanup(T0().addSecs(3601));
  EXPECT_FALSE(QFile::exists(plain.path));
  EXPECT_TRUE(QFile::exists(sealed.path));
}

TEST_F(RetentionTest, ClearingRemovesEverythingAtOnce) {
  auto store = Make();
  const auto a = *store->Retain(Staged("a"), "a", true, T0());
  const auto b = *store->Retain(Staged("b"), "b", false, T0());
  store->Clear();

  EXPECT_FALSE(QFile::exists(a.path));
  EXPECT_FALSE(QFile::exists(b.path));
  EXPECT_EQ(revoked_.size(), 2);
  EXPECT_TRUE(store->Entries().isEmpty());
  EXPECT_TRUE(QDir(Root())
                  .entryList(QDir::NoDotAndDotDot | QDir::AllEntries)
                  .isEmpty());
  EXPECT_TRUE(Make()->Entries().isEmpty());
}

TEST_F(RetentionTest, OrphansOnBothSidesAreDropped) {
  auto store = Make();
  const auto recorded = *store->Retain(Staged("kept"), "kept", false, T0());
  const auto lost = *store->Retain(Staged("lost"), "lost", false, T0());

  // A file nobody recorded, and a record whose file is gone.
  const auto stray = Root() + "/stray-session/leftover.txt";
  Write(stray, "left behind");
  QFile::remove(lost.path);

  store->Cleanup(T0());
  EXPECT_FALSE(QFileInfo::exists(QFileInfo(stray).absolutePath()));
  EXPECT_TRUE(revoked_.contains(stray));
  ASSERT_EQ(store->Entries().size(), 1);
  EXPECT_EQ(store->Entries().front().path, recorded.path);
}

TEST_F(RetentionTest, ARecordNeverReachesOutsideTheRoot) {
  // A record naming something outside the root, as a tampered or corrupt
  // index might.
  const auto outside = dir_.path() + "/precious.txt";
  Write(outside, "do not touch");
  QJsonArray files{
      QJsonObject{{"id", ".."},
                  {"name", "precious.txt"},
                  {"eligible", "2000-01-01T00:00:00Z"}},
      QJsonObject{{"id", "x"},
                  {"name", "../../precious.txt"},
                  {"eligible", "2000-01-01T00:00:00Z"}},
  };
  QFile index(Index());
  ASSERT_TRUE(index.open(QIODevice::WriteOnly));
  index.write(
      QJsonDocument(QJsonObject{{"version", 1}, {"files", files}}).toJson());
  index.close();

  auto store = Make();
  EXPECT_TRUE(store->Entries().isEmpty());
  store->Cleanup(T0());
  store->Clear();
  EXPECT_EQ(Read(outside), "do not touch");
}

TEST(StagingAreaTest, EachFileGetsItsOwnPathAndTheSessionGoesWithIt) {
  QTemporaryDir root;
  QString session;
  {
    StagingArea staging(root.path());
    session = staging.Directory();
    const auto a = staging.NewPath("same.txt");
    const auto b = staging.NewPath("same.txt");
    EXPECT_NE(a, b);
    EXPECT_EQ(QFileInfo(a).fileName(), "same.txt");
    EXPECT_TRUE(a.startsWith(session + "/"));
    ASSERT_TRUE(Write(a, "staged"));
  }
  EXPECT_FALSE(QFileInfo::exists(session));
}

TEST(StagingAreaTest, ANameIsAlwaysOnePathComponent) {
  using GpgFrontend::Mobile::SafeFileName;
  EXPECT_EQ(SafeFileName("report.pdf"), "report.pdf");
  EXPECT_EQ(SafeFileName("a/b/report.pdf"), "report.pdf");
  EXPECT_EQ(SafeFileName(".."), "file");
  EXPECT_EQ(SafeFileName("."), "file");
  EXPECT_EQ(SafeFileName(""), "file");

  QTemporaryDir root;
  StagingArea staging(root.path());
  const auto path = staging.NewPath("..");
  EXPECT_EQ(QFileInfo(path).fileName(), "file");
  EXPECT_TRUE(path.startsWith(staging.Directory() + "/"));
}

TEST(StagingAreaTest, OnlySessionsNoLiveProcessHoldsAreOrphans) {
  QTemporaryDir root;
  StagingArea live(root.path());
  Write(live.NewPath("in-use"), "busy");

  // A session left by a process that died: no lock anyone holds.
  const auto dead = root.path() + "/dead-session";
  Write(dead + "/0/leftover", "old");

  StagingArea::RemoveOrphans(root.path());
  EXPECT_FALSE(QFileInfo::exists(dead));
  EXPECT_TRUE(QFileInfo::exists(live.Directory()));
}

TEST(StagingAreaTest, AStagedFileIsDeletedUnlessReleased) {
  QTemporaryDir root;
  StagingArea staging(root.path());
  const auto dropped = staging.NewPath("dropped");
  const auto kept = staging.NewPath("kept");
  Write(dropped, "x");
  Write(kept, "y");
  {
    StagedFile a(dropped);
    StagedFile b(kept);
    EXPECT_EQ(b.Release(), kept);
    EXPECT_TRUE(b.Empty());

    // Moving hands the file over; the moved-from one deletes nothing.
    StagedFile c(std::move(a));
    EXPECT_TRUE(a.Empty());  // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(c.Path(), dropped);
  }
  EXPECT_FALSE(QFile::exists(dropped));
  EXPECT_TRUE(QFile::exists(kept));
}

}  // namespace GpgFrontend::Test::Mobile
